
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

/*For test the HSM.*/
unsigned long HSMSelfTest(unsigned long test_loop)
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

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (ret)
	{
		printf("pin confirm failed!\n");
		error_count++;
		HSMHardwareDeinit();
		return 1;
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
			HSMHardwareDeinit();
			return 1;
		}
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
	}

	/*set loops of test.*/
	while(test_loop--)
	{
		for(i=0;i<15;i++)
		{
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(i,plain_text,16,cipher_text))
			{
				printf("ISTECC512A_SM2Encrypt failed!\n");
				HSMHardwareDeinit();
				return 1;
			}
			
			memset(temp,0x00,sizeof(temp));
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(i,cipher_text,16+96,temp))
			{
				printf("ISTECC512A_SM2Decrypt failed!\n");
				HSMHardwareDeinit();
				return 1;
			}
			else
			{
				hex_dump(plain_text,16,16,"plain_text:");
				hex_dump(temp,16,16,"result of decrypt:");
				if(memcmp(temp,plain_text,16))
				{
					printf("result of decrypt comapre failed!\n");
					HSMHardwareDeinit();
					return 1;
				}
			}

			/*sign*/
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignEValue(i,hash,rs))
			{
				printf("ISTECC512A_SM2SignEValue failed\n");
				HSMHardwareDeinit();
				return 1;

			}
			hex_dump(rs,64,32,"RS:");
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,hash,rs))
			{
				printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
				HSMHardwareDeinit();
				return 1;
			}
		}
		right_count++;
	}
	printf("TEST END:\n");
	printf("TEST OPTION:\n");
	printf("1.ISTECC512A_SM2Encrypt:\n");
	printf("2.ISTECC512A_SM2Decrypt:\n");
	printf("3.ISTECC512A_SM2SignEValue:\n");
	printf("4.ISTECC512A keyi _SM2VerifyEValueWithPubkeyIndex:\n");
	printf("TEST COUNT:%8d\n",right_count);
	HSMHardwareDeinit();
	return 0;
}



void * thread1(void *arg)
{
    int error_count = 0;
	int right_count = 0;
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int test_loop = 100;
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

	printf(" thread1\n");
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
	

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		pthread_exit("thread1 exit! ISTECC512A_StatusSync  failed \n");
	}
	printf("sync success\n");
	for(i=0;i<1;i++)
	{
		/*demo read verison.
		the verison is  1.8.7  .in this time(2022/6/16)*/
		ret  = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
		if(ret)
		{
			printf("THREAD1 module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
			printf("read verison failed.\n");
			HSMHardwareDeinit();
			pthread_exit("thread1 exit! ISTECC512A_CosVersionRead  failed \n");
		}
		else
		{
			right_count++;
			printf("thread2 right:%8d error %4d\n",right_count,error_count);
		}
	
	}
	
	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (ret)
	{
		printf("pin confirm failed!\n");
		error_count++;
		HSMHardwareDeinit();
		pthread_exit("thread1 exit! ISTECC512A_PinConfirm  failed \n");
	}
	else
	{
		printf("pin confirm success!\n");
	}

	/*Generate 8 group key pairs.*/
	for(i=0;i<8;i++)
	{	
		if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2GenKeyPair(i))
		{
			printf("ISTECC512A_SM2GenKeyPair failed!\n");
			HSMHardwareDeinit();
			pthread_exit("thread1 exit! ISTECC512A_SM2GenKeyPair  failed \n");
		}
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
	}

	/*set loops of test.*/
	while(1)
	{
		for(i=0;i<8;i++)
		{
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(i,plain_text,16,cipher_text))
			{
				printf("ISTECC512A_SM2Encrypt failed!\n");
				HSMHardwareDeinit();
				pthread_exit("thread1 exit! ISTECC512A_SM2Encrypt  failed \n");
			}
			
			memset(temp,0x00,sizeof(temp));
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(i,cipher_text,16+96,temp))
			{
				printf("ISTECC512A_SM2Decrypt failed!\n");
				HSMHardwareDeinit();
				pthread_exit("thread1 exit! ISTECC512A_SM2Decrypt  failed \n");
			}
			else
			{
				hex_dump(plain_text,16,16,"plain_text:");
				hex_dump(temp,16,16,"result of decrypt:");
				if(memcmp(temp,plain_text,16))
				{
					printf("result of decrypt comapre failed!\n");
					HSMHardwareDeinit();
					pthread_exit("thread1 exit! ISTECC512A_SM2Decrypt compare  failed \n");
				}
			}

			/*sign*/
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignEValue(i,hash,rs))
			{
				printf("ISTECC512A_SM2SignEValue failed\n");
				HSMHardwareDeinit();
				pthread_exit("thread1 exit! ISTECC512A_SM2SignEValue  failed \n");
			}
			hex_dump(rs,64,32,"RS:");
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,hash,rs))
			{
				printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
				HSMHardwareDeinit();
				pthread_exit("thread1 exit! ISTECC512A_SM2VerifyEValueWithPubkeyIndex  failed \n");
			}
		}
		right_count++;
	}
	printf("TEST END:\n");
	printf("TEST OPTION:\n");
	printf("1.ISTECC512A_SM2Encrypt:\n");
	printf("2.ISTECC512A_SM2Decrypt:\n");
	printf("3.ISTECC512A_SM2SignEValue:\n");
	printf("4.ISTECC512A keyi _SM2VerifyEValueWithPubkeyIndex:\n");
	printf("TEST COUNT:%8d\n",right_count);
	HSMHardwareDeinit();
	pthread_exit("thread1 exit!\n");

}

void * thread2(void *arg)
{
  int error_count = 0;
	int right_count = 0;
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int test_loop = 100;
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

	printf(" thread2\n");
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		HSMHardwareDeinit();
		pthread_exit("thread2 exit! ISTECC512A_StatusSync  failed \n");
	}
	printf("sync success\n");


	for(i=0;i<1;i++)
	{
		/*demo read verison.
		the verison is  1.8.7  .in this time(2022/6/16)*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
		if(ret)
		{
			printf("THREAD2 module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
			printf("read verison failed.\n");
			error_count += 1;
		}
		else
		{
			right_count++;
			printf("thread2 right:%8d error %4d\n",right_count,error_count);
		}
	}
	

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (ret)
	{
		printf("pin confirm failed!\n");
		error_count++;
		HSMHardwareDeinit();
		pthread_exit("thread2 exit! ISTECC512A_PinConfirm failed \n");
	}
	else
	{
		printf("pin confirm success!\n");
	}

	/*Generate 8 group key pairs.*/
	for(i=8;i<16;i++)
	{	
		if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2GenKeyPair(i))
		{
			printf("ISTECC512A_SM2GenKeyPair failed!\n");
			HSMHardwareDeinit();
			pthread_exit("thread2 exit! ISTECC512A_SM2GenKeyPair failed \n");
		}
		printf("INDEX %d:\n",i);
		ISTECC512AFunctionPointerStructure.ISTECC512A_SM2ExportPubkey(i,sm2_public_key,sm2_public_key+32);
	}

	/*set loops of test. use index 8-14*/
	while(1)
	{
		for(i=8;i<14;i++)
		{
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(i,plain_text,16,cipher_text))
			{
				printf("ISTECC512A_SM2Encrypt failed!\n");
				HSMHardwareDeinit();
				pthread_exit("thread2 exit! ISTECC512A_SM2Encrypt failed \n");
			}
			
			memset(temp,0x00,sizeof(temp));
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(i,cipher_text,16+96,temp))
			{
				printf("ISTECC512A_SM2Decrypt failed!\n");
				HSMHardwareDeinit();
				pthread_exit("thread2 exit! ISTECC512A_SM2Decrypt failed \n");
			}
			else
			{
				hex_dump(plain_text,16,16,"plain_text:");
				hex_dump(temp,16,16,"result of decrypt:");
				if(memcmp(temp,plain_text,16))
				{
					printf("result of decrypt comapre failed!\n");
					HSMHardwareDeinit();
					pthread_exit("thread2 exit! ISTECC512A_SM2Decrypt compare failed \n");
				}
			}

			/*sign*/
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignEValue(i,hash,rs))
			{
				printf("ISTECC512A_SM2SignEValue failed\n");
				HSMHardwareDeinit();
				pthread_exit("thread2 exit! ISTECC512A_SM2SignEValue failed \n");
			}
			hex_dump(rs,64,32,"RS:");
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,hash,rs))
			{
				printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
				HSMHardwareDeinit();
				pthread_exit("thread2 exit! ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed \n");
			}
		}
		right_count++;
	}
	printf("TEST END:\n");
	printf("TEST OPTION:\n");
	printf("1.ISTECC512A_SM2Encrypt:\n");
	printf("2.ISTECC512A_SM2Decrypt:\n");
	printf("3.ISTECC512A_SM2SignEValue:\n");
	printf("4.ISTECC512A keyi _SM2VerifyEValueWithPubkeyIndex:\n");
	printf("TEST COUNT:%8d\n",right_count);
	HSMHardwareDeinit();
	pthread_exit("thread2 exit!\n");
}

unsigned long HSMSelfTestWithMultithreading(void)
{
 	int ret  =  0;
	pthread_t hsm_thread1;
	pthread_t hsm_thread2;
	/*Init the hardware . spi interface  and reset,busy io*/
	/*Thread 1*/
	printf("aaa\n");
	HSMHardwareInit(10000000);
	ret = pthread_create(&hsm_thread1,NULL,thread1,NULL);
	if(ret != 0)
	{
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	/*thread 2*/
	ret = pthread_create(&hsm_thread2,NULL,thread2,NULL);
	if(ret != 0)
	{
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
	
	/*If you want to exit test. input 1234*/
	do{
		scanf("%d",&ret);
	}	while(ret != 1234);

	HSMHardwareDeinit();
	return 0;	
}





unsigned long HSMSelfTestWithMulProcess(void)
{

	int error_count = 0;
	int right_count = 0;
	int ret;
	unsigned int i;
	int len;
	int message_len = 0;
	unsigned char default_pin[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int test_loop = 100;
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

		/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(10000000);
	/*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);

	
	/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
	if (ret)
	{
		printf("sync failed\n");
		return 1;
	}
	printf("sync success\n");

	for(i=0;i<1;i++)
	{
		/*demo read verison.
		the verison is  1.8.7  .in this time(2022/6/16)*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
		if(ret)
		{
			printf("HSMSelfTestWithMulProcess module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
			printf("read verison failed.\n");
			HSMHardwareDeinit();
			return 1;
		}
		else
		{
			right_count++;
			printf("ISTECC512A_CosVersionRead right:%8d error %4d\n",right_count,error_count);
		}
	}
	

	/*pin confirm*/
	printf("pin is 8 byte password.!\n");
	ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
	if (ret)
	{
		printf("pin confirm failed!\n");
		error_count++;
		HSMHardwareDeinit();
		return 1;
	}
	else
	{
		printf("pin confirm success!\n");
	}

	/*set loops of test.*/
	while(1)
	{
		for(i=14;i<16;i++)
		{
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Encrypt(i,plain_text,16,cipher_text))
			{
				printf("ISTECC512A_SM2Encrypt failed!\n");
				HSMHardwareDeinit();
				return 1;
			}
			
			memset(temp,0x00,sizeof(temp));
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2Decrypt(i,cipher_text,16+96,temp))
			{
				printf("ISTECC512A_SM2Decrypt failed!\n");
				HSMHardwareDeinit();
				return 1;
			}
			else
			{
				hex_dump(plain_text,16,16,"plain_text:");
				hex_dump(temp,16,16,"result of decrypt:");
				if(memcmp(temp,plain_text,16))
				{
					printf("result of decrypt comapre failed!\n");
					HSMHardwareDeinit();
					return 1;
				}
			}

			/*sign*/
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2SignEValue(i,hash,rs))
			{
				printf("ISTECC512A_SM2SignEValue failed\n");
				HSMHardwareDeinit();
				return 1;

			}
			hex_dump(rs,64,32,"RS:");
			if(ISTECC512AFunctionPointerStructure.ISTECC512A_SM2VerifyEValueWithPubkeyIndex(i,hash,rs))
			{
				printf("ISTECC512A_SM2VerifyEValueWithPubkeyIndex failed\n");
				HSMHardwareDeinit();
				return 1;
			}
		}
		right_count++;
	}
	printf("TEST END:\n");
	printf("TEST OPTION:\n");
	printf("1.ISTECC512A_SM2Encrypt:\n");
	printf("2.ISTECC512A_SM2Decrypt:\n");
	printf("3.ISTECC512A_SM2SignEValue:\n");
	printf("4.ISTECC512A keyi _SM2VerifyEValueWithPubkeyIndex:\n");
	printf("TEST COUNT:%8d\n",right_count);
	HSMHardwareDeinit();
	return 0;
}