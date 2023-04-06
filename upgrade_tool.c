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

#include "hsm_test_task.h"
#include "script_deal.h"
#define STEP1_CHECK_VERISON  		0X01
#define STEP2_ERASE_HSM_APP  		0X02
#define STEP3_DOWNLOAD_NEW_HSM_APP  0X03
#define STEP4_CHECK_VERISON_AGAIN  	0X04


#define CHECK_VERISON_FAILED        0X01
#define ERASE_HSM_APP_FAILED        0X02
#define DOWNLOAD_NEW_APP_FAILED     0X03
#define CHECK_VERISON_AGAIN_FAILED  0X04

/*
THE FLOW DESCRIPTOR:
1. CHECK THE HSM VERISON.
   IF CHECK SUCCESS .YOU WILL GET THE HSM APP VERSION.  IF YOU WANT TO COMPARE THE VERISON. YOU CAN DO IT HERE.
   IF CHECK FAILED.  MAYBE HAS BEEN ERASE THE HSM APP. JUMP TO STEP3.
2. ERASE THE CURRUENT APP.
   
3. REFLASH THE NEW VERISON APP.
    
4.CHECK THE CURRENT HSM VERISON.
  IF CHECK SUCCESS. SHOW THAT THE VEW VERISON HSM APP DOWNLOAD SUCCESS.COMPARE THE VERSION NUMBER TO CHECK IT.
  CONGRATULATIONS.
*------------------------------------------------------------------------------------------------------------
V0.0.2 .
1.Configurable busy and reset. Will be passed as a parameter.
2.The upgrade file is passed to the program as a separate file.
so the paremter is 
paremeter0:.exe_name
paremeter1:.device name
paremeter2:.gpio_reset number
paremeter3:.gpio_busy  number
paremeter4:.upgrade file name (xxxx.ini)
for example:  

 ./SKF-IMX8-IS32U512A-UPGRADE-TOOL_test  /dev/spidev32766.0   98 96  DownloadFile18906.ini
*/
/*------------------------------------------------------------------------------------------------------------
V0.0.6 .
1.Configurable busy and reset. Will be passed as a parameter.
2.The upgrade file is passed to the program as a separate file.
so the paremter is 
paremeter0:.exe_name
paremeter1:.device name
paremeter2:.gpio_reset number
paremeter3:.gpio_busy  number
paremeter4:.upgrade file name (xxxx.ini)
paremeter5:.SPI-Frequency(I recommend the frequency is 1M~4M).

for example:  
 ./HSM_UPGRADE-TOOL_test   /dev/spidev32766.0   98 96  DownloadFile18906.ini   2000000

*/


/*Print log with English*/
static void print_usage_English(void)
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
	printf("VERSION : 0.0.6\n");
	printf("TIME:  2023-4-6\n");
}



char SPI_DEV_NAME[100];
int busy;
int reset;
unsigned int spi_frequency;

int main(int argc, char *argv[])
{
	int ret = 0;
	int time = 0;

	int step = 0;

	print_usage_English();

	memcpy(SPI_DEV_NAME,argv[1],strlen(argv[1]));
	printf("SPI_DEV_NAME IS%s\n",SPI_DEV_NAME);
	printf("PAR0: %s\n",argv[0]);
	printf("PAR1: %s\n",argv[1]);
	printf("PAR2: %s\n",argv[2]);
	printf("PAR3: %s\n",argv[3]);
	printf("PAR4: %s\n",argv[4]);
	printf("PAR5: %s\n",argv[5]);
	if (argc != 6)
	{
		printf("Abnormal parameter Number!\n");
		return 1;
	}
	busy = atoi(argv[2]);
	reset = atoi(argv[3]);
	spi_frequency = atoi(argv[5]);
	printf("your spi_frequency is %4d\n",spi_frequency);
	if((spi_frequency >= 20000000) ||(spi_frequency <= 500000))
	{
		printf("Please input right spi_frequency!\n");
		return  2;
	}
	/*STEP1.CHECK VERSION*/
	step = STEP1_CHECK_VERISON;
	if(step == STEP1_CHECK_VERISON)
	{	
		printf("CURRENT STEP :%4d\n",step);
		ret = IS32U512AReadVerisonTest();
		if(ret)
		{
			printf("THE VERISON READ FAILED,MAYBE THE APP MAYBE HAS BEEN ERASE.SO WE WILL EXE STEP3\n");
			printf("TRY TO UPGRADE\n");
			step = STEP3_DOWNLOAD_NEW_HSM_APP;
		}
		else{
			printf("READ APP VERISON SUCCESS\n");
			step = STEP2_ERASE_HSM_APP;
		}
	}


	
	/*STEP2 ERASE CURRENT HSM APP  */
	if(step == STEP2_ERASE_HSM_APP)
	{
		printf("CURRENT STEP :%4d\n",step);
		ret = ISTECCEraseAPPTest2();
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

		ret = script_analysis(argv[4],1);
		//ret = ISTECCUpdateTest();
		if(ret)
		{
			printf("TRY TO DOWNLOAD NEW HSM APP FAILED ,PLEASE CHECK THE COMMUCAITON\n");
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
		ret = IS32U512AReadVerisonTest();
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
  
