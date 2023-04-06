/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
/**
  ******************************************************************************
  * file: hsm_test_task.c
  * brif: HSM Module test demo.
  * ahth: qinxd
  * mail: qinxd@istecc.com
  ******************************************************************************
  * 
  *support  the demo of HSM funciton.
　*
  ******************************************************************************
  */

#include "hsm_test_task.h"

//提供操作底层硬件操作
#include "hsm_hardware_level.h"
//提供算法指令封装操作
#include "hsm_logic_level.h"
//提供计时操作,用来统计算法效率
#include <sys/time.h>

//用来提供malloc支持。数据存储和读取使用
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
//用来提供产生随机数的函数
#include <time.h>
#include <string.h>
#include <unistd.h>

#define SPI_SPEED_01M 1000000
#define SPI_SPEED_10M 10000000
#define SPI_SPEED_15M 15000000
#define SPI_SPEED_18M 18000000
#define SPI_SPEED_20M 20000000
extern unsigned int spi_frequency;
typedef struct{
	unsigned char SM2PubKey[64];
	unsigned char SM2PrivKey[32];
	unsigned char ID[36];
	unsigned long IDLen;	
}SM2KeyPairParTyedef;
/*定义一个支持最大数为5的秘钥池*/
SM2KeyPairParTyedef SM2KeyPairpar[5];

unsigned char sign_verify_tx_buff[1000];
unsigned char sign_verify_rx_buff[128];
unsigned char sign_verify_temp_prikey[32];
unsigned char sign_verify_temp_pubkey[64];
unsigned char  sign_verify_e_value[32];  
unsigned char sign_verify_id[32] = {0x43,0x68,0x69,0x6e,0x61};
unsigned int  sign_verify_id_len = 5;   
unsigned char sign_data[100] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x11,0x22,0x33,0x44
};
unsigned char verify_data[200];

unsigned char encrpyt_data[100] = {
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44
};
unsigned char encrpyt_data_result[200];
unsigned char decrpyt_data[200];
static unsigned char version[4];
unsigned int sign_data_len = 16;
unsigned char sign_verify_rs[64];
unsigned int  sign_verify_error_count =0;
unsigned int  sign_verify_right_count =0;
static unsigned int sign_verify_ret;

unsigned char sm4_key[16] = {
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
unsigned char sm4_message[128] = {
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
};
unsigned char sm4_message_encrypt_result[128];
unsigned char sm4_message_decrypt_result[128];


/*
THis demo will show you how to use the hsm_logic_level and the hsm_hardware_level
1. new a ISTECCFunctionPointer_t. this is a  pointer of fucntion table.
ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
2. init the pointer with
FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
3. reset the HSM module .
4. realize the hsm_hardware_level's function. this demo show the nxp .imx8mq
  1>
*/
unsigned long  IS32U512AReadVerisonTest(void)
{
	int ret;
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	/*reset the 512A module*/
	HSMReset();

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		return 1;
	}
	printf("sync success\n");
	/*demo read verison.
	the verison is  1.8.7  .in this time(2022/6/16)*/
	ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
	printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
	/*deinit the hardware. spi interface and reset,busy io.release the source*/
	HSMHardwareDeinit();
}

/*Erase APP Test,If you not sure what you're doing, don't test it.
 */
unsigned long  ISTECCEraseAPPTest(void)
{
	unsigned int i;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	unsigned char test_id[32];

	int ret;
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	/*reset the 512A module*/
	HSMReset();
	/*How to use sync ? if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		return 1;
	}
	printf("sync success\n");

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("pin confirm failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("pin confirm success!\n");
	}
	/*Erase APP*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPErase();
	if (sign_verify_ret)
	{
		printf("Erase app failed！\n");
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("Erase app success!\n");
	}
	HSMHardwareDeinit();
	return 0;
}



/*Upgated the erasing process, if the PIN code authentication failed. it will return directly.*/
unsigned long  ISTECCEraseAPPTest2(void)
{
	unsigned int i;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	unsigned char test_id[32];

	int ret;
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	/*reset the 512A module*/
	HSMReset();
	/*How to use sync ? if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		return 1;
	}
	printf("sync success\n");

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("pin confirm failed!\n");
		sign_verify_error_count++;
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("pin confirm success!\n");
	}
	/*Erase APP*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPErase();
	if (sign_verify_ret)
	{
		printf("Erase app failed！\n");
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("Erase app success!\n");
	}
	HSMHardwareDeinit();
	return 0;
}

/*update APP*/
unsigned long ISTECCUpdateTest(void)
{
  	int ret;
  	/*Create a pointer struct and Init it. */
  	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
 	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	/*reset the 512A module*/
	HSMReset();
	HSMMsDelay(300);
 	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPUpdate();
  	if (ret)
  	{
		printf("update failed\n");
    	HSMHardwareDeinit();
    	return 1;
  	}
  	else
  	{
    	printf("update success!\n");
  	}
  	HSMHardwareDeinit();
  	return 0;
}


