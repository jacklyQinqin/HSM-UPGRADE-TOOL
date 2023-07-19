#include "hsm_upgrade_test.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/time.h>
#include "hsm_hardware_level.h"
#include "hsm_test_task.h"
#include "script_deal.h"
#include "hsm_logic_level.h"
#define STEP1_CHECK_VERISON  				0X01
#define STEP1_CHECK_STATUS					STEP1_CHECK_VERISON
#define STEP2_DOWNLOAD_BOOTLOADER  			0X02
#define STEP3_ERASE_HSM_FW  				0X03
#define STEP4_DOWNLOAD_FW  					0X04

#define UPGRADE_SUCCESS						0X00
#define DOWNLOAD_BOOTLOADER_FAILED      	0X01
#define ERASE_FIRMWARE_FAILED        		0X02
#define DOWNLOAD_FIRMWARE_FAILED     		0X03

/*Print log with English*/
static void print_upgrade_message(void)
{
	printf("This is a tool for upgrade hdxa sm2 module\n");
	printf("Please Input below paremeters.\n");
	printf("paremeter0:.exe_name\n");
	printf("paremeter1:.device name\n");
	printf("paremeter2:.gpio_reset number. If you don't have reset Pin, Please user power on again instand of HSMReset()\n");
	printf("paremeter3:.gpio_busy  number\n");
	printf("paremeter4:.upgrade file name (xxxx.ini)\n");
	printf("paremeter5:.SPI-Frequency\n");
	printf("LINK THIS(MY ENVIRONMENT):\n");
	printf("./HSM_UPGRADE-TOOL_test  /dev/spidev32766.0  98 96 DwonloadFile18907.ini  1000000");
	printf("NOTE:\n");
	printf(".I recommend the frequency is 1M~4M,Not too slow,not too fast.\n");	
	printf("VERSION : 0.0.8\n");
	printf("TIME:  2023-7-17\n");
}

extern char SPI_DEV_NAME[100];
extern int busy;
extern int reset;

/*
TIME:2023-7-15
Add new feature.
I will add new feature for  keep sm2 keypair . So the logic is different  from  the old verison.
升级流程:
1.同步失败。说明不是loader和FW.执行2
如果同步成功,检测是FW还是BOOTLOADER.如果是FW.执行3.如果是LOADER
2.下载LOADER
3.擦除FW.
4.下载FW.
由于整个升级过程不可打断。因此将线程锁和进程的位置进行调整。
--------------------------------------------------------------------------------------------------
Upgrade process:
flow:
If the synchronization failed.Which means not have  BOOTLOADER and FIRMWARE. Execute DOWNLOAD BOOTLOADER FLOW.
	If the synchronization is successful, check whether it is FIRMWARE or BOOTLOADER. If it is FIMEWARE. Execute ERASE FRIEWARE. and download new FIRMWARE. 
	If it is BOOTLOADER , DOWNLOAD FIREWARE.

#define STEP1_CHECK_VERISON  				0X01
#define STEP1_CHECK_STATUS					STEP1_CHECK_VERISON
#define STEP2_DOWNLOAD_BOOTLOADER  			0X02
#define STEP3_ERASE_HSM_FW  				0X03
#define STEP4_DOWNLOAD_FW  					0X04

and the bootloader verison 1s 1.0.3 + 'spiloader'
so ,if get the verison is 1.x.x + 'spiloader'.the HSM have a bootloader.
In my test, I get verison 
01 00 03 73 70 69 6C 6F 61 64 65 72
hex(0X73 0X70 0X69 0X6C 0X6F 0X61 0X64 0X65 0X72)---->spiloader.

Since the entire upgrade process cannot be interrupted. Therefore, adjust the position of the thread lock and the process.
*/
unsigned long HSMUpgradeTest(int argc, char *argv[])
{
	int i;
    int  ret;
    int time;
    int sem_status;
    int step = 0;
	unsigned int spi_frequency;
    busy = atoi(argv[2]);
    reset = atoi(argv[3]);
    spi_frequency = atoi(argv[5]);
	char default_pin[38] = {0XBF, 0X0C, 0X0E, 0X00, 0x00, 0X00,1,2,3,4,5,6,7,8};
	char version[16];
	char temp[128];
    /*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	
	printf("your spi_frequency is %4d\n",spi_frequency);
    if((spi_frequency >= 20000000) ||(spi_frequency <= 500000))
    {
        printf("Please input right spi_frequency!\n");
        return  5;
    }
	print_upgrade_message();
	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
	step = STEP1_CHECK_STATUS;

	/*ALL OF THE UPGRADE FLOW,CAN'T BE BREAK */
	if(step == STEP1_CHECK_STATUS)
	{	
		/*同步成功-说明是BOOTLOADER或者是FW.同步失败-跳转至下载bootloader*/
		printf("CURRENT STEP :%4d ,STEP1_CHECK_STATUS\n",step);
		/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
		if (ret)
		{
			printf("THE SYNCHRONISM FAILED,MAYBE THE HSM DON'T HAVE BOOTLOADER AND FIRMWARE. WILL EXECUTE STEP3-DOWNLOAD BOOTLOADER\n");
			step = STEP2_DOWNLOAD_BOOTLOADER;
		}
		else
		{
			printf("sync success ,check current is fw status. or bootloader status\n");
			/*demo read verison.
			the verison is  1.8.7  .in this time(2022/6/16)*/
			ret = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
			printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
			/*if read verison have verison + "spiloader" . it's bootloader.else it's firmware*/
			if(memcmp(&version[3],"spiloader",9))
			{
				printf("COMPARE FIALED.THIS IS FRIMWARE.NEXT STEP ERASE FIRMWARE\n");
				step = STEP3_ERASE_HSM_FW;
			}
			else
			{
				printf("COMPARE SUCCESS.YOU HAVE A BOOTLOADER ,JUST NEED DOWNLOAD FIRMWARE\n");
				step = STEP4_DOWNLOAD_FW;
			}
		}
	}

	/*锁定整个擦除和下载的流程*/
	HSMSetPMutexAndSemphre();

	if(step == STEP2_DOWNLOAD_BOOTLOADER)
	{

		/*This flow will sync commucaiton.-->*/
		for(i=0;i<10;i++)
		{	
			ret = ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(temp,16);
			HSMMsDelay(20);
			if((temp[0] == 0X63) && (temp[1] == 0X62) && (temp[2] == 0X63) && (temp[3] == 0X65))
			{
				break;
			}
		}
		if(i == 10)
		{
			printf("Please reset the hsm and try again.!\n");
			return DOWNLOAD_BOOTLOADER_FAILED;
		}
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(temp,16);
			HSMMsDelay(20);
		/*This flow will sync commucaiton.<--*/


		printf("CURRENT STEP :%4d\n",step);
		HSMMsDelay(100);
		ret = script_analysis("HSM_BOOTLOADER.ini",1);	
		if(ret)
		{
			printf("TRY TO DOWNLOAD NEW HSM APP FAILED ,PLEASE CHECK THE COMMUCAITON,THEN WILL RETURN\n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return DOWNLOAD_BOOTLOADER_FAILED;
		}
		else{
			printf("DOWNLOAD NEW HSM APP SUCCESS!\n");
			printf("THE NEXT STEP WILL CHECK NEW VERISON\n");
			step = STEP4_DOWNLOAD_FW;
		}
	}	
		/*ERASE CURRENT HSM APP  */
	if(step == STEP3_ERASE_HSM_FW)
	{
		printf("CURRENT STEP :%4d STEP2_ERASE_HSM_FW\n",step);
		/*pin confirm*/
		printf("pin is 8 byte password.!\n");
		hex_dump(default_pin+6,8,8,"default_pin");
		/*默认是8字节的密码12345678*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SendOneMessageOneShot(default_pin, 14);
		HSMMsDelay(100);
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(temp,16);
		if (ret==0 && temp[0]==0x90 && temp[1] == 0x00)
		{
			printf("pin confirm success!\n");
		}
		else 
		{
			printf("pin confirm failed!\n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return ERASE_FIRMWARE_FAILED;	
		}

		/*ERASE FIRMWARE*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPErase();
		if(ret)
		{
			printf("TRY TO ERASE HSM APP FAILED.PLEASE CHECK THE COMMUNACATION OR PIN \n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return ERASE_FIRMWARE_FAILED;
		}
		else
		{
			printf("ERASE HSM APP SUCCESS.\n");
			printf("THE NEXT STEP WILL DOWNLOAD NEW HSM FIRMWARE OR BOOTLOADER\n");
			for(i=0;i<10;i++)
			{	
				printf("I IS %4d\n",i);	
				ret = ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(temp,16);
				HSMMsDelay(20);
				hex_dump(temp,16,16,"temp:");
				if((temp[0] == 0X63) && (temp[1] == 0X62) && (temp[2] == 0X63) && (temp[3] == 0X65))
				{
					ret = ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(temp,16);
					HSMMsDelay(20);
					printf("CURRENT STEP2_DOWNLOAD_BOOTLOADER.\n");
					step = STEP2_DOWNLOAD_BOOTLOADER;
					break;
				}
				else if((temp[0]==0X6E) && (temp[1] == 0X00))
				{
					printf("CURRENT STEP4_DOWNLOAD_FW.\n");
					step = STEP4_DOWNLOAD_FW;
					break;
				}
			}
		}
	}


	if(step == STEP2_DOWNLOAD_BOOTLOADER)
	{
		printf("CURRENT STEP :%4d\n",step);
		HSMMsDelay(100);
		ret = script_analysis("HSM_BOOTLOADER.ini",1);	
		if(ret)
		{
			printf("TRY TO DOWNLOAD NEW HSM APP FAILED ,PLEASE CHECK THE COMMUCAITON,THEN WILL RETURN\n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return DOWNLOAD_BOOTLOADER_FAILED;
		}
		else{
			printf("DOWNLOAD NEW HSM APP SUCCESS!\n");
			printf("THE NEXT STEP WILL CHECK NEW VERISON\n");
			step = STEP4_DOWNLOAD_FW;
		}
	}	

	if(step == STEP4_DOWNLOAD_FW)
	{
		printf("CURRENT STEP :%4d  STEP4_DOWNLOAD_FW\n",step);
		HSMMsDelay(100);
		ret = script_analysis_for_bootloader(argv[4],1);
		HSMMsDelay(100);
		if(ret)
		{
			printf("TRY TO DOWNLOAD NEW HSM APP FAILED ,PLEASE CHECK THE COMMUCAITON,THEN WILL RETURN\n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return DOWNLOAD_FIRMWARE_FAILED;
		}
		else{
			printf("DOWNLOAD NEW HSM APP SUCCESS!\n");
			printf("THE NEXT STEP WILL CHECK NEW VERISON\n");
			HSMClearPMutexAndSemphre();
			HSMHardwareDeinit();
			return UPGRADE_SUCCESS;
		}
	}
}
