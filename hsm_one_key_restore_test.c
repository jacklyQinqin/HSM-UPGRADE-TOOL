
#include "hsm_hardware_level.h"
#include "hsm_logic_level.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

//提供计时操作,用来统计算法效率
#include <sys/time.h>

//用来提供malloc支持。数据存储和读取使用
#include <stdlib.h>
#include <stdio.h>
//用来提供产生随机数的函数
#include <time.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>



unsigned long HSMOneKeyRestore(void)
{

    int error_count = 0;
	int right_count = 0;
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    
	unsigned char message[1024] ;
	unsigned char temp[1024];
    unsigned char plain_text[16] ={0X00,0X11,0X22,0X33,0X44,0X55,0X66,0X77,0X88,0X99,0XAA,0XBB,0XCC,0XDD,0XEE,0XFF};
	unsigned char cipher_text[96+16];
	unsigned char sm2_private_key[32];
	unsigned char sm2_public_key[64];
	unsigned char version[4];
	unsigned char rs[64];
	unsigned char hash[32] = {
		0X00,0X11,0X22,0X33,0X44,0X55,0X66,0X77,0X88,0X99,0XAA,0XBB,0XCC,0XDD,0XEE,0XFF,
		0X00,0X11,0X22,0X33,0X44,0X55,0X66,0X77,0X88,0X99,0XAA,0XBB,0XCC,0XDD,0XEE,0XFF,
	};
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(10000000);
	HSMThreadMutexInit();
	/*初始化信号量*/
	HSMSempohreInit();
	HSMSetSemphre();
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
	the verison is  1.8.7  .in this time(2022/6/16)
    Now the new verison is 1.8.9.8(2023.5.9)
    */
	ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
	printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (ret)
	{
		printf("pin confirm failed!\n");
		error_count++;
		return (1);
	}
	else
	{
		printf("pin confirm success!\n");
	}

	/*Generate 16 group key pairs.*/
	for(i=0;i<15;i++)
	{	
		if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2GenKeyPair(i))
		{
			printf("ISTECC512A_SM2GenKeyPair failed!\n");
			//HSMHardwareDeinit();
			return (1);
		}
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(i,sm2_private_key);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
	}

    /*export the 16 group key pairs.*/
    for(i=0;i<15;i++)
	{	
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(i,sm2_private_key);
        hex_dump(sm2_private_key,32,32,"PRIKEY:");
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
        hex_dump(sm2_public_key,32,64,"PUBKEY:");
	}

    /*one key to restore.*/
    if(ISTECC512AFunctionPointerStructure.ISTECC512A_Restore())
    {
        printf("ISTECC512A_Restore failed!\n");
        return 1;
    }
    /*export the 16 group key pairs.*/
    for(i=0;i<15;i++)
	{	
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPrikey(i,sm2_private_key);
        hex_dump(sm2_private_key,32,32,"PRIKEY:");
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
        hex_dump(sm2_public_key,32,64,"PUBKEY:");
	}

	printf("TEST END:\n");
	HSMHardwareDeinit();
	return 0;
}