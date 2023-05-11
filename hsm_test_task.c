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

/**
 * @brief
 * This demo show how to use the hsm_logic_level and hsm_hardware_level.
 * 2022-6-30 10:24:23  sign_data_len = 100;  set length of message date is 100.
 * @return int
 */
unsigned long  IS32U512AFunctionTest(void)
{
	int ret;
	unsigned int i;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (sign_verify_ret)
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

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_FactoryNumberRead(result);
	printf("IS32U512A module's ISTECC512A_FactoryNumberRead is  %s\n", result);

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

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirmCancel(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_PinConfirmCancel failed !\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_PinConfirmCancel success!\n");
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_PinConfirm failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_PinConfirm success!\n");
	}

	// sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinChange(default_pin, new_pin, 8);
	// if (sign_verify_ret)
	// {
	// 	printf("ISTECC512A_PinChange failed!\n");
	// 	sign_verify_error_count++;
	// }
	// else
	// {
	// 	printf("ISTECC512A_PinChange success!\n");
	// }

	
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2GenKeyPair(0);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2GenKeyPair failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2GenKeyPair success!\n");
	}
	
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(0, sign_verify_temp_pubkey, sign_verify_temp_pubkey + 32);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2ExportPubkey failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2ExportPubkey success!\n");
	}
	
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(0, sign_verify_temp_prikey);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2ExportPrikey failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2ExportPrikey success!\n");
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(0, sign_verify_temp_pubkey,
																					sign_verify_temp_pubkey + 32);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2ImportPubkey failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2ImportPubkey success!\n");
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPrikey(0, sign_verify_temp_prikey);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2ImportPrikey success!\n");
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SetID(0, sign_verify_id, sign_verify_id_len);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2SetID failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2SetID success!\n");
	}
	/*Message 签名*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignMessage(0, sign_data, sign_data_len, sign_verify_rs);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2SignMessage failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("the length of message is %d sign success!\n", sign_data_len);
	}

	/*verify.INDEX=0. PACKAGE:CMD + MESSAGE + RS*/
	memcpy(verify_data, sign_data, sign_data_len);
	memcpy(verify_data + sign_data_len, sign_verify_rs, 64);
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SingleVerifyMessage(0, verify_data, sign_data_len, sign_verify_rs);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2SingleVerifyMessage failed\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2SingleVerifyMessage success!\n");
		sign_verify_right_count++;
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(0, encrpyt_data, sign_data_len, encrpyt_data_result);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2Encrypt failed\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2Encrypt success!\n");
		sign_verify_right_count++;
	}
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(0, encrpyt_data_result, sign_data_len + 96, decrpyt_data);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2Decrypt failed\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2Decrypt success!\n");
		sign_verify_right_count++;
	}

	printf("verify success time is %04d and error count is %04d\n", sign_verify_right_count, sign_verify_error_count);
	sign_verify_right_count = 0;
	sign_verify_error_count = 0;
	/*deinit the hardware. spi interface and reset,busy io.release the source*/
	HSMHardwareDeinit();
}

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
	HSMHardwareInit(SPI_SPEED_10M);
	//HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	//HSMReset();

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
	//HSMHardwareDeinit();
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
	HSMHardwareInit(SPI_SPEED_10M);
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
	HSMHardwareInit(SPI_SPEED_10M);
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
	HSMHardwareInit(SPI_SPEED_01M);
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




/**
 * @brief 
 * Set id and HASH value verify Test.
 * 
 * @return int 
 */
unsigned long  ISTECCSetIDTest(void)
{
	// unsigned int i;
	// unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	// unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	// unsigned char result[16];

	// int ret;
	// /*Create a pointer struct and Init it. */
	// ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	// FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	// /*Init the hardware . spi interface  and reset,busy io*/
	// HSMHardwareInit(SPI_SPEED_10M);
	// /*reset the 512A module*/
	// HSMReset();
	// /*How to use sync .if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	// ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	// if (ret)
	// {
	// 	printf("sync failed\n");
	// 	HSMHardwareDeinit();
	// 	return 1;
	// }
	// printf("sync success\n");

	// /*pin confirm*/
	// printf("pin is 8 byte password.!\n");
	// sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	// if (sign_verify_ret)
	// {
	// 	printf("pin confirm failed!\n");
	// 	sign_verify_error_count++;
	// }
	// else
	// {
	// 	printf("pin confirm success!\n");
	// }
	// /*Erase APP*/
	// sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPErase();
	// if (sign_verify_ret)
	// {
	// 	printf("Erase app failed！\n");
	// 	HSMHardwareDeinit();
	// 	return 1;
	// }
	// else
	// {
	// 	printf("Erase app success!\n");
	// }
	// HSMHardwareDeinit();
	return 0;
}


/*Test pointer decompress.

this is the test  data
Y0
PUBKEY:
3015FAF1F4805427784895861175AF5130F022D11140ECC51D4CA484F38FB8E5
DE91377B5641B6D88A9D5F176AA75542A8C063EFA8812BE983CC240802C015D2
PRIKEY:
28989CB9A0C17B6A3B26C2BDCD7C0395C4390147377F3C7C800D2C503A86A2CC

Y1
PUBKEY:
7445AB24000E41AC92F6C7DD14A48E6372AC69B53FC08A613BD89BB5D514D9B7
3200E5DC216D5D89526CBB64E22BE3078FD616935F68992CA610B614193F2329
PRIKEY:
188EE0B1C10B742403ED39E06888D94DFB6536F971D0C36C351643A5FAC8AAE8
*/
unsigned long  ISTECCSPointerDecompressTest(void)
{
	unsigned int i;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];


	/*Y0  mode 2*/
	unsigned char test_public[64] = {
		0x30,0x15,0xFA,0xF1,0xF4,0x80,0x54,0x27,
		0x78,0x48,0x95,0x86,0x11,0x75,0xAF,0x51,
		0x30,0xF0,0x22,0xD1,0x11,0x40,0xEC,0xC5,
		0x1D,0x4C,0xA4,0x84,0xF3,0x8F,0xB8,0xE5,
		0xDE,0x91,0x37,0x7B,0x56,0x41,0xB6,0xD8,
		0x8A,0x9D,0x5F,0x17,0x6A,0xA7,0x55,0x42,
		0xA8,0xC0,0x63,0xEF,0xA8,0x81,0x2B,0xE9,
		0x83,0xCC,0x24,0x08,0x02,0xC0,0x15,0xD2
	};
	unsigned char test_private[32]= {
		0x28,0x98,0x9C,0xB9,0xA0,0xC1,0x7B,0x6A,
		0x3B,0x26,0xC2,0xBD,0xCD,0x7C,0x03,0x95,
		0xC4,0x39,0x01,0x47,0x37,0x7F,0x3C,0x7C,
		0x80,0x0D,0x2C,0x50,0x3A,0x86,0xA2,0xCC
	};


	
	/* add new test data Y1 mode 3*/
	unsigned char test_public2[64] = {
	0x74,0x45,0xAB,0x24,0x00,0x0E,0x41,0xAC,
	0x92,0xF6,0xC7,0xDD,0x14,0xA4,0x8E,0x63,
	0x72,0xAC,0x69,0xB5,0x3F,0xC0,0x8A,0x61,
	0x3B,0xD8,0x9B,0xB5,0xD5,0x14,0xD9,0xB7,
	0x32,0x00,0xE5,0xDC,0x21,0x6D,0x5D,0x89,
	0x52,0x6C,0xBB,0x64,0xE2,0x2B,0xE3,0x07,
	0x8F,0xD6,0x16,0x93,0x5F,0x68,0x99,0x2C,
	0xA6,0x10,0xB6,0x14,0x19,0x3F,0x23,0x29
	};
	unsigned char test_private2[32]= {
	0x18,0x8E,0xE0,0xB1,0xC1,0x0B,0x74,0x24,
	0x03,0xED,0x39,0xE0,0x68,0x88,0xD9,0x4D,
	0xFB,0x65,0x36,0xF9,0x71,0xD0,0xC3,0x6C,
	0x35,0x16,0x43,0xA5,0xFA,0xC8,0xAA,0xE8	
	};

	unsigned char test_temp[64];
	int ret;
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();
	/*How to use sync .if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
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
	/*Pointer Decompress mode 2 y0*/
	memcpy(test_temp,test_public,32);
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2PointerDecompress(test_temp,ISTECC_POINT_DECOMPRESS_2,test_temp);

	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2PointerDecompress failed！\n");
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("ISTECC512A_SM2PointerDecompress success!\n");
		ret = memcmp(test_public,test_temp,64);
		if(ret)
		{
			printf("Decomplress result error! maybe the mode is wrong.\n");
		}
	}

	/*Pointer Decompress mode 3.   y1*/
	memset(test_temp,0x00,64);
	memcpy(test_temp,test_public2,32);
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2PointerDecompress(test_temp,ISTECC_POINT_DECOMPRESS_3,test_temp);

	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2PointerDecompress failed！\n");
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("ISTECC512A_SM2PointerDecompress success!\n");
		ret = memcmp(test_public2,test_temp,64);
		if(ret)
		{
			printf("Decomplress result error! maybe the mode is wrong.\n");
		}
	}
	HSMHardwareDeinit();
	return 0;
}





static int test_complete_private_key_derivative(void)
{
    /*gen sm2 key-pair.*/
	uint32_t ret;
	uint32_t i = 71;
	uint32_t j = 10;
	/*Put the key -pair in  index 15 buff.*/
	uint32_t CONST_KEY_STORE_INDEX = 0;
	uint32_t index = 0;
	uint32_t time = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	const unsigned char  right_ecpryt_kdf_result[]  = {
		0x1d,0x89,0x14,0x3b,0xcf,0xbf,0xc0,0x5b,
		0x19,0xc2,0x87,0x90,0x23,0x2c,0x1f,0x7e,
		0x0f,0xa6,0xbe,0xf0,0xf1,0x56,0x75,0xf5,
		0x7d,0xf1,0x9f,0x8e,0xbd,0x1b,0x8b,0x09
	};
	const unsigned char  right_sign_kdf_result[]  = {
		0x74,0x18,0x3a,0xc3,0xff,0x6f,0x81,0x00,
		0xe2,0x4e,0x6a,0x8c,0xd1,0xe8,0xdd,0x3e,
		0x8d,0x6a,0x98,0x5e,0x92,0xd1,0x55,0x66,
		0x26,0x69,0x2a,0x07,0x7d,0x84,0xb5,0x1f
	};
	//334b266f348d935e72e8649d290db70d
	// uint8_t ke[16] =
	// {
	// 	0x33,0x4b,0x26,0x6f,0x34,0x8d,0x93,0x5e,0x72,0xe8,0x64,0x9d,0x29,0x0d,0xb7,0x0d
	// };
	uint8_t ke[16] =
	{
		0x33,0x4b,0x26,0x6f, 
		0x34,0x8d,0x93,0x5e,  
		0x72,0xe8,0x64,0x9d,  
		0x29,0x0d,0xb7,0x0d   
	};
	uint8_t pri_key[32] =
	{
		0X58,0X0e,0Xe0,0X2f,0X22,0Xec,0Xf9,0X08,0X20,0X40,0X88,0X20,0X26,0X8b,0Xa9,0Xfa,
		0X11,0Xbb,0X49,0X6a,0Xab,0X91,0Xcb,0Xe9,0Xf3,0Xa3,0X75,0X87,0X89,0Xf2,0X43,0X54
	};
	uint8_t pub_key[64] = {

		0x8f,0Xcb,0X37,0X10,0X08,0Xf5,0X03,0Xdd,0X8e,0Xdf,0X0e,0Xc2,0Xc6,0X34,0X3f,0X9b,
		0Xea,0X1d,0Xca,0Xfb,0Xd8,0X23,0Xcb,0Xb2,0Xbb,0Xd0,0X02,0Xfd,0X55,0X24,0X7b,0Xdf,
		0X33,0X1a,0X90,0Xf9,0Xd3,0Xcd,0X6f,0X6c,0Xa3,0X5c,0X51,0Xe8,0X6d,0Xf7,0X01,0X9c,
		0X6a,0X0b,0X7c,0X50,0X17,0X15,0Xce,0X82,0Xe5,0Xe7,0X55,0X90,0X7f,0X52,0Xef,0X33
	};
	uint8_t c[32] =
    {
        0x52,0X53,0Xb3,0X6c,0X8e,0X70,0X1a,0X4a,0X61,0X98,0X72,0X70,0X0c,0X9d,0X59,0X5e,
		0X24,0X05,0X2e,0X73,0X9f,0Xfc,0X02,0X64,0X53,0X25,0X69,0X20,0X63,0X28,0Xcd,0X9b
    };

	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();

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

    /*导入私钥到最后一组KEY_PAIR空间*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPrikey(CONST_KEY_STORE_INDEX,pri_key);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed,index is %4d\n",CONST_KEY_STORE_INDEX);
		HSMHardwareDeinit();
		return 1;
	}
	printf("ISTECC512A_SM2ImportPrikey success\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(CONST_KEY_STORE_INDEX,&pub_key[0],&pub_key[32]);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed,index is %4d\n",CONST_KEY_STORE_INDEX);
		HSMHardwareDeinit();
		return 1;
	}
	printf("ISTECC512A_SM2ImportPrikey success\n");

	/*sign key KDF and put it in index */
	index = 5;
    ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2CompleteKDF(CONST_KEY_STORE_INDEX,index, i, j , ke,c);
    if (ret)
    {
        printf("ISTECC512A_SM2CompleteKDF failed.\n");
    }
    else
    {

        printf("ISTECC512A_SM2CompleteKDF success\n");
    }

    ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(index, pri_key);
    if (ret)
    {
        printf("ExportSM2Prikey error!\n");
    }
    printf("ExportSM2Prikey success ,INDEX IS %4d!\n",index);
    hex_dump(pri_key, 32, 16, "PRI_KEY:");
    return ret;
}


static int test_encrypt_and_sign_key_derivative(void)
{

    /*gen sm2 key-pair.*/
	uint32_t ret;
	uint32_t i = 71;
	uint32_t j = 10;
	/*Put the key -pair in  index 15 buff.*/
	uint32_t CONST_KEY_STORE_INDEX = 0;
	uint32_t index = 0;
	uint32_t time = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	const unsigned char  right_ecpryt_kdf_result[]  = {
		0x1d,0x89,0x14,0x3b,0xcf,0xbf,0xc0,0x5b,
		0x19,0xc2,0x87,0x90,0x23,0x2c,0x1f,0x7e,
		0x0f,0xa6,0xbe,0xf0,0xf1,0x56,0x75,0xf5,
		0x7d,0xf1,0x9f,0x8e,0xbd,0x1b,0x8b,0x09
	};
	const unsigned char  right_sign_kdf_result[]  = {
		0x74,0x18,0x3a,0xc3,0xff,0x6f,0x81,0x00,
		0xe2,0x4e,0x6a,0x8c,0xd1,0xe8,0xdd,0x3e,
		0x8d,0x6a,0x98,0x5e,0x92,0xd1,0x55,0x66,
		0x26,0x69,0x2a,0x07,0x7d,0x84,0xb5,0x1f
	};
	//334b266f348d935e72e8649d290db70d
	// uint8_t ke[16] =
	// {
	// 	0x33,0x4b,0x26,0x6f,0x34,0x8d,0x93,0x5e,0x72,0xe8,0x64,0x9d,0x29,0x0d,0xb7,0x0d
	// };
	uint8_t ke[16] =
	{
		0x33,0x4b,0x26,0x6f, 
		0x34,0x8d,0x93,0x5e,  
		0x72,0xe8,0x64,0x9d,  
		0x29,0x0d,0xb7,0x0d   
	};
	uint8_t pri_key[32] =
	{
		0X58,0X0e,0Xe0,0X2f,0X22,0Xec,0Xf9,0X08,0X20,0X40,0X88,0X20,0X26,0X8b,0Xa9,0Xfa,
		0X11,0Xbb,0X49,0X6a,0Xab,0X91,0Xcb,0Xe9,0Xf3,0Xa3,0X75,0X87,0X89,0Xf2,0X43,0X54
	};
	uint8_t pub_key[64] = {

		0x8f,0Xcb,0X37,0X10,0X08,0Xf5,0X03,0Xdd,0X8e,0Xdf,0X0e,0Xc2,0Xc6,0X34,0X3f,0X9b,
		0Xea,0X1d,0Xca,0Xfb,0Xd8,0X23,0Xcb,0Xb2,0Xbb,0Xd0,0X02,0Xfd,0X55,0X24,0X7b,0Xdf,
		0X33,0X1a,0X90,0Xf9,0Xd3,0Xcd,0X6f,0X6c,0Xa3,0X5c,0X51,0Xe8,0X6d,0Xf7,0X01,0X9c,
		0X6a,0X0b,0X7c,0X50,0X17,0X15,0Xce,0X82,0Xe5,0Xe7,0X55,0X90,0X7f,0X52,0Xef,0X33
	};
	uint8_t c[32] =
    {
        0x52,0X53,0Xb3,0X6c,0X8e,0X70,0X1a,0X4a,0X61,0X98,0X72,0X70,0X0c,0X9d,0X59,0X5e,
		0X24,0X05,0X2e,0X73,0X9f,0Xfc,0X02,0X64,0X53,0X25,0X69,0X20,0X63,0X28,0Xcd,0X9b
    };

	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();
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

    /*导入私钥到最后一组KEY_PAIR空间*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPrikey(CONST_KEY_STORE_INDEX,pri_key);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed,index is %4d\n",CONST_KEY_STORE_INDEX);
		HSMHardwareDeinit();
		return 1;
	}
	printf("ISTECC512A_SM2ImportPrikey success\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(CONST_KEY_STORE_INDEX,&pub_key[0],&pub_key[32]);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed,index is %4d\n",CONST_KEY_STORE_INDEX);
		HSMHardwareDeinit();
		return 1;
	}
	printf("ISTECC512A_SM2ImportPrikey success\n");

	/*sign key KDF and put it in index */
	index = 5;
    ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2EncryptSignKeyKDF(CONST_KEY_STORE_INDEX,index, i, j , ke);
    if (ret)
    {
        printf("ISTECC512A_SM2EncryptKeyKDF failed.\n");
    }
    else
    {

        printf("ISTECC512A_SM2EncryptKeyKDF success\n");
    }

    ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(index, pri_key);
    if (ret)
    {
        printf("ExportSM2Prikey error!\n");
    }
    printf("ExportSM2Prikey success ,INDEX IS %4d!\n",index);
    hex_dump(pri_key, 32, 16, "PRI_KEY:");
    return ret;
}


// int ISTECCKeyDeriveTest(void)
// {

//     int ret;
	
// #if 1
// 		ret = test_encrypt_and_sign_key_derivative();
// 		if (ret)
// 		{
// 			printf("encpryt key derive failed!\n");
// 			return 1;
// 		}
	
// #endif

//     ret = test_complete_private_key_derivative();

//     if (ret)
//     {
//         printf("sign key derive failed!\n");

//     }
//     return ret;

// }


/*
*key derive flow's init. get kS kE, (a,A) (p,P)
*1.a gen  sm4's kS 
*1.b gen  sm4's kE
*1.c gen  SM2 keypiar's pubkey A (a,A)
*1.d gen  SM2 keypiar's pubkey P (p,P)*/

/*
	*8.V2X Device:
	*For each ( i INT, j INT) , compute:
	*a) Signature extended private key, b i , j = ( a+ f S ( k S , i INT, j INT)) mod l
	*b) Encrypted extended private key, q i , j = ( p+ f E ( k E , i INT, j INT)) mod l
	*9) For each ( i INT, j INT) , verify the PCA signature in SCT i , j :
	*a) If the verification is successful,decrypt CT i , j using qi , j to get ( PCi , j , c ) , 
	*and calculate the complete private key s i , j = ( b i ) corresponding to the public key in the PC i , j , j + c ) mod l
	*b) if validation fails, exit
	*/	

// int  V2XDeviceGetKeyDeriveSignPrivateKey(int i,int j,char *kS, char *kE,unsigned char * CTij,int index)
// {

// 	return 0;
// }
/*
this demo show the full flow of the key derive.
*/
unsigned long ISTECCKeyDeriveTest(void)
{
	int ret;
	int i = 71;
	int j = 10;

	char kS[16];
	char kE[16];
	char sign_keypair_prikey_factor[32];
	char sign_keypair_pubkey_factor[64];
	char encrpyt_keypair_prikey_factor[32];
	char encrpyt_keypair_pubkey_factor[64];

	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();


	/*TEST RANDOM*/
	ISTECC512AFunctionPointerStructure.ISTECC512A_GetRandom(encrpyt_keypair_pubkey_factor,64);
	/*1. Get 2 sets of sm4 key and 2 sets of sm2 pubkey */
	ISTECC512AFunctionPointerStructure.ISTECC512A_DeviceKeyDeriveFlowInit(kS,kE,sign_keypair_pubkey_factor,encrpyt_keypair_pubkey_factor);
	/*
	*2.Now you get the (kS,kE,A,P) 
	*V2X device send (kS,kE,A,P) with EcRaCertRequest to PRA.
	*/
	
	/*
	*3.PRA will calculate the Bi,j Qi,j for every (iINT, jINT)
	*
	*/
	/*
	*4.PRA send (kS,kE,A,P) with EaAcaCertRequest to PCA.
	*
	*/

	/*
	*5.PCA:Generate a pair of SM2 keys ( c, C=c * G ) to hide the complete public
	*key in the pseudonymous certificate  from the PRA , and calculate the complete public key S i , j = B i , j + C ;
	/*

	/*
	*6.PCA:Use the complete public key S i , j to
	*construct ToBeSignedCertificate , sign the explicit certificate PC i , j , use
	*Qi , j to encrypt ( PC i , j , c ) to obtain
	*the ciphertext CT i , j , and the PCA reencrypts the ciphertext Sign to get SCT i , j , and
	*send SCT i , j to PRA
	*/


	/*
	*7.PRA:For a certain V2X device, PRA
	*organizes and packages all its S CT i ,
	*j in a certain stage i , and the j
	*corresponding to each S CT i , j , and
	*returns after the V2X device initiates
	*a pseudonym certificate download request.
	*/

	/*
	*8.V2X Device:
	*For each ( i INT, j INT) , compute:
	*a) Signature extended private key, b i , j = ( a+ f S ( k S , i INT, j INT)) mod l
	*b) Encrypted extended private key, q i , j = ( p+ f E ( k E , i INT, j INT)) mod l
	*9) For each ( i INT, j INT) , verify the PCA signature in SCT i , j :
	*a) If the verification is successful,decrypt CT i , j using qi , j to get ( PCi , j , c ) , 
	*and calculate the complete private key s i , j = ( b i ) corresponding to the public key in the PC i , j , j + c ) mod l
	*b) if validation fails, exit
	*/	
	// ret = V2XDeviceGetKeyDeriveSignPrivateKey(i,j,kS,kE,0);
	return ret;
}


/*如何使用SM4 CCM加密？
	1.设置KEY参数
	2.调用ccm_auth_crypt
	详细说明:
	
	int ccm_auth_crypt( sm4_context *ctx, int mode, unsigned int length,
                           const unsigned char *iv, unsigned int iv_len,
                           const unsigned char *add, unsigned int add_len,
                           const unsigned char *input, unsigned char *output,
                           unsigned char *tag, unsigned int tag_len )


	加密模式:
	参数1:&ctx 在函数开始声明
	sm4_context ctx即可。
	参数2:模式  
	CCM_ENCRYPT 加密模式
	CCM_DECRYPT 解密模式
	参数3:unsigned int length
	加密/解密数据的长度
	参数4:IV值的指针。通常在SM4CCM中这个值被成为nonce.长度为12字节。
	参数5:IV的长度,SM4CCM为12字节
	参数6:AAD数据。可选的认证数据。如果不使用,长度设置为0'
	参数7:AAD.也就是参数5的长度.如果没有使用,设置为0
	参数8:input  输入值 在加密模式下,输入明文plaintext.参数3的长度就是明文的长度。
	参数9:output 加密模式下的加密结果输出
	参数10:MAC值,也就是TAG值.长度为16.在加密模式下是输出参数
	参数11:tag的长度,SM4CCM固定为16(0X10)bytes.

	解密模式:
	参数1:&ctx 在函数开始声明
	sm4_context ctx即可。
	参数2:模式  
	CCM_ENCRYPT 加密模式
	CCM_DECRYPT 解密模式
	参数3:unsigned int length
	加密/解密数据的长度
	参数4:IV值的指针。通常在SM4CCM中这个值被成为nonce.长度为12字节。
	参数5:IV的长度,SM4CCM为12字节
	参数6:AAD数据。可选的认证数据。如果不使用,长度设置为0'
	参数7:AAD.也就是参数5的长度.如果没有使用,设置为0
	参数8:input  输入值 在解密模式下,输入密文的数据CipherText.
	参数9:output 解密模式下的解密结果输出
	参数10:MAC值,也就是TAG值.长度为16.在解密模式下,是输入参数
	参数11:tag的长度,SM4CCM固定为16(0X10)bytes.
*/
unsigned long ISTECCSm4CcmTest(void)
{
	return 0;
}



/*如何使用SM2 加密/解密？
1.先来了解以下SM2加密和RSA的不同。
RSA的加密结果和输入的明文的长度是一致的。
SM2的加密结果，分为3个部分。C1 C3 C2
C1 = [k]G = (x1,y1)   64 Bytes
C2 = M xor t   长度和输入的明文相同
C3 = hash(x2||M||y2) 使用SM3进行HASH. 32字节。
加密结果的输出格式为:
C1 || C3 || C2
长度为
64 + 数据长度 + 32
同样的,解密的时候传入的参数也是 
C1 || C3 || C2的格式
C1和C3是固定的长度。
waring:由于SM2加密有随机数参与,所以他的解密结果不是固定的。

以下是一个SM2加密/解密的例子。
1.产生一组秘钥对是必要的。但是导出秘钥对和导出秘钥对不是必须的。此处是为了方便使用其他工具进行数据的验证。
2.产生秘钥对和导入秘钥对,选其中一个即可.产生的秘钥对是随机的。如果有固定的测试秘钥，可以选择使用导入秘钥功能。
*/
unsigned long  IS32U512ASM2EncyprtDecryptTest(void)
{
	int ret = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	unsigned char encrpyt_data_result[200];
	unsigned char decrpyt_data[200];
	unsigned char version[4];
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
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

	ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
	printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_FactoryNumberRead(result);
	printf("IS32U512A module's ISTECC512A_FactoryNumberRead is  %s\n", result);

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

	
	
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2GenKeyPair(0);
	if (ret)
	{
		printf("ISTECC512A_SM2GenKeyPair failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_SM2GenKeyPair success!\n");
	}
	
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(0, sign_verify_temp_pubkey, sign_verify_temp_pubkey + 32);
	if (ret)
	{
		printf("ISTECC512A_SM2ExportPubkey failed!\n");
	}
	else
	{
		printf("ISTECC512A_SM2ExportPubkey success!\n");
	}
	
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(0, sign_verify_temp_prikey);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_SM2ExportPrikey failed!\n");
	}
	else
	{
		printf("ISTECC512A_SM2ExportPrikey success!\n");
	}

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(0, sign_verify_temp_pubkey,
																					sign_verify_temp_pubkey + 32);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPubkey failed!\n");
	}
	else
	{
		printf("ISTECC512A_SM2ImportPubkey success!\n");
	}

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPrikey(0, sign_verify_temp_prikey);
	if (ret)
	{
		printf("ISTECC512A_SM2ImportPrikey failed!\n");
	}
	else
	{
		printf("ISTECC512A_SM2ImportPrikey success!\n");
	}

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(0, encrpyt_data, 100, encrpyt_data_result);
	if (ret)
	{
		printf("ISTECC512A_SM2Encrypt failed\n");
	}
	else
	{
		printf("ISTECC512A_SM2Encrypt success!\n");
	}
	hex_dump(encrpyt_data_result,100+96,16,"encrpyt_data_result:");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(0, encrpyt_data_result, 100 + 96, decrpyt_data);
	if (ret)
	{
		printf("ISTECC512A_SM2Decrypt failed\n");
	}
	else
	{
		printf("ISTECC512A_SM2Decrypt success!\n");
	}
	hex_dump(decrpyt_data,100,16,"decrpyt_data result:");
	HSMHardwareDeinit();
	return  0;
}



/*
A right test data.
private key:
CE0169F3C02C711786FBD573DF010C34886CAD14AE92B48A9074C89F9247ACF5
id:
054368696e61
e:
3687ABF3F13341AB9AEC9A63B056EEC6CF92B7D866DCCF7DC7D8194E295592FF
pubkey:
226460E469D77ED18E8C04E1094E9675DAD386D14D9746C4E22EB52A1E22FD88B9F53705CB7AB1BD5CDD19C0CB24CDA3EBDB50274E4AD667A72D73B1D8B1C52A
rs:
AB37DBBEE735AACFE17942E7F405B26345037CDE0F352C4AA898DA1F35FFD7984DC86A3F93C7462B9B4B12FB5AAC6752074A09D72CC7EE5482346FEDDCE39229

private key:
0xCE,0x01,0x69,0xF3,0xC0,0x2C,0x71,0x17,0x86,0xFB,0xD5,0x73,0xDF,0x01,0x0C,0x34,0x88,0x6C,0xAD,0x14,0xAE,0x92,0xB4,0x8A,0x90,0x74,0xC8,0x9F,0x92,0x47,0xAC,0xF5
e:
0x36,0x87,0xAB,0xF3,0xF1,0x33,0x41,0xAB,0x9A,0xEC,0x9A,0x63,0xB0,0x56,0xEE,0xC6,0xCF,0x92,0xB7,0xD8,0x66,0xDC,0xCF,0x7D,0xC7,0xD8,0x19,0x4E,0x29,0x55,0x92,0xFF
pubkey:
0x22,0x64,0x60,0xE4,0x69,0xD7,0x7E,0xD1,0x8E,0x8C,0x04,0xE1,0x09,0x4E,0x96,0x75,0xDA,0xD3,0x86,0xD1,0x4D,0x97,0x46,0xC4,0xE2,0x2E,0xB5,0x2A,0x1E,0x22,0xFD,0x88,
0xB9,0xF5,0x37,0x05,0xCB,0x7A,0xB1,0xBD,0x5C,0xDD,0x19,0xC0,0xCB,0x24,0xCD,0xA3,0xEB,0xDB,0x50,0x27,0x4E,0x4A,0xD6,0x67,0xA7,0x2D,0x73,0xB1,0xD8,0xB1,0xC5,0x2A
rs:
0xAB,0x37,0xDB,0xBE,0xE7,0x35,0xAA,0xCF,0xE1,0x79,0x42,0xE7,0xF4,0x05,0xB2,0x63,0x45,0x03,0x7C,0xDE,0x0F,0x35,0x2C,0x4A,0xA8,0x98,0xDA,0x1F,0x35,0xFF,0xD7,0x98,
0x4D,0xC8,0x6A,0x3F,0x93,0xC7,0x46,0x2B,0x9B,0x4B,0x12,0xFB,0x5A,0xAC,0x67,0x52,0x07,0x4A,0x09,0xD7,0x2C,0xC7,0xEE,0x54,0x82,0x34,0x6F,0xED,0xDC,0xE3,0x92,0x29

*/

/*对R+RS + 公钥index这种模式进行验签*/
unsigned long  IS32U512ASm2VerifyEvalueWithPubKeyIndex(void)
{
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	unsigned char z[32];
	unsigned char private_key[32] = {0xCE,0x01,0x69,0xF3,0xC0,0x2C,0x71,0x17,0x86,0xFB,0xD5,0x73,0xDF,0x01,0x0C,0x34,0x88,0x6C,0xAD,0x14,0xAE,0x92,0xB4,0x8A,0x90,0x74,0xC8,0x9F,0x92,0x47,0xAC,0xF5
	};
	unsigned char e[32] = {
		0x36,0x87,0xAB,0xF3,0xF1,0x33,0x41,0xAB,0x9A,0xEC,0x9A,0x63,0xB0,0x56,0xEE,0xC6,0xCF,0x92,0xB7,0xD8,0x66,0xDC,0xCF,0x7D,0xC7,0xD8,0x19,0x4E,0x29,0x55,0x92,0xFF
	};
	unsigned char id[33];
	unsigned char rs[64] = {
		0xAB,0x37,0xDB,0xBE,0xE7,0x35,0xAA,0xCF,0xE1,0x79,0x42,0xE7,0xF4,0x05,0xB2,0x63,0x45,0x03,0x7C,0xDE,0x0F,0x35,0x2C,0x4A,0xA8,0x98,0xDA,0x1F,0x35,0xFF,0xD7,0x98,
0x4D,0xC8,0x6A,0x3F,0x93,0xC7,0x46,0x2B,0x9B,0x4B,0x12,0xFB,0x5A,0xAC,0x67,0x52,0x07,0x4A,0x09,0xD7,0x2C,0xC7,0xEE,0x54,0x82,0x34,0x6F,0xED,0xDC,0xE3,0x92,0x29

	};
	unsigned char pubkey[64] =  {
		0x22,0x64,0x60,0xE4,0x69,0xD7,0x7E,0xD1,0x8E,0x8C,0x04,0xE1,0x09,0x4E,0x96,0x75,0xDA,0xD3,0x86,0xD1,0x4D,0x97,0x46,0xC4,0xE2,0x2E,0xB5,0x2A,0x1E,0x22,0xFD,0x88,
0xB9,0xF5,0x37,0x05,0xCB,0x7A,0xB1,0xBD,0x5C,0xDD,0x19,0xC0,0xCB,0x24,0xCD,0xA3,0xEB,0xDB,0x50,0x27,0x4E,0x4A,0xD6,0x67,0xA7,0x2D,0x73,0xB1,0xD8,0xB1,0xC5,0x2A

	};

	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (sign_verify_ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		return 1;
	}
	printf("sync success\n");

	/*demo read verison.*/
	ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
	printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_FactoryNumberRead(result);
	printf("IS32U512A module's ISTECC512A_FactoryNumberRead is  %s\n", result);

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

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirmCancel(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_PinConfirmCancel failed !\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_PinConfirmCancel success!\n");
	}

	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (sign_verify_ret)
	{
		printf("ISTECC512A_PinConfirm failed!\n");
		sign_verify_error_count++;
	}
	else
	{
		printf("ISTECC512A_PinConfirm success!\n");
	}

	for(i=0;i<16;i++)
	{
		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(i, pubkey,
																					pubkey + 32);
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2ImportPubkey failed!\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2ImportPubkey success!\n");
		}

		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,e,rs);
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex success!\n");
			sign_verify_right_count++;
		}

		printf("verify success time is %04d and error count is %04d\n", sign_verify_right_count, sign_verify_error_count);

	}


	/*deinit the hardware. spi interface and reset,busy io.release the source*/
	HSMHardwareDeinit();
}


/*E+RS verify and  e sign.*/
unsigned long  IS32U512ASm2SignEvalueAndVerifyEvalueWithPubKeyIndex(void)
{
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char new_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	unsigned char result[16];
	unsigned char z[32];
	unsigned char private_key[32] = {0xCE,0x01,0x69,0xF3,0xC0,0x2C,0x71,0x17,0x86,0xFB,0xD5,0x73,0xDF,0x01,0x0C,0x34,0x88,0x6C,0xAD,0x14,0xAE,0x92,0xB4,0x8A,0x90,0x74,0xC8,0x9F,0x92,0x47,0xAC,0xF5
	};
	unsigned char e[32] = {
		0x36,0x87,0xAB,0xF3,0xF1,0x33,0x41,0xAB,0x9A,0xEC,0x9A,0x63,0xB0,0x56,0xEE,0xC6,0xCF,0x92,0xB7,0xD8,0x66,0xDC,0xCF,0x7D,0xC7,0xD8,0x19,0x4E,0x29,0x55,0x92,0xFF
	};
	unsigned char rs[64] = {
		0xAB,0x37,0xDB,0xBE,0xE7,0x35,0xAA,0xCF,0xE1,0x79,0x42,0xE7,0xF4,0x05,0xB2,0x63,0x45,0x03,0x7C,0xDE,0x0F,0x35,0x2C,0x4A,0xA8,0x98,0xDA,0x1F,0x35,0xFF,0xD7,0x98,
0x4D,0xC8,0x6A,0x3F,0x93,0xC7,0x46,0x2B,0x9B,0x4B,0x12,0xFB,0x5A,0xAC,0x67,0x52,0x07,0x4A,0x09,0xD7,0x2C,0xC7,0xEE,0x54,0x82,0x34,0x6F,0xED,0xDC,0xE3,0x92,0x29

	};
	unsigned char pubkey[64] =  {
		0x22,0x64,0x60,0xE4,0x69,0xD7,0x7E,0xD1,0x8E,0x8C,0x04,0xE1,0x09,0x4E,0x96,0x75,0xDA,0xD3,0x86,0xD1,0x4D,0x97,0x46,0xC4,0xE2,0x2E,0xB5,0x2A,0x1E,0x22,0xFD,0x88,
0xB9,0xF5,0x37,0x05,0xCB,0x7A,0xB1,0xBD,0x5C,0xDD,0x19,0xC0,0xCB,0x24,0xCD,0xA3,0xEB,0xDB,0x50,0x27,0x4E,0x4A,0xD6,0x67,0xA7,0x2D,0x73,0xB1,0xD8,0xB1,0xC5,0x2A

	};

	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(SPI_SPEED_10M);
	/*reset the 512A module*/
	HSMReset();

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (sign_verify_ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		return 1;
	}
	printf("sync success\n");

	/*demo read verison.*/
	ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
	printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);

	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_FactoryNumberRead(result);
	printf("IS32U512A module's ISTECC512A_FactoryNumberRead is  %s\n", result);

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


	for(i=0;i<16;i++)
	{
		/*import private key*/
		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPrikey(i,private_key);
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2ImportPrikey failed!\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2ImportPrikey success!\n");
		}
		/*签名*/
		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignEValue(i,e,rs);
		hex_dump(rs,64,32,"RS:");
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2SignEValue failed\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2SignEValue success!\n");
			sign_verify_right_count++;
		}

		/*improt pubkey */
		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ImportPubkey(i, pubkey,
																					pubkey + 32);
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2ImportPubkey failed!\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2ImportPubkey success!\n");
		}
		/*e value verify*/
		sign_verify_ret = ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,e,rs);
		if (sign_verify_ret)
		{
			printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
			sign_verify_error_count++;
		}
		else
		{
			printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex success!\n");
			sign_verify_right_count++;
		}

		printf("success time is %04d and error count is %04d\n", sign_verify_right_count, sign_verify_error_count);
		
	}


	/*deinit the hardware. spi interface and reset,busy io.release the source*/
	HSMHardwareDeinit();
}
