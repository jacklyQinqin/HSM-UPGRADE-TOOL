/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
/**
 ******************************************************************************
 * File name: hsm_logic_level.c
 * Description:
 * Auth : qin xiaodong
 * Email: qinxd@istecc.com
 * API VERSION: 0.3
 * Time : 2022-7-22
 ******************************************************************************
 * NOTE: 	  SS      SPI-ENABLE
 *           MOSI
 *           MISO
 *           CLK
 *           BUSY   the state of 512A. 1(HIGH LEVEL)  busy   0(LOW LEVEL) free
 *           RESET  low level will  reset the 512A.
 *           After powering on, please lower the RESET for 100ms to ensure that all chips are working properly
 *           if you want to update.Please makse sure the clk is less of 1mbps.(but now you dont'need it.Please
 *            don't try AppErase())
 *	HISTORY:  Modify the 		 hsm_write(fd,tx_buff,tx_buff_len); function.
 *           Modify the 		 hsm_read(fd,tx_buff,tx_buff_len); function.
 *           HSMWrite(tx_buff,tx_buff_len);
 *           HSMRead (tx_buff,tx_buff_len);
 *           Because descriptor (fd) is unique to Linux systems. It affects the portability of the program.
 *
 ******************************************************************************
 */

// hardware of the spi - reset control and busy
#include "hsm_hardware_level.h"
#include "hsm_logic_level.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define DELAY_OF_IMPORT_PUB_KEY             (1)
#define DELAY_OF_IMPORT_PRI_KEY             (20)
#define DELAY_OF_EXPORT_PUB_KEY             (1)
#define DELAY_OF_EXPORT_PRI_KEY             (1)
#define DELAY_OF_READ_VERISON               (1)
#define DELAY_OF_READ_FAC                   (1)
#define DELAY_OF_SYNC                       (1)
#define DELAY_OF_GEN_KEY_PAIR               (50)
#define DELAY_OF_SET_ID                     (10)
#define DELAY_OF_SM2_READ_RESLUT            (1)
#define DELAY_OF_SM2_ENCRYPT                (10)
#define DELAY_OF_SM2_DECRYPT                (8)
#define DELAY_OF_SM2_E_SIGN                 (8)  
#define DELAY_OF_SM2_E_VERIFY               (10) 
#define DELAY_OF_SM2_M_SIGN                 (8)  
#define DELAY_OF_SM2_M_VERIFY               (10)  
#define DELAY_OF_SM2_DECOMPRESS             (8)
#define PIN_CONFIRM                         (1)
#define PIN_CANCEL                          (1)
#define PIN_CHANGE                          (10)
#define DELAY_OF_SM4_IMPROT_KEY      	    (8) 
#define DELAY_OF_SM4_ENCRYPT_DECRYPT        (10) 
#define DELAY_OF_RESTORE_ALL_KEYPAIR        (800)
#define DELAY_OF_MOD_ADD                    (2)
#define DELAY_OF_GET_Bij                    (5) 
#define DELAY_OF_GET_Qij                    (5) 
#define DELAY_OF_GET_RANDOM                 (1)


/*创建一个全局的锁*/
#include <pthread.h>
#include <semaphore.h>

/*线程锁*/
static pthread_mutex_t hsm_mutex_pthread;
/*init pthread mutext*/
int  HSMThreadMutexInit(void)
{
    int ret;
    ret = pthread_mutex_init(&hsm_mutex_pthread,NULL);
    return ret;
}
/**/
void HSMThreadMutexDeinit(void)
{
    pthread_mutex_destroy(&hsm_mutex_pthread);
}


/*Creat a semphore.*/
#include <sys/sem.h>
//#include <semun.h>
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short * array;
    struct seminfo *__buf;
    /* data */
};


static int hsm_semphore_id;
int HSMSetSemphre(void);
static void HSMDeleteSemphre(void);
static int HSMPostSemphre(void);
int HSMVSemphre(void);
int HSMSempohreInit(void);



int HSMSetSemphre(void)
{
    union semun sem_union;
    sem_union.val = 1;

    if(semctl(hsm_semphore_id,0,SETVAL,sem_union) == -1)
        return 0;
    return 1;
    
}
void HSMDeleteSemphre(void)
{
    union semun sem_union;
    sem_union.val = 1;

    if(semctl(hsm_semphore_id,0,IPC_RMID,sem_union) == -1)
       fprintf(stderr,"Failed to delete semphore.\n");
}
/*信号量-1操作*/
int HSMPSemphre(void)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(hsm_semphore_id,&sem_b,1)  == -1)
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
int HSMVSemphre(void)
{
    
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(hsm_semphore_id,&sem_b,1)  == -1)
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
int HSMSempohreInit(void)
{
    hsm_semphore_id = semget((key_t)1234,1,0666|IPC_CREAT);
    
    printf("get sem id %4d\n",hsm_semphore_id);
    return hsm_semphore_id;
}

int HSMSempohreDeInit(void)
{
    HSMDeleteSemphre();
    return 0;
}


int HSMGetSem(void)
{
    union semun sem_union;
    sem_union.val = -2;

    sem_union.val = semctl(hsm_semphore_id,0,GETVAL,sem_union) ;
    //printf("the value is %4d\n",sem_union.val);    
    return  sem_union.val;
}

/*这里是为了在其他的地方可以单独的锁定SPI的硬件*/
int HSMSetPMutexAndSemphre(void)
{
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
}

int HSMClearPMutexAndSemphre(void)
{
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
}
/*print log yes or no*/
#define HSM_LOGIC_LINIX_DEBUG_ON 0
/*send and receive buff*/
static unsigned char tx_buff[2064] = {0};
static unsigned char rx_buff[2064] = {0};

unsigned long  V2XDeviceGetRandom(unsigned char * buff, unsigned long len);
unsigned long  V2XDeviceGetSM4Key(unsigned char * buff);

/****************************************************************\
* Function:			CosReadVersion
*
* Description: 		read HSM module verison.
*
* Calls:
*					None
*
* Called By:
*
* Input:
*
* Output:
*					version
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:	     This time the verison is 1.8.7 . 2022-6-16
\****************************************************************/
unsigned long CosReadVersion(unsigned char *version)
{
    unsigned char tx_buff[2064] = {0};
    unsigned char rx_buff[2064] = {0};
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 16;
    /*copy the command to send buff*/
    const uint8_t read_version[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x10, 0x06, 0x00, 0x0, 0x0};
    
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, read_version, IS32U512A_SM2_MODULE_CMD_LEN);
    /*cal the total length*/
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    /*get module status.if the io is 1. Means busy
    the io is 0. means free*/
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
         HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_READ_VERISON);
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(version, rx_buff + 2, rx_buff_len-2);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "CosReadVersion rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			GenKeyPair
*
* Description: 		Generate a set of SM2 key pairs.
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					Index:0-5
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:	     Generate a set of SM2 key pairs.
*The private key is 32 bytes and the public key is 64 bytes.
*Once generated, it is retained in the RAM space of the HSM module.
*You can use it based on the index value.
\****************************************************************/
unsigned long GenKeyPair(unsigned long index)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 4;
    const unsigned char gen_key_pair[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF, 0X05, 0X06, 0X00, 0X00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, gen_key_pair, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_GEN_KEY_PAIR);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }
    else if (rx_buff[0] == 0x69 && rx_buff[1] == 0x85)/*GerKeypair fail*/
    {
        #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(rx_buff, rx_buff_len, 16, "GenKeyPair rx:");
        #endif
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    else/*Gen KeyPair not done .waitting a fix time ,and read result again */
    {
        HSMMsDelay(DELAY_OF_GEN_KEY_PAIR);
        while (HSMGetBusystatus())
            ;
        ret = HSMRead(rx_buff, rx_buff_len);
        if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
        {
            HSMVSemphre();
            pthread_mutex_unlock(&hsm_mutex_pthread);
            return sucess;
        }

        #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(rx_buff, rx_buff_len, 16, "GenKeyPair rx:");
        #endif
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;   
    }
 
}

/****************************************************************\
* Function:			SM2SetID
*
* Description: 		set id parameter
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					Index： id_index.
* 				    sm2_id  ：id value.
*                   length  : length of id
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark: the format of package is : CMD + len +ID
* Id is the parameter when SM2 hashes using SM3.
\****************************************************************/
unsigned long SM2SetID(unsigned long index, unsigned char *sm2_id, unsigned char length)
{

    unsigned long ret = 0;
    unsigned long  tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + length + 1;
    unsigned long rx_buff_len = 4;
    const uint8_t is32u512a_set_id[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x14, 0x00, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, is32u512a_set_id, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_LEN] = length;
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN + 1], sm2_id, length);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    // HSMMsDelay(1);
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
         HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(DELAY_OF_SET_ID);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2SetID rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}
/****************************************************************\
* Function:			ImportSM2Pubkey
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*				Index:    : index of key_pair
* 				pubkey_x  ：Gx
*         		pubkey_y  : Gy
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:    Import the public key into the HSM module.
*
\****************************************************************/
unsigned long ImportSM2Pubkey(unsigned long index, unsigned char *pubkey_x, unsigned char *pubkey_y)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + IS32U512A_SM2_PUBKEY_LEN;
    unsigned long rx_buff_len = 4;
    const uint8_t import_sm2pubkey_cmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x01, 0x46, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, import_sm2pubkey_cmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], pubkey_x, 32);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN + 32], pubkey_y, 32);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
         HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_IMPORT_PUB_KEY);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "ImportSM2Pubkey rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}
/****************************************************************\
* Function:	      ImportSM2Prikey
*
* Description:
*
* Calls:
*				  None
*
* Called By:
*
* Input:
*				Index:    : index of key_pair
* 				prikey_d  ：sm2 private key
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:  Import the private key into the HSM module.
*
\****************************************************************/
unsigned long ImportSM2Prikey(unsigned long index, unsigned char *prikey_d)
{

    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + IS32U512A_SM2_PRIKEY_LEN;
    unsigned long rx_buff_len = 4;
    uint8_t import_sm2prikey_cmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x02, 0x00, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, import_sm2prikey_cmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], prikey_d, 32);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {

        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_IMPORT_PRI_KEY);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "ImportSM2Prikey rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			ExportSM2Pubkey
*
* Description: 		get sm2 public key
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					Index:    : index of key_pair
* 				    pubkey_x  ：Gx
*                   pubkey_y  ：Gy
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:  If you call the key generation function, use this function to get a SM2 algorithm public key（Gx,Gy）
*
\****************************************************************/
unsigned long ExportSM2Pubkey(unsigned long index, unsigned char *pubkey_x, unsigned char *pubkey_y)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 2 + IS32U512A_SM2_PUBKEY_LEN;
    const uint8_t export_sm2pubkey_cmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x06, 0x06, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, export_sm2pubkey_cmd, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);

    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_EXPORT_PUB_KEY);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(pubkey_x, &rx_buff[2], 32);
        memcpy(pubkey_y, &rx_buff[34], 32);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "ExportSM2Pubkey rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}
/****************************************************************\
* Function:			ExportSM2Prikey
*
* Description: 		Get sm2 private key
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					Index:    : index of key_pair
* 				    prikey_d  ：32byte private key
* Output:
*					None
*
* Return:
*				    SUCCESS：       0X00
*				    FAIL   ：       0X01
*
* Others:
*				    None
*
* Remark:  If you call the key generation function, use this function to get a SM2 algorithm private key

* Modify History:
* 1.8.9.9 delete export pri key api.
*
\****************************************************************/
unsigned long ExportSM2Prikey(unsigned long index, unsigned char *prikey_d)
{
    /*NOT USED*/
     return fail;
}

/****************************************************************\
* Function:			SM2SingleVerify
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					PubkeyIndex:   : index of key_pair
*                   p_org_data     ：message
*					org_len		   ：the length of message.
*                   p_sign_result  : The signature result of SM2 algorithm
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:          The purpose of this program is to verify that the digital signature of the message is correct.
*
\****************************************************************/
unsigned long SM2SingleVerify(unsigned long pubkey_index, unsigned char *p_org_data, unsigned long org_len, unsigned char *p_sign_result)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + org_len + SM2_R_LEN + SM2_S_LEN;
    unsigned long rx_buff_len = 4;
    unsigned char SM2VerifyCmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x04, 0x00, 0x00, 0x00, 0x00};
     pthread_mutex_lock(&hsm_mutex_pthread);
     HSMPSemphre();
    memcpy(tx_buff, SM2VerifyCmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], p_org_data, org_len);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN + org_len], p_sign_result, SM2_R_LEN + SM2_S_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = pubkey_index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    if(org_len == 0)
    {
        printf("len of org errror!\n");
    }
    HSMUsDelay(VERIFY_DELAY);
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);

    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    
    ret  = (org_len + 15)/16;
    HSMMsDelay(DELAY_OF_SM2_M_VERIFY*ret);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2SingleVerify rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SM2Sign
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					index       : index of key_pair
*         			p_org_data  : message
*					org_len		: length of message
* Output:
*					p_sign_data  : Digital signature result
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:        The signature result of SM2 algorithm is 64 bytes.
* It is divided into R 32 bytes and S 32 bytes
*
\****************************************************************/
unsigned long SM2Sign(unsigned long prikey_index, unsigned char *p_org_data, unsigned long org_len, unsigned char *p_sign_data)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + org_len;
    unsigned long rx_buff_len = 2 + SM2_R_LEN + SM2_S_LEN;

    unsigned char sm2_sign_cmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x03, 0x00, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, sm2_sign_cmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], p_org_data, org_len);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = prikey_index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);

    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    
    ret = (org_len+15) /16;
    HSMMsDelay(DELAY_OF_SM2_M_SIGN * ret);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(p_sign_data, &rx_buff[2], 64);
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2Sign rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SM2SignEValue
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					index       : index of key_pair
*         			e           : e value .(32bytes)
* Output:
*					p_sign_data  : Digital signature result
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:        The signature result of SM2 algorithm is 64 bytes.
* It is divided into R 32 bytes and S 32 bytes
*
\****************************************************************/
unsigned long SM2SignEValue(unsigned long prikey_index, unsigned char *e,unsigned char *p_sign_data)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + 32;
    unsigned long rx_buff_len = 2 + SM2_R_LEN + SM2_S_LEN;

    unsigned char sm2_sign_cmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x23, 0x00, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, sm2_sign_cmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], e, 32);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = prikey_index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);

    if (0 != ret)
    {

        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_SM2_E_SIGN);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {    
        memcpy(p_sign_data, &rx_buff[2], SM2_R_LEN + SM2_S_LEN);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2SignEValue rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			PinConfirm
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*				PIN:
 *				LEN:
* Output:
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:       After the PIN is authenticated, you can perform operations requiring permissions
*
\****************************************************************/
unsigned long PinConfirm(unsigned char *pin_value, unsigned long len_of_pin)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len_of_pin;
    unsigned long rx_buff_len = 4;

    unsigned char pin_verify[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF, 0X0C, 0X0E, 0X00, 0x00, 0X00};
    unsigned char pin_cancel_verify[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF, 0X0D, 0X0E, 0X00, 0X00, 0x00};
     pthread_mutex_lock(&hsm_mutex_pthread);
     HSMPSemphre();
    memcpy(tx_buff, pin_verify, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], pin_value, len_of_pin);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = 0;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(PIN_CONFIRM);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "PinConfirm rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			PinConfirmCancel
*
* Description: 		cancel the pin certification
*
* Calls:
*					None
*
* Called By:
*
* Input:
*				PIN:
 *				LEN:
* Output:
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
*
\****************************************************************/
unsigned long PinConfirmCancel(unsigned char *pin_value, unsigned long len_of_pin)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len_of_pin;
    unsigned long rx_buff_len = 4;

    unsigned char pin_cancel_verify[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF, 0X0D, 0X0E, 0X00, 0X00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, pin_cancel_verify, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], pin_value, len_of_pin);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = 0;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(PIN_CANCEL);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "PinConfirmCancel rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}
/****************************************************************\
* Function:			PinChange
*
* Description: 		change the PIN .
*
* Calls:
*					None
*
* Called By:
*
* Input:
*				old_pin_value:lod pin password
*				new_pin_value:lod pin password
*               len_of_pin: length of new pin value
* Output:
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
*
\****************************************************************/
unsigned long PinChange(unsigned char *old_pin_value, unsigned char *new_pin_value, unsigned long len_of_pin)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len_of_pin + len_of_pin;
    unsigned long rx_buff_len = 4;

    unsigned char pin_change[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF, 0X0E, 0X0E, 0X00, 0X00, 0x00};
     pthread_mutex_lock(&hsm_mutex_pthread);
     HSMPSemphre();
    memcpy(tx_buff, pin_change, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], old_pin_value, len_of_pin);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN + len_of_pin], new_pin_value, len_of_pin);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = 0;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(PIN_CHANGE);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "PinChange rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SyncStatus
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*
* Output:
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*
* Remark:        HSM module has two states: receiving instruction and sending execution result.
*Every time SPI data transmission is completed, the state will be switched once.
*This program will try to switch the state of the module to receiving state.
\****************************************************************/
unsigned long SyncStatus(void)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 4;
    unsigned char tx_buff[2064] = {0};
    unsigned char rx_buff[2064] = {0};
    const uint8_t is32u512a_module_sync[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0xFA, 0x06, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();

    memcpy(tx_buff, is32u512a_module_sync, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;
    
    /*try to switch the state of the module to receiving state once */
    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(DELAY_OF_SYNC);
    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(1);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    /*try to switch the state of the module to receiving state twice */
    HSMMsDelay(1);
    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(1);
    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(1);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SyncStatus rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:	    SM2Encrypt
*
* Description:
*
* Calls:
*				None
*
* Called By:
*
* Input:
*               index
*               message plaintext.
*
*              	len     length of plaintext.
*				out     ciplertext
* Output:
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:   This program is a combination of encryption and read encryption results of two instructions.
The process is to send encryption instructions and data, read whether the calculation is successful,
send the instruction to read encryption results, read the encrypted data.
\****************************************************************/
unsigned long SM2Encrypt(unsigned long index, unsigned char *message, unsigned long len, unsigned char *out)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len;
    unsigned long rx_buff_len = 4;
    const unsigned char is32u512a_sm2_encrypt[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x08, 0x00, 0x00, 0x00, 0x00};
    const unsigned char is32u512a_sm2_read_encrypt_result[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x09, 0x06, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, is32u512a_sm2_encrypt, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], message, len);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;
    if(len == 0)
    {
        printf("SM2Encrypt len error.!\n");
        return 1;
    }

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    ret = (len + 15) /16;
    HSMMsDelay(DELAY_OF_SM2_ENCRYPT*ret);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        ;
    }
    else
    {
        #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(rx_buff, rx_buff_len, 16, "SM2Encrypt rx:");
        #endif
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    /*the reusult is C1 64BYTE + C3 32byte + C2 len data */
    tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    rx_buff_len = 2 + len + 96;
    memcpy(tx_buff, is32u512a_sm2_read_encrypt_result, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_SM2_READ_RESLUT);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(out, &rx_buff[2], rx_buff_len - 2);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2Encrypt Read Result rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}
/****************************************************************\
* Function:	    SM2Decrypt
*
* Description:
*
* Calls:
*				None
*
* Called By:
*
* Input:        index   : the  index of sm2 key pair
*               message : ciphertext buff
*               len     : length of ciphertext
*               out     : plaintext , result of decode.
*
* Output:
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:    This program is a combination of decryption and read decryption results.
    1>send decryption instruction and ciphertext,read whether the calculation is successful,
    2>send read decryption data instruction, read the decrypted plaintext.
*
\****************************************************************/
unsigned long SM2Decrypt(unsigned long index, unsigned char *message, unsigned long len, unsigned char *out)
{
    unsigned long ret = 0;
    /*length of send data, length of receive data.*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len;
    unsigned long rx_buff_len = 4;

    /*sm2 decrypt Instruction*/
    const uint8_t is32u512a_sm2_decrypt[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x0a, 0x00, 0x00, 0x00, 0x00};
    /*sm2 read decrypt result*/
    const uint8_t is32u512a_sm2_read_decrypt_result[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x0b, 0x06, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*Packaging */
    memcpy(tx_buff, is32u512a_sm2_decrypt, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], message, len);
    /*Computes index and instruction total length*/
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;


    if(len == 0)
    {
        printf("SM2Decrypt len error !\n");
    }
    /*Watting for HSM free.*/
    while (HSMGetBusystatus())
        ;

    /*send instruction and data*/
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    ret = (len+15) /  16;
    HSMMsDelay(DELAY_OF_SM2_DECRYPT*ret);
    /*Watting for HSM free.*/
    while (HSMGetBusystatus())
        ;
    /*receive encrypt result*/
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        ;
    }
    else
    {
        #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(rx_buff, rx_buff_len, 16, "SM2Decrypt rx:");
        #endif    
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    /*send instruction*/
    tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    rx_buff_len = 2 + len - 96;
    memcpy(tx_buff, is32u512a_sm2_read_decrypt_result, IS32U512A_SM2_MODULE_CMD_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_SM2_READ_RESLUT);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(out, &rx_buff[2], rx_buff_len - 2);
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return 0;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2Decrypt Read Result rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			ReadFactoryNumber
*
* Description: 		the UUID of HSM module
*
* Calls:
*					None
*
* Called By:
*
* Input:
*
* Output:
*					fac_num  13byte
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
\****************************************************************/
unsigned long ReadFactoryNumber(unsigned char *fac_num)
{
    /*定义返回值*/
    unsigned long ret = 0;
    /*定义发送和接收数据的长度*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 15;
    /*定义基本指令部分*/
    const uint8_t read_fac_num[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x0f, 0x06, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*将需要发送的数据进行组包*/
    memcpy(tx_buff, read_fac_num, IS32U512A_SM2_MODULE_CMD_LEN);
    /*计算INDEX和数据总长度*/
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_READ_FAC);
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(fac_num, rx_buff + 2, rx_buff_len - 2);
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "ReadFactoryNumber rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SM4ImportKey
*
* Description: 		导入SM4的秘钥
*
* Calls:
*					None
*
* Called By:
*
* Input:            INDEX  秘钥分组--目前只有0
*                   KEY    秘钥指针--SM4秘钥固定16字节
* Output:
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
\****************************************************************/
unsigned long SM4ImportKey(unsigned long index, unsigned char *key)
{
    /*定义返回值*/
    unsigned long  ret = 0;
    /*定义发送和接收数据的长度*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + IS32U512A_SM4_KEY_LEN;
    unsigned long rx_buff_len = 4;
    /*定义基本指令部分*/
    const uint8_t sm4_import_key[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x11, 0x16, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*将需要发送的数据进行组包*/
    memcpy(tx_buff, sm4_import_key, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], key, IS32U512A_SM4_KEY_LEN);
    /*计算INDEX和数据总长度*/
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_SM4_IMPROT_KEY);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM4ImportKey rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SM4Encrpyt
*
* Description: 		SM4加密
*
* Calls:
*					None
*
* Called By:
*
* Input:            INDEX 秘钥分组。目前只支持0.传入其他参数无效
*                   in 输入明文
 *                  len 数据长度
 *                  out 输出密文
* Output:
*					version
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
\****************************************************************/
unsigned long SM4Encrpyt(unsigned long index, unsigned char *in, unsigned long len, unsigned char *out)
{
    /*定义返回值*/
    unsigned long ret = 0;
    /*定义发送和接收数据的长度*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len;
    unsigned long rx_buff_len = 2 + len;
    /*定义基本指令部分*/
    const uint8_t sm4_encrypt[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x12, 0x16, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*将需要发送的数据进行组包*/
    memcpy(tx_buff, sm4_encrypt, IS32U512A_SM2_MODULE_CMD_LEN);
    /*计算INDEX和数据总长度*/
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;
    if((len==0) || (len%16))
    {
        printf("sm4 len error !\n");
        return 1;
    }
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    ret = (len+15) / 16;
    HSMMsDelay(DELAY_OF_SM4_ENCRYPT_DECRYPT*ret);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(out, rx_buff + 2, rx_buff_len - 2);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM4Encrpyt rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:			SM4Decrpyt
*
* Description: 		SM4解密--ECB模式
*
* Calls:
*					None
*
* Called By:
*
* Input:            INDEX 秘钥分组。目前只支持0.传入其他参数无效
*                   in 输入明文
 *                  len 数据长度
 *                  out 输出密文
* Output:
*					version
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:
\****************************************************************/
unsigned long SM4Decrpyt(unsigned long index, unsigned char *in, unsigned long len, unsigned char *out)
{
    /*定义返回值*/
    unsigned long ret = 0;
    /*定义发送和接收数据的长度*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + len;
    unsigned long rx_buff_len = 2 + len;
    /*定义基本指令部分*/
    const uint8_t sm4_decrypt[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x13, 0x16, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*将需要发送的数据进行组包*/
    memcpy(tx_buff, sm4_decrypt, IS32U512A_SM2_MODULE_CMD_LEN);
    /*计算INDEX和数据总长度*/
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;


    if((len==0) || (len%16))
    {
        printf("sm4 len error !\n");
        return 1;
    }

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }


    ret = (len+15) / 16;
    HSMMsDelay(DELAY_OF_SM4_ENCRYPT_DECRYPT*ret);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(out, rx_buff + 2, rx_buff_len - 2);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM4Decrpyt rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/****************************************************************\
* Function:	    SM2PointerDecompress
*
* Description:  pointer decompress.
*
* Calls:
*				None
*
* Called By:
*
* Input:       	gx  part of public key(Gx)
*              	mode mode (2/3)
* Output:	  	pubkey publlic key
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark.
*
\****************************************************************/
unsigned long SM2PointerDecompress(unsigned char * gx, ISTECCPointDecompressMode_t mode,unsigned char *pubkey)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN +  IS32U512A_SM2_GX_LEN; // 1byte is length of mode.
    unsigned long rx_buff_len = 2 + IS32U512A_SM2_PUBKEY_LEN;
    //点解压命令 CMD(5) +MODE(1)+GX(32)共38字节，返回值为9000 + PUBKEY 66字节
    const unsigned char pointDecompressCmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF,0X25,0X00,0X00,0X21,0X00};

    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memset(tx_buff,0x00,100);
    memcpy(tx_buff, pointDecompressCmd, sizeof(pointDecompressCmd));
    tx_buff[IS32U512A_SM2_MODULE_CMD_LEN-1] = mode;
    //copy GX.
	memcpy(tx_buff+IS32U512A_SM2_MODULE_CMD_LEN,gx,IS32U512A_SM2_GX_LEN);
    if((mode != ISTECC_POINT_DECOMPRESS_2) &&( mode != ISTECC_POINT_DECOMPRESS_3))
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return ERROR_DECOMPRESS_MODE_NO_SUPPORT;
    }
    
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
 
        if (0 != ret)
        {
            HSMVSemphre();
             pthread_mutex_unlock(&hsm_mutex_pthread);
            return fail;
        }
        HSMMsDelay(DELAY_OF_SM2_DECOMPRESS);
        while (HSMGetBusystatus())
            ;

    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(pubkey, rx_buff + 2, rx_buff_len - 2);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2PointerDecompress rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;

}


/****************************************************************\
* Function:	   SM2Getbij
*
* Description: sign key derivative .final you got a  Signature extension private key
*
* Calls:
*				None
*
* Called By:
*
* Input:       
*               kS   sm4 key                 
*				i     parameters i .
*              	j     parameters j
*				
* Output:	  	
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:     for example: 
*             
\****************************************************************/
unsigned long SM2Getbij(unsigned char *kS,unsigned long i , unsigned long j)
{

    unsigned long ret = 0;
    unsigned long tx_buff_len = 0X24; // 1byte is length of mode.
    unsigned long rx_buff_len = 2 + 4;

    const unsigned char sign_key_kdf[IS32U512A_SM2_MODULE_CMD_LEN+2] = {0XBF,0X3C,0X00,0X00,0X00,0X00};
	pthread_mutex_lock(&hsm_mutex_pthread);	
    HSMPSemphre();
    memset(tx_buff,0x00,100);
    memcpy(tx_buff, sign_key_kdf, sizeof(sign_key_kdf));

    tx_buff[6] = 0x00;
    tx_buff[7] = 0x24;

    tx_buff[8] = (IS32U512A_CONST_SIGN_FACTOR_INDEX>>8 ) & 0xff;
    tx_buff[9] = IS32U512A_CONST_SIGN_FACTOR_INDEX & 0xff;
    tx_buff[10] = ((IS32U512A_CONST_SIGN_FACTOR_INDEX-2)>>8 ) & 0xff;
    tx_buff[11] = (IS32U512A_CONST_SIGN_FACTOR_INDEX-2) & 0xff;


    tx_buff[12] = (i>>24) & 0xff;
    tx_buff[13] = (i>>16) & 0xff;
    tx_buff[14] = (i>>8 ) & 0xff;
    tx_buff[15] = i & 0xff;


    tx_buff[16] = (j>>24) & 0xff;
    tx_buff[17] = (j>>16) & 0xff;
    tx_buff[18] = (j>>8 ) & 0xff;
    tx_buff[19] = j & 0xff;
		
		
    memcpy(&tx_buff[20],kS,16);
    

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
  
    if (0 != ret)
    {
        HSMVSemphre();
            pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_GET_Bij);
    while (HSMGetBusystatus())
            ;

    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2Getbij rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;

}

/****************************************************************\
* Function:	   SM2Getqij
*
* Description: enctypt kdf private.
*
* Calls:
*				None
*
* Called By:
*
* Input:        
*               kE    sm4 key 
*				i     parameters i .
*              	j     parameters j
*				
* Output:	  	
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:     
*             
\****************************************************************/
unsigned long SM2Getqij(unsigned char *kE,unsigned long i , unsigned long j)
{

    unsigned long ret = 0;
    unsigned long tx_buff_len = 0X24; // 1byte is length of mode.
    unsigned long rx_buff_len = 2 + 4;
    //get qij
    const unsigned char encrypt_key_kdf[IS32U512A_SM2_MODULE_CMD_LEN+2] = {0XBF,0X3D,0X00,0X00,0X00,0X00};
	pthread_mutex_lock(&hsm_mutex_pthread);	
    HSMPSemphre();
    memset(tx_buff,0x00,100);
    memcpy(tx_buff, encrypt_key_kdf, sizeof(encrypt_key_kdf));

    tx_buff[6] = 0x00;
    tx_buff[7] = 0x24;

    tx_buff[8] = (IS32U512A_CONST_DECRYPT_FACTOR_INDEX>>8 ) & 0xff;
    tx_buff[9] = IS32U512A_CONST_DECRYPT_FACTOR_INDEX & 0xff;
    tx_buff[10] = ((IS32U512A_CONST_DECRYPT_FACTOR_INDEX-2)>>8 ) & 0xff;
    tx_buff[11] = (IS32U512A_CONST_DECRYPT_FACTOR_INDEX-2) & 0xff;


    tx_buff[12] = (i>>24) & 0xff;
    tx_buff[13] = (i>>16) & 0xff;
    tx_buff[14] = (i>>8 ) & 0xff;
    tx_buff[15] = i & 0xff;


    tx_buff[16] = (j>>24) & 0xff;
    tx_buff[17] = (j>>16) & 0xff;
    tx_buff[18] = (j>>8 ) & 0xff;
    tx_buff[19] = j & 0xff;
		
		
    memcpy(&tx_buff[20],kE,16);
    

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_GET_Qij);
    while (HSMGetBusystatus())
        ;

    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2Getqij rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;

}


/*mod add
THIS FUNCITON IS  si,j = (bi,j + c) mod l 
in order to facilitate debugging, i set parameter 1 and parameter 2 as exportable.
after debugging, these parameters can not be exprted for safety.juest can be used.

//Modify History:
unsigned long ModAdd(unsigned char *bij,unsigned char *c,unsigned char *out_sij)
2023-6-6
unsigned long ModAdd(unsigned int bij_index,unsigned char *c,unsigned char *out_sij)
*/

unsigned long ModAdd(unsigned int bij_index,unsigned char *c,unsigned char *out_sij)
{

    unsigned long ret = 0;
    unsigned long tx_buff_len = 0x2c; /*CMD_LEN*/
    unsigned long rx_buff_len =34;
    const unsigned char kdf_mod_add[12] = {0XBF,0X3B,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff,kdf_mod_add,12);

    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = bij_index;
    tx_buff[6] = (tx_buff_len)/256;
    tx_buff[7] = (tx_buff_len)%256;
 
    memcpy(&tx_buff[12],c,32);
    while(HSMGetBusystatus());
    ret =  HSMWrite(tx_buff,tx_buff_len);
    if(0!=ret)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(DELAY_OF_MOD_ADD);
    while(HSMGetBusystatus());
    ret = HSMRead(rx_buff,rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(out_sij,rx_buff+2,32);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "ModAdd rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}



/*This fucntion contains two parts:
1. SM2 decode the  .ciphertext_sm4_key( the cKey（v,t,c format)   and get sm4 key 
2. use sm4 ccm decode the data. and get full si,j.
@PRA
unsigned long qij_index    
in the interface . the qij_index is 1
unsigned char * EcencP256EncryptedKey
unsigned char *CTij, unsigned long CTij_len,
unsigned char *nonce,unsigned long nonce_len,
unsigned char * tag, unsigned long tag_len,
unsigned char * palin_text

*/
  unsigned long SM2Getsij(unsigned char * seed_a,
      unsigned char *kS,unsigned int iINT,
      unsigned int jINT,char * c,
      unsigned char * sij)
{
    /*NOT USED*/
    return fail;
}

/****************************************************************\
* Function:			APPErase
*
* Description: 		erase the app.now the moudle does't work.
*
* Calls:
*					None
*
* Called By:
*
* Input:
*
* Output:
*
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:          WARNING:If you're not sure, don't do it.
\****************************************************************/
unsigned long APPErase(void)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 4;

    const uint8_t read_version[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0xfb, 0x06, 0x00, 0x0, 0x0};
    // pthread_mutex_lock(&hsm_mutex_pthread);
    // HSMPSemphre();
    memcpy(tx_buff, read_version, IS32U512A_SM2_MODULE_CMD_LEN);

    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        // HSMVSemphre();
        // pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(20);
    // HSMVSemphre();
    // pthread_mutex_unlock(&hsm_mutex_pthread);
    return 0;
}

/**
 * @brief
 * update need this function .you don't need it now.
 * @param send
 * @param len
 * @return unsigned long
 */
unsigned long IS32U512ASendOneMessage(unsigned char *send, unsigned long len)
{
    unsigned long time;
    unsigned long i = 0;
    unsigned long remainder;
    unsigned long ret;
    time = (len + 15) / 16;
    memcpy(tx_buff, send, time*16);
    for (i = 0; i < time; i++)
    {
        ret = HSMWrite(tx_buff + 16 * i, 16);
#if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(tx_buff + 16 * i, 16, 16, "send one meessage:");
#endif
        if (0 != ret)
        {
            return fail;
        }
        HSMMsDelay(2);
    }
    return sucess;
}

unsigned long IS32U512ASendOneMessageOneShot(unsigned char *send, unsigned long len)
{
    unsigned long time;
    unsigned long i = 0;
    unsigned long remainder;
    unsigned long ret;
    time = (len + 15) / 16;
    memcpy(tx_buff, send, len);
    ret = HSMWrite(tx_buff, len);
#if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
     hex_dump(tx_buff, len, 16, "send one meessage:");
#endif
    return sucess;
}
/**
 * @brief
 *
 * @param rec
 * @param len
 * @return unsigned long
 */
unsigned long IS32U512AReceiveOneMessage(unsigned char *rec, unsigned long len)
{
    unsigned long time;
    unsigned long count;
    unsigned long i = 0;
    unsigned long ret;
    time = (len + 15) / 16;

    for (i = 0; i < time; i++)
    {
        ret = HSMRead(rec + i * 16, 16);
        if (0 != ret)
        {
            return fail;
        }
        HSMMsDelay(1);
    }
    #if(HSM_LOGIC_LINIX_DEBUG_ON==1)
    hex_dump(rec,16,16,"IS32U512AReceiveOneMessage:");
    #endif
    return sucess;
}


unsigned long  V2XDeviceGetRandom(unsigned char * buff,unsigned long len)
{
    unsigned long ret;
    /*定义发送和接收数据的长度*/
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 2 + len;
    /*定义基本指令部分*/
    const unsigned char get_random[IS32U512A_SM2_MODULE_CMD_LEN] = {0XBF,0X40,0X00,0X00,0X00,0X00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    /*计算INDEX和数据总长度*/
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    memset(tx_buff,0x00,100);
    memcpy(tx_buff, get_random, sizeof(get_random));

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_GET_RANDOM);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        memcpy(buff, &rx_buff[2], len);
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }
    
    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "V2XDeviceGetRandom rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

unsigned long  V2XDeviceGetSM4Key(unsigned char * buff)
{
    return V2XDeviceGetRandom(buff,16);
}

/*
* gen kS. kE .(a,A) and (p,P)
*/
unsigned long  V2XDeviceKeyDeriveFlowInit(unsigned char *kS,unsigned char *kE,unsigned char *A, unsigned char *P)
{
    unsigned long ret;

    ret = V2XDeviceGetSM4Key(kS);
    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(kS,16,16,"kS:");
    #endif 
    if(ret)
    {
        return ret;
    }
    ret = V2XDeviceGetSM4Key(kE);
    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(kE,16,16,"ke:");
    #endif 
    if(ret)
    {
        return ret;
    }

    /*Gen sign key pair factor to the fixed index*/
    ret = GenKeyPair(IS32U512A_CONST_SIGN_FACTOR_INDEX);
    if(ret)
    {
        return ret;
    }
    ret = ExportSM2Pubkey(IS32U512A_CONST_SIGN_FACTOR_INDEX,A,A+32);
    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(A,64,16,"A:");
    #endif 
    if(ret)
    {
        return ret;
    }

    ret = GenKeyPair(IS32U512A_CONST_DECRYPT_FACTOR_INDEX);
    if(ret)
    {
        return ret;
    }
    ret = ExportSM2Pubkey(IS32U512A_CONST_DECRYPT_FACTOR_INDEX,P,P+32);
    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(P,64,16,"P:");
    #endif 
    if(ret)
    {
        return ret;
    }
    return ret;
}




/****************************************************************\
* Function:			SM2EValueVerify
*
* Description:
*
* Calls:
*					None
*
* Called By:
*
* Input:
*					PubkeyIndex:   : index of key_pair
*                   e              : e value .(hsah value 32bytes.)
*                   rs             : The signature result of SM2 algorithm (64bytes)
* Output:
*					None
*
* Return:
*				  SUCCESS：       0X00
*				  FAIL   ：       0X01
*
* Others:
*					None
*
* Remark:          The purpose of this program is to verify that the digital signature of the e is correct.
*
\****************************************************************/
unsigned long SM2EValueVerify(unsigned long pubkey_index, unsigned char *e,unsigned char *rs)
{
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN + 32 + SM2_R_LEN + SM2_S_LEN;
    unsigned long rx_buff_len = 4;
    unsigned char SM2EvalueVerifyCmd[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0x28, 0x00, 0x00, 0x00, 0x00};
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, SM2EvalueVerifyCmd, IS32U512A_SM2_MODULE_CMD_LEN);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN], e, 32);
    memcpy(&tx_buff[IS32U512A_SM2_MODULE_CMD_LEN + 32], rs, SM2_R_LEN + SM2_S_LEN);
    tx_buff[IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET] = pubkey_index;
    tx_buff[IS32U512A_SM2_DATA_LEN_H_OFFSET] = tx_buff_len / 256;
    tx_buff[IS32U512A_SM2_DATA_LEN_L_OFFSET] = tx_buff_len % 256;

    HSMUsDelay(VERIFY_DELAY);
    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
        HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }

    HSMMsDelay(DELAY_OF_SM2_E_VERIFY);
    while (HSMGetBusystatus())
        ;
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "SM2EValueVerify rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}


unsigned long HSMRestoreKeys(void)
{
    unsigned char tx_buff[2064] = {0};
    unsigned char rx_buff[2064] = {0};
    unsigned long ret = 0;
    unsigned long tx_buff_len = IS32U512A_SM2_MODULE_CMD_LEN;
    unsigned long rx_buff_len = 6;
    /*copy the command to send buff*/
    const uint8_t hsm_restore[IS32U512A_SM2_MODULE_CMD_LEN] = {0xbf, 0xE1, 0x55, 0xAA, 0xCC, 0x00};
    
    pthread_mutex_lock(&hsm_mutex_pthread);
    HSMPSemphre();
    memcpy(tx_buff, hsm_restore, IS32U512A_SM2_MODULE_CMD_LEN);

    while (HSMGetBusystatus())
        ;
    ret = HSMWrite(tx_buff, tx_buff_len);
    if (0 != ret)
    {
         HSMVSemphre();
         pthread_mutex_unlock(&hsm_mutex_pthread);
        return fail;
    }
    HSMMsDelay(DELAY_OF_RESTORE_ALL_KEYPAIR);
    ret = HSMRead(rx_buff, rx_buff_len);
    if (ret == 0 && rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }

    #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
    hex_dump(rx_buff, rx_buff_len, 16, "HSMRestoreKeys rx:");
    #endif
    HSMVSemphre();
    pthread_mutex_unlock(&hsm_mutex_pthread);
    return fail;
}

/**
 * @brief
 * This function provides a logical function call interface to the algorithm and initializes the function pointer
 * @param 
 * @return unsigned long
 */
unsigned long FunctionPointerInit(ISTECCFunctionPointer_t *p)
{
    /*512A SUPPORT*/
    p->ISTECC512A_SM2ImportPubkey = ImportSM2Pubkey;
    p->ISTECC512A_SM2ImportPrikey = ImportSM2Prikey;
    p->ISTECC512A_SM2SignMessage = SM2Sign;
    p->ISTECC512A_SM2SingleVerifyMessage = SM2SingleVerify;
    p->ISTECC512A_SM2GenKeyPair = GenKeyPair;
    p->ISTECC512A_SM2ExportPubkey = ExportSM2Pubkey;
    p->ISTECC512A_SM2ExportPrikey = ExportSM2Prikey;
    p->ISTECC512A_SM2Encrypt = SM2Encrypt;
    p->ISTECC512A_SM2Decrypt = SM2Decrypt;
    p->ISTECC512A_SM2SetID = SM2SetID;

    p->ISTECC512A_PinConfirm = PinConfirm;
    p->ISTECC512A_PinConfirmCancel = PinConfirmCancel;
    p->ISTECC512A_PinChange = PinChange;

    p->ISTECC512A_FactoryNumberRead = ReadFactoryNumber;
    /*TEST PASS*/
    p->ISTECC512A_CosVersionRead = CosReadVersion;
    p->ISTECC512A_StatusSync = SyncStatus;

    /*SM4 fucntion . is32u512a not support */
    p->ISTECC512A_SM4ImportKey = SM4ImportKey;
    p->ISTECC512A_SM4Encrypt = SM4Encrpyt;
    p->ISTECC512A_SM4Decrypt = SM4Decrpyt;
    /*Erase  app,reset the chip ,will return to bootloader*/
    p->ISTECC512A_APPErase = APPErase;

    /*add 2022-9-25*/
    p->ISTECC512A_SM2PointerDecompress = SM2PointerDecompress;
    p->ISTECC512A_GetRandom  = V2XDeviceGetRandom;
    /*add cos verison 1.8.8.5 test fucntion.*/
    p->ISTECC512A_DeviceKeyDeriveFlowInit = V2XDeviceKeyDeriveFlowInit;
    p->ISTECC512A_KDFGetbij  =SM2Getbij;
    p->ISTECC512A_KDFGetqij  =SM2Getqij;
    //p->ISTECC512A_KDFGetsij  = SM2SCompleteKeyKDF;
    p->ISTECC512A_KDFGetsij  = SM2Getsij;
    p->ISTECC512A_ModAdd = ModAdd;
    
    /*add 2022-11-8*/
    p->ISTECC512A_SM2VerifyEValueWithPubkeyIndex = SM2EValueVerify;
        /*add 2022-11-23 e value sign.*/
    p->ISTECC512A_SM2SignEValue  =  SM2SignEValue;
    p->ISTECC512A_Restore   =  HSMRestoreKeys;

    /*add  SendOneMessage and RecOneMessage for upgrade tool*/
    p->ISTECC512A_SendOneMessage = IS32U512ASendOneMessage;
    p->ISTECC512A_ReceiveOneMessage = IS32U512AReceiveOneMessage;
    p->ISTECC512A_SendOneMessageOneShot = IS32U512ASendOneMessageOneShot;
}
