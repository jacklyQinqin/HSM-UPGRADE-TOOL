/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
/**
  ******************************************************************************
  * File name: hsm_hardware_level.c
  * Description: 
  * Auth : qin xiaodong 
  * Email: qinxd@istecc.com
  * API VERSION: 0.3
  * Time : 2023-3-15
  ******************************************************************************
  * NOTE:
  * The following test code for the hardware part, based on the author's NXP - IMX8MQ environment implementation. 
  * If you use your own hardware environment, you need to change the parameters corresponding to the hardware. 
  * For example, SPI device path, and IO parameters and so on.
  *	HISTORY:  
  * 
  ******************************************************************************
  */

#include "hsm_hardware_level.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "hsm_gpio.h"



/*FOR MUL PROCESS ,TRY  TO ADD SEMPOHRE FOR INIT*/
/*Creat a semphore.*/
#include <sys/sem.h>
//#include <semun.h>
static union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short * array;
    struct seminfo *__buf;
    /* data */
};


static int hardware_semphore_id;
int HardwareSetSemphre(void)
{
    union semun sem_union;
    sem_union.val = 1;

    if(semctl(hardware_semphore_id,0,SETVAL,sem_union) == -1)
        return 0;
    return 1;
    
}
void HardwareDeleteSemphre(void)
{
    union semun sem_union;
    sem_union.val = 1;

    if(semctl(hardware_semphore_id,0,IPC_RMID,sem_union) == -1)
       fprintf(stderr,"Failed to delete semphore.\n");
}
/*信号量-1操作*/
int HardwarePSemphre(void)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(hardware_semphore_id,&sem_b,1)  == -1)
    {
        fprintf(stderr,"Failed to HSMPSemphre.\n");
        return 0;
    }
    printf(
        "P"
    );
    return 1;
}
/*信号量+1操作*/
int HardwareVSemphre(void)
{
    
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(hardware_semphore_id,&sem_b,1)  == -1)
    {
        fprintf(stderr,"Failed to HSMVSemphre.\n");
        return 0;
    }
    printf(
        "V"
    );
    return 1;
}

/*creat semphore or get key*/
int HardwareSempohreInit(void)
{
    hardware_semphore_id = semget((key_t)3210,1,0666|IPC_CREAT);
    printf("get sem id %4d\n",hardware_semphore_id);
    return hardware_semphore_id;
}

int HardwareSempohreDeInit(void)
{
    HardwareDeleteSemphre();
    return 0;
}


int HardwareGetSem(void)
{
    union semun sem_union;
    sem_union.val = -2;

    sem_union.val = semctl(hardware_semphore_id,0,GETVAL,sem_union) ;
    //printf("the value is %4d\n",sem_union.val);    
    return  sem_union.val;
}



/*SPI default device name , and number of busy io, reset io*/
char SPI_DEV_NAME[100] = "/dev/spidev32766.0";
int busy = 98;
int reset = 96;

#define HSM_HARDWARE_DEBUG 0


//hardware init.
//spi init
//and handshake init.
/*Linux environment, SPI descriptor.*/
static int fd;
static int  mode;
/*bit mode : byte mode.(8bit)*/
static char bits = DEFAULT_MODE;
/*default spi clk speed.*/
uint32_t  speed = 5000000;
static uint16_t delay = 50;
static int verbose;


/*
* for NXP.IMX8MQ
*IMX8MQ.CS  ---->IS32U512A  
*IMX8MQ.CLK ---->IS32U512A  
*IMX8MQ.MOSI---->IS32U512A  
*IMX8MQ.MISO<----IS32U512A  
*IMX8MQ.BUSY<----IS32U512A  
*IMX8MQ.RESET--->IS32U512A  
*/

GpioMessageTyepdef busyPin = 
{
	.pin = 98,
	.direction = GPIO_IN,
};

//mesg csPin = 
//{
//	.pin = 110,
//	.direction = GPIO_OUT,
//};
GpioMessageTyepdef resetPin = 
{
	.pin = 96,
	.direction = GPIO_OUT,
};


/****************************************************************\
* Function:	    hex_dump
*
* Description:  Convert the 16 into the data to print the output for the character 
*
* Calls:
*				None
*
* Called By:
*
* Input:       src 			hex data
*              length   	length of hex data
			   line_size	The length of each line 
			   prefix		prefix of each line
*				
* Output:	   None
*
*
* Return:
*			
*
* Others:
*			None
*
* Remark:	  
*
\****************************************************************/

void hex_dump(const void *src, size_t length, size_t line_size, char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;
	printf("\n");
	printf("%s\n", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			// printf(" | ");  /* right close */
			// while (line < address) {
			// 	c = *line++;
			// 	printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			// }
			
			printf("\n");
			if (length > 0){
				printf("\n");
				//printf("%s | \n", prefix);
			}
			
		}
	}
	printf("\n");
}


/**
 * @brief 
 * In Linux, you can use export to initialize the I/O direction and value.
 * In this routine, we export an IO as BUSY and another IO as reset
 * @return unsigned int 
 */
static unsigned int ExportGpioAndInit(void)
{
	// /*导出需要的3个IO*/ 
	// gpio_export(&busyPin);
	// //gpio_export(&csPin);
	// gpio_export(&resetPin);
	// /*设置方向*/ 
	// gpio_direction(&busyPin);
	// //gpio_direction(&csPin);
	// gpio_direction(&resetPin);
	// /*设置默认值*/
	// //gpio_write(&csPin,1);
	// gpio_write(&resetPin,1);
	
	return 0;	
} 


/*Release IO resources */
static void UnexportGpioAndInit(void)
{
	// gpio_unexport(&busyPin);
	// //gpio_unexport(&csPin);
	// gpio_unexport(&resetPin);
}




// unsigned int hsm_set_speed(unsigned long in_speed)
// {
//     speed = in_speed;
// }



/****************************************************************\
* Function:	    HSMHardwareInit
*
* Description:  Master SOC initializes SPI interfaces connected to HSM and IO such as RESET and BUSY.
*
* Calls:
*				None
*
* Called By:
*
* Input:       	in_speed		SPI CLK_SPEED 			   
*				
* Output:	   	int
*
*
* Return:
*			
*
* Others:
*				None
*
* Remark:	    1.Don't exceed 20Mbps best 
*				2.This initialization is based on my NXP IMX8 test environment. 
* Please refer to your own hardware circuit for initialization of the hardware part, 
* do not directly use. This partial implementation is for demonstration use only.	
\****************************************************************/
unsigned int  HSMHardwareInit(unsigned long in_speed)
{
	int time  = 0;
    int ret = 0;
    speed = in_speed;
	int sem_status = -1;
    printf("hsm_hardware_init and the speed is %ld \n",in_speed);
    fd = open(SPI_DEV_NAME, O_RDWR);
    if (fd < 0)
        printf("can't open device");
    /*
     * spi mode
     */
    ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
    if (ret == -1)
        printf("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
    if (ret == -1)
        printf("can't get spi mode");

    /*
     * bits per word
     */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        printf("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        printf("can't get bits per word");

    /*
     * max speed hz
     */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &in_speed);
    if (ret == -1)
        printf("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &in_speed);
    if (ret == -1)
        printf("can't get max speed hz");


    printf("spi mode: 0x%x\n", mode);
    printf("bits per word: %d\n", bits);
	printf("spi fd is: %d\n", fd);


	/*add semphore init  and mutex init at this.*/
	/*初始化逻辑层信号量*/
	HSMSempohreInit();
	/*初始化HARDWARE层信号量*/
	HardwareSempohreInit();
	/*初始化线程锁*/
	HSMThreadMutexInit();

	ret = HardwareGetSem();
	if(ret != 1)
	{
		printf("FIRST INIT!\n");
		HSMSetSemphre();
		HardwareSetSemphre();
	}
	else
	{
		printf("SEMPHORE HAVE BEEN INITED!\n");
	}

	// /*READ THE SEMPHRE 10000 TIMES .IF NOT HAVE 1. IT'S NO INIT.*/
	// for(time =0;time < 50000;time++)
	// {
	// 	ret = HSMGetSem();
	// 	HSMUsDelay(10);
	// 	if(ret == 1)
	// 	{
	// 		printf("time is %4d,get 1.\n",time);
	// 		sem_status = 1;
	// 		break;
	// 	}
	// }
	// if(sem_status != 1)
	// {
	// 	/*SET*/
	// 	printf("the sem_status == 0,set semphore\n");
	// 	HSMSetSemphre();
	// }
	// else
	// {
	// 	printf("catch the semphre is 1!\n it has been init\n");
	// }

	//ExportGpioAndInit();
    return 0;
}

/*close spi and release the io */
unsigned int  HSMHardwareDeinit(void)
{
	if(fd > 0)
	{
		printf("close spi fd: %d\n",fd);
		close(fd);
	}
	//UnexportGpioAndInit();
    return 0;
}




/****************************************************************\
* Function:	    HSMWrite
*
* Description:  Master soc send date.
*
* Calls:
*				None
*
* Called By:
*
* Input:       	tx		send buff			   
*				tx_len  length of send  data
* Output:	   	int
*
*
* Return:
*			
*
* Others:
*				None
*
* Remark:	    
\****************************************************************/
unsigned int HSMWrite(unsigned char * tx,unsigned int tx_len)
{
    int ret;
	int tmp_len;
	if(BYTE_TR_MODE==bits)
	{
	    struct spi_ioc_transfer tr =
	    {
	        .tx_buf = (unsigned long)tx,
	        .len = tx_len,
	        .delay_usecs = delay,
	        .speed_hz = speed,
	        .bits_per_word = bits,
	    };

	    if (mode & SPI_TX_QUAD)
	        tr.tx_nbits = 4;
	    else if (mode & SPI_TX_DUAL)
	        tr.tx_nbits = 2;
	    if (mode & SPI_RX_QUAD)
	        tr.rx_nbits = 4;
	    else if (mode & SPI_RX_DUAL)
	        tr.rx_nbits = 2;
	    if (!(mode & SPI_LOOP))
	    {
	        if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
	            tr.rx_buf = 0;
	        else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
	            tr.tx_buf = 0;
	    }

	    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	    if (ret < 1)
	    {
	        printf("can't send spi message");
	        return ret;
	    }
		#if(HSM_HARDWARE_DEBUG==1)
		hex_dump(tx, tx_len, 32, "the send message is:");
		#endif

	    return 0;
	}
	// else if (WORD_TR_MODE==bits)
	// {
	// 	unsigned int * word_tx;
	// 	/*wrod send mode. */
	// 	tmp_len = (tx_len+3)&(0XFFFFFFFC);
	
	// 	word_tx = (unsigned int *)malloc(tmp_len);
	// 	if(word_tx == NULL)
	// 	{
	// 		printf("malloc word_tx failed!\n");
	// 		free(word_tx);
	// 		return 1;

	// 	}

	// 	byteToWord(word_tx,tx,tx_len);
	// 	hex_dump(word_tx,tx_len, 16,"word_tx:");
		
	// 	struct spi_ioc_transfer tr =
	//     {
	//         .tx_buf = (unsigned long)word_tx,
	//         .len = tmp_len,
	//         .delay_usecs = delay,
	//         .speed_hz = speed,
	//         .bits_per_word = bits,
	//     };
			
	// 	if (mode & SPI_TX_QUAD)
	// 		tr.tx_nbits = 4;
	// 	else if (mode & SPI_TX_DUAL)
	// 		tr.tx_nbits = 2;
	// 	if (mode & SPI_RX_QUAD)
	// 		tr.rx_nbits = 4;
	// 	else if (mode & SPI_RX_DUAL)
	// 		tr.rx_nbits = 2;
	// 	if (!(mode & SPI_LOOP))
	// 	{
	// 		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
	// 			tr.rx_buf = 0;
	// 		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
	// 			tr.tx_buf = 0;
	// 	}

	// 	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	// 	if (ret < 1)
	// 	{
	// 		printf("can't tx spi message\n");
	// 		free(word_tx);
	// 		return ret;
	// 	}
		
	// 	hex_dump(tx, tx_len, 16, "the tx message is:");
	// 	free(word_tx);
	// 	return 0;
	// }
	else
	{
		printf("can't support bits.!\n");
		return 1;
	}  
}


/****************************************************************\
* Function:	    HSMRead
*
* Description:  Master soc read return value.
*
* Calls:
*				None
*
* Called By:
*
* Input:       	rx		receive buff			   
*				tx_len  length of receive data
* Output:	   	int
*
*
* Return:
*			
*
* Others:
*				None
*
* Remark:	    
\****************************************************************/
unsigned int HSMRead(unsigned char * rx,unsigned int rx_len)
{
    int ret;
	int tmp_len;
	
	if(BYTE_TR_MODE==bits)
	{
		struct spi_ioc_transfer tr =
		{
			.rx_buf = (unsigned long)rx,
			.len = rx_len,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		};
		if (mode & SPI_TX_QUAD)
			tr.tx_nbits = 4;
		else if (mode & SPI_TX_DUAL)
			tr.tx_nbits = 2;
		if (mode & SPI_RX_QUAD)
			tr.rx_nbits = 4;
		else if (mode & SPI_RX_DUAL)
			tr.rx_nbits = 2;
		if (!(mode & SPI_LOOP))
		{
			if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
			else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
		}

		ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
		if (ret < 1)
		{
			printf("can't rec spi message");
			return ret;
		}
		#if(HSM_HARDWARE_DEBUG==1)
		hex_dump(rx, rx_len, 16, "the rec message is:");
		#endif

		return 0;
	
	}
	// else if (WORD_TR_MODE==bits)
	// {
	// 	unsigned int * word_rx;
	// 	tmp_len = (rx_len+3)&(0XFFFFFFFC);
	// 	printf("tmp_rec_len is %d\n",tmp_len);
	// 	/*申请对齐字节长度的数据空间*/
	// 	word_rx = (unsigned int *)malloc(tmp_len);
	// 	if(word_rx == NULL)
		
	// 	{
	// 		free(word_rx);
	// 		printf("malloc failed!\n");
	// 	}
	// 	struct spi_ioc_transfer tr =
	// 	{
	// 		.rx_buf = (unsigned long)word_rx,
	// 		.len = tmp_len,
	// 		.delay_usecs = delay,
	// 		.speed_hz = speed,
	// 		.bits_per_word = bits,
	// 	};
	// 	if (mode & SPI_TX_QUAD)
	// 	tr.tx_nbits = 4;
	// 	else if (mode & SPI_TX_DUAL)
	// 	tr.tx_nbits = 2;
	// 	if (mode & SPI_RX_QUAD)
	// 	tr.rx_nbits = 4;
	// 	else if (mode & SPI_RX_DUAL)
	// 	tr.rx_nbits = 2;
	// 	if (!(mode & SPI_LOOP))
	// 	{
	// 	if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
	// 	tr.rx_buf = 0;
	// 	else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
	// 	tr.tx_buf = 0;
	// 	}

	// 	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	// 	if (ret < 1)
	// 	{
	// 		printf("can't rec spi message\n");
	// 		free(word_rx);
	// 		return ret;
	// 	}
	// 	wordToByte(rx,word_rx,rx_len);
	// 	free(word_rx);
	// 	hex_dump(rx, rx_len, 16, "the rec message is:");

	// 	return 0;
	// }
	else
	{
		printf("can't support bits.!\n");
		return 1;
	}  
}

/****************************************************************\
* Function:	    HSMGetBusystatus
*
* Description:  Get the HSM module state.
*
* Calls:
*				None
*
* Called By:
*
* Input:       	
* Output:	   	int
*
*
* Return:
*			
*
* Others:
*				None
*
* Remark:	    0   free 
*               1   busy
\****************************************************************/
unsigned int HSMGetBusystatus(void)
{

	usleep(100);
	//return  gpio_read(&busyPin);
	return 0;
}


/*reset HSM module operation*/
unsigned int  HSMReset(void)
{
	gpio_write(&resetPin,0);
	usleep(10000);
	gpio_write(&resetPin,1);
	usleep(10000);
    return 0;
}

/*us delay
hsm_logic_level.c  invoking*/
int HSMUsDelay(uint32_t us_delay)
{
    usleep(us_delay);
}

/*us delay
hsm_logic_level.c  invoking*/
int HSMMsDelay(uint32_t ms_delay)
{
    usleep(ms_delay*1000);
}