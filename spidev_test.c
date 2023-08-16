/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
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

#include "hsm_self_test.h"
#include "hsm_test_task.h"
#include "hsm_upgrade_test.h"
#include "hsm_one_key_restore_test.h"

/*TEST MODE*/
#define READ_VERISON   			0x00
#define ALL_FUN_TEST    		0X01
#define APP_ERASE       		0X02
#define APP_UPDATE      		0X03
#define POINTER_DECOMPRESS 		0X04
#define KEY_DERIVE              0X05	
#define SM4_CCM		            0X06
#define SM2_ENCRYPT_DECRYPT     0X07
#define SM4_STRESS     			0X08
#define SM2_VERIFY_ONE_CORE     0X09  /*original verify*/
#define SM2_VERIFY_E_RS			0x0A  /*10 e value sign*/
#define SM2_SIGN_E_VALUE		0x0B  /*11 E value sign*/
#define SELF_TEST				0X0C  /*12*/
#define SELF_TEST_WITH_MUTEX	0X0D  //13  TEST MUL PTHREAD.
#define SELF_TEST_MUL_PROCESS	0X0E  //14  TEST MUL PROCESS.
#define HSM_ONE_KEY_RESOTRE		0X0F  //15  TEST ONE KEY ERSTORE.
#define HSM_MODADD_TEST			0X10  //16  TEST MOD ADD

#define HSM_EXCEPTION_TEST		0X70  //0X70 测试抛出异常的情况 112
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static void pabort(const char *s)
{
	perror(s);
	abort();
}

extern const unsigned char UserID00_UserInfo[];
/*Print log with English*/
static void print_usage_English(void)
{
	printf("  istecc's is32u512a test code for linux\n");
	printf("  author: Qinxd\n");
	printf("  func 0: Read moudle verison(maybe 1.8.7)\n");
	printf("  func 1: All function test\n");
	printf("  func 4: Pointer Descompress\n");
	printf("  func 10: E+RS verify mode\n");
	printf("  func 11: HSM E sign test\n");
	printf("  func 12: HSM Self test: SIGN-VERIFY-ENCODE-DECODE\n");
	printf("  func 13: SELF TEST WITH MUTEX:MUL PHTREAD\n");
	printf("  func 14: HSM MUL PROCESS TEST ,YOU NEED FIRST RUN \n");
	printf("  func 15: HSM TEST ONE MESSAGE RESTORE ALL THE KEY PAIRS\n");
	printf("  func 16: MOD ADD TEST\n");
	printf("  input parameter with the test number\n");
}




extern char SPI_DEV_NAME[100];
extern int busy;
extern int reset;
extern unsigned int spi_frequency;

int main(int argc, char *argv[])
{
	int ret = 0;
	int time = 0;
	int test_loop = 100;
	int sem_status = 0;
	int step = 0;
	int x = 100;

	/*if you need upgrade .you will pass 6 parameters*/
	if (argc == 6)
	{
		memcpy(SPI_DEV_NAME,argv[1],strlen(argv[1]));
		printf("SPI_DEV_NAME IS%s\n",SPI_DEV_NAME);
		printf("PAR0: %s\n",argv[0]);
		printf("PAR1: %s\n",argv[1]);
		printf("PAR2: %s\n",argv[2]);
		printf("PAR3: %s\n",argv[3]);
		printf("PAR4: %s\n",argv[4]);
		printf("PAR5: %s\n",argv[5]);

		HSMUpgradeTest(argc, argv);
		return 0;
	}
	else if (argc == 2)
	{
		x = atoi(argv[1]);
		print_usage_English();
		switch (x)
		{
		case READ_VERISON:
			IS32U512AReadVerisonTest();
			break;
		case ALL_FUN_TEST:
			IS32U512AFunctionTest();
			break;
		case POINTER_DECOMPRESS:
			printf("ISTECCSPointerDecompressTest TEST.");
			ISTECCSPointerDecompressTest();
			break;
		case SM2_ENCRYPT_DECRYPT:
			IS32U512ASM2EncyprtDecryptTest();
			break;
		case SM2_VERIFY_E_RS:
			IS32U512ASm2VerifyEvalueWithPubKeyIndex();
			break;
		case SM2_SIGN_E_VALUE:
			IS32U512ASm2SignEvalueAndVerifyEvalueWithPubKeyIndex();	
			break;
		case SELF_TEST:
			if(HSMSelfTest(test_loop))
			{
				printf("HSMSelfTest failed!\n");
			}
			break;
		case SELF_TEST_WITH_MUTEX:
			printf("1122233\n");
			HSMSelfTestWithMultithreading();
			break;
		case SELF_TEST_MUL_PROCESS:
			HSMSelfTestWithMulProcess();
			break;
		case HSM_ONE_KEY_RESOTRE:
			if(HSMOneKeyRestore())
			{
				printf("HSMOneKeyRestore failed!\n");
			}
			break;
		case HSM_MODADD_TEST:
			ISTECCMODADDTest();
			break;
		case HSM_EXCEPTION_TEST:
			EXCEPTIONTest();
			break;
		default:
			printf("not support!\n");
			break;
		}
	}
	else
	{
		printf("Pra error !\n");
		return 1;
	}
	
	printf("TestTask end:\n");
	return 0;
}
