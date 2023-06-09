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
#define STEP1_CHECK_VERISON  		0X01
#define STEP2_ERASE_HSM_APP  		0X02
#define STEP3_DOWNLOAD_NEW_HSM_APP  0X03
#define STEP4_CHECK_VERISON_AGAIN  	0X04


#define CHECK_VERISON_FAILED        0X01
#define ERASE_HSM_APP_FAILED        0X02
#define DOWNLOAD_NEW_APP_FAILED     0X03
#define CHECK_VERISON_AGAIN_FAILED  0X04




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
	printf("\n");
	printf("Please note:\n1.Erase or after download file sucess,you need RESET the HSM. or POWER ON AGAIN\n");
	printf("2.I recommend the frequency is 1M~4M,Not too slow,not too fast.\n");	
	printf("VERSION : 0.0.7\n");
	printf("TIME:  2023-4-11\n");
}


extern char SPI_DEV_NAME[100];
extern int busy;
extern int reset;


unsigned long HSMUpgradeTest(int argc, char *argv[])
{

    int  ret;
    int time;
    int sem_status;
    int step = 0;
	unsigned int spi_frequency;
    busy = atoi(argv[2]);
    reset = atoi(argv[3]);
    spi_frequency = atoi(argv[5]);
	char default_pin[8] = {1,2,3,4,5,6,7,8};
	char version[3];
    /*Create a pointer struct and Init it. */
	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
	
	printf("your spi_frequency is %4d\n",spi_frequency);
    if((spi_frequency >= 20000000) ||(spi_frequency <= 500000))
    {
        printf("Please input right spi_frequency!\n");

        return  2;
    }

	print_upgrade_message();
	/*Init the hardware . spi interface  and reset,busy io*/
	HSMHardwareInit(spi_frequency);
	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
	

	step = STEP1_CHECK_VERISON;
	/*ALL OF THE UPGRADE FLOW,CAN'T BE BKEAK */
	if(step == STEP1_CHECK_VERISON)
	{	
		printf("CURRENT STEP :%4d ,READ HSM VERISON\n",step);
		/*How to use sync ?if you has reset the module. you don't need sync. the default state of HSM module is receive instuction*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
		if (ret)
		{
			printf("THE SYNCHRONISM FAILED,MAYBE THE APP MAYBE HAS BEEN ERASE.SO WE WILL EXE STEP3\n");
			printf("TRY TO UPGRADE\n");
			step = STEP3_DOWNLOAD_NEW_HSM_APP;
		}
		else
		{
			printf("sync success\n");
			/*demo read verison.
			the verison is  1.8.7  .in this time(2022/6/16)*/
			ret = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
			printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
			if(ret)
			{
				printf("THE VERISON READ FAILED,MAYBE THE APP MAYBE HAS BEEN ERASE.SO WE WILL EXE STEP3\n");
				printf("TRY TO UPGRADE\n");
				step = STEP3_DOWNLOAD_NEW_HSM_APP;
			}
			else
			{
				printf("READ APP VERISON SUCCESS\n");
				step = STEP2_ERASE_HSM_APP;
			}

		}
		
	}

	/*STEP2 ERASE CURRENT HSM APP  */
	if(step == STEP2_ERASE_HSM_APP)
	{
		printf("CURRENT STEP :%4d\n",step);
		/*pin confirm*/
		printf("pin is 8 byte password.!\n");
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_PinConfirm(default_pin, 8);
		if (ret)
		{
			printf("pin confirm failed!\n");

			return 1;
		}
		else
		{
			printf("pin confirm success!\n");
		}

		/*Erase APP*/
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_APPErase();
		if(ret)
		{
			printf("TRY TO ERASE HSM APP FAILED.PLEASE CHECK THE COMMUNACATION OR PIN \n");

			return ERASE_HSM_APP_FAILED;
		}
		else{
			printf("ERASE HSM APP SUCCESS.PLEASE POWER BACK OR RESET HSM MODULE!\n");
			printf("THE NEXT STEP WILL DOWNLOAD NEW HSM APP\n");
			step = DOWNLOAD_NEW_APP_FAILED;
		}
	}
	

	/*STEP3 DOWNLOAD NEW HSM APP*/
	if(step == DOWNLOAD_NEW_APP_FAILED)
	{

		printf("CURRENT STEP :%4d\n",step);
		printf("NOTE: PLEASE RESET HSM MODULE\n");
		scanf("INPUT ANY NUM TO CONTINUE%d",ret);	

		/*NOTE : THE UPGRADE OF HSM CAN'T BE BREAK. SO ADD HSMPSemphre*/
		HSMPSemphre();
		ret = script_analysis(argv[4],1);
		HSMVSemphre();

		if(ret)
		{
			
			printf("TRY TO DOWNLOAD NEW HSM APP FAILED ,PLEASE CHECK THE COMMUCAITON,THEN WILL RETURN\n");
			return DOWNLOAD_NEW_APP_FAILED;
		}
		else{
			printf("DOWNLOAD NEW HSM APP SUCCESS!\n");
			printf("THE NEXT STEP WILL CHECK NEW VERISON\n");
			step = STEP4_CHECK_VERISON_AGAIN;
		}
	}

	if(step == STEP4_CHECK_VERISON_AGAIN)
	{
		printf("CURRENT STEP :%4d\n",step);

		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_StatusSync();
		if (ret)
		{
			printf("SYNC FAILED.MAYBE THE COMMUNICATION HAVE SOME WRONG\n");
			return DOWNLOAD_NEW_APP_FAILED;
		}
		ret = ISTECC512AFunctionPointerStructure.ISTECC512A_CosVersionRead(version);
		printf("IS32U512A module's verison is  %2d %2d %2d %2d\n", version[0], version[1], version[2],version[3]);
		if(ret)
		{
			printf("THE VERISON READ FAILED,PLEASE CHECK THE COMMUNACATION.\n");
			printf("TRY TO UPGRADE\n");
			return CHECK_VERISON_AGAIN_FAILED;
		}
		else{
			printf("READ APP VERISON SUCCESS\n");
			printf("UPGRADE SUCCESS . CONGRATULATIONS!\n");
			return 0 ;
		}
	}
}
