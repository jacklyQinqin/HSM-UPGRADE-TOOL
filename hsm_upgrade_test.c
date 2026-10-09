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

/*工厂码状态定义：只有02态才需要走测试态流程，03/04/05态直接下载bootloader*/
#define FACTORY_CODE_STATE_TEST				0X02
#define FACTORY_CODE_STATE_03				0X03
#define FACTORY_CODE_STATE_04				0X04
#define FACTORY_CODE_STATE_05				0X05

/*读工厂码的SPI时序参数*/
#define FACTORY_CODE_CMD_LEN				16		/*读工厂码指令长度*/
#define FACTORY_CODE_DUMMY_LEN				16		/*dummy 接收长度*/
#define FACTORY_CODE_PACKET_LEN				16		/*每包数据长度：发送/接收统一按16字节分包*/
#define FACTORY_CODE_RESPOND_LEN			32		/*响应缓冲长度：状态在 offset 25，故至少需 26 字节，取 32 对齐=2包*/
#define FACTORY_CODE_RESPOND_PACKETS		(FACTORY_CODE_RESPOND_LEN / FACTORY_CODE_PACKET_LEN)	/*响应包数=2*/
#define FACTORY_CODE_STATE_INDEX			25		/*状态字节在响应中的偏移*/
#define FACTORY_CODE_DELAY_MS				5		/*固定延时5ms*/

/*------------------------- 测试态(02态)流程参数 -------------------------*/
/*测试态指令发送缓冲区长度：必须是 16 的整数倍（SendOneMessage 按 16 字节分包），
  不足部分用 0xFF 补齐，与 spiReadFactoryCmd 的长度字段+填充方式一致*/
#define TEST_MODE_CMD1_BUF_LEN				32		/*test_1：有效22字节(BF45鉴别)，补10字节0xFF*/
#define TEST_MODE_CMD2_BUF_LEN				16		/*test_2：有效14字节(BFEE FT)，补2字节0xFF*/
#define TEST_MODE_CMD3_BUF_LEN				16		/*test_3：有效14字节(BFEE退测试态)，补2字节0xFF*/

/*测试态收发长度*/
#define TEST_MODE_PACKET_LEN				16		/*SPI 单包长度*/
#define TEST_MODE_DUMMY_LEN					16		/*dummy 接收长度(1包)*/
#define TEST_MODE_RESPOND_LEN				16		/*响应接收长度(1包，判据用到偏移8~11)*/
#define TEST_MODE_RESPOND_HDR_LEN			8		/*响应头长度：50 42 53 55 + 4字节长度，状态字段自偏移8起*/

/*Delay calibration:the MCU side waits with busy loops
  (for(i=0;i<N;i++),1 loop about 30ns).The DELAY_AFTER macros
  below are the converted values(rounded UP to ms,minimum 1ms).
  Calibrate by changing THESE ms values only.*/
#define TEST_MODE_DELAY_AFTER_CMD1_MS		10UL	
#define TEST_MODE_DELAY_AFTER_DUMMY1_MS		10UL			
#define TEST_MODE_DELAY_AFTER_CMD2_MS		5000UL		
#define TEST_MODE_DELAY_AFTER_DUMMY2_MS		60UL		
#define TEST_MODE_DELAY_AFTER_CMD3_MS		10UL		
#define TEST_MODE_DELAY_AFTER_DUMMY3_MS		10UL			
#define TEST_MODE_RESET_WAIT_MS				1000UL

/*
  除非确认时能了HSM-RESET功能。否则TEST_MODE_INCLUDE_DOWNLOAD确保为0.如果不确认此功能。请保持为0.
  Only if the HSM-RESET function is confirmed, TEST_MODE_INCLUDE_DOWNLOAD can be set to 1. 
  If this function is not confirmed, please keep it at 0.
 */
#define TEST_MODE_INCLUDE_DOWNLOAD			0

/*工厂码读取与测试态流程的返回码*/
#define READ_FACTORY_CODE_SUCCESS			0X00
#define READ_FACTORY_CODE_FAILED			0X01
#define TEST_MODE_PROCESS_SUCCESS			0X00
#define TEST_MODE_PROCESS_FAILED			0X01

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

/****************************************************************\
* Function:			FactoryCodeDelay
*
* Description: 		读工厂码时用的固定延时，每次 5ms。
*
* Input:
*					None
*
* Return:
*					None
\****************************************************************/
static void FactoryCodeDelay(void)
{
	HSMMsDelay(FACTORY_CODE_DELAY_MS);
}

/****************************************************************\
* Function:			ReadFactoryCodeState
*
* Description: 		读取工厂码，并从中取出当前状态字节。
*					芯片处于 ROM boot 态时，用此接口判断是否需要先走测试态流程。
*
* Calls:
*					ISTECC512A_SendOneMessage		- 分包发送(每包16字节)
*					ISTECC512A_ReceiveOneMessage	- 分包接收(每包16字节)
*
* Input:
*					pFunc	- 已 Init 的逻辑层函数指针结构体
*					state	- 输出参数，工厂码中的状态字节(02/03/04/05...)
*
* Return:
*					0X00(READ_FACTORY_CODE_SUCCESS) ：读取成功
*					0X01(READ_FACTORY_CODE_FAILED)  ：读取失败
*
* Others:
*					时序对齐 MCU 侧实现：
*					  1. 发 16 字节读工厂码指令(1包)
*					  2. 固定延时 5ms
*					  3. 收 16 字节 dummy(1包)
*					  4. 固定延时 5ms
*					  5. 显式按 16 字节分包收响应(2包)，状态取 receive_data[25]
\****************************************************************/
static unsigned long ReadFactoryCodeState(ISTECCFunctionPointer_t *pFunc,
                                          unsigned char *state)
{
	/*读工厂码指令：40 42 53 55 | 0E 00 00 00 | BF 48 00 00 12 EF | FF FF */
	const unsigned char spiReadFactoryCmd[FACTORY_CODE_CMD_LEN] = {
		0x40, 0x42, 0x53, 0x55, 0x0E, 0x00, 0x00, 0x00,
		0xBF, 0x48, 0x00, 0x00, 0x12, 0xEF, 0xFF, 0xFF
	};
	unsigned char spi_read_dummy[FACTORY_CODE_DUMMY_LEN];
	unsigned char receive_data[FACTORY_CODE_RESPOND_LEN];
	unsigned long ret;
	int i;
	unsigned char temp_status;
	unsigned long respond_len;

	memset(spi_read_dummy, 0x00, sizeof(spi_read_dummy));
	memset(receive_data, 0x00, sizeof(receive_data));

	/*Step 1 —— 发送读工厂码指令：16字节，1包（内部每包后 HSMMsDelay(2)）*/
	ret = pFunc->ISTECC512A_SendOneMessage((unsigned char *)spiReadFactoryCmd,
	                                       FACTORY_CODE_CMD_LEN);
	if (ret)
	{
		printf("READ FACTORY CODE FAILED: SEND CMD\n");
		return READ_FACTORY_CODE_FAILED;
	}
	FactoryCodeDelay();

	/*Step 2 —— 接收 16 字节 dummy，用于对齐时序：1包*/
	ret = pFunc->ISTECC512A_ReceiveOneMessage(spi_read_dummy,
	                                          FACTORY_CODE_DUMMY_LEN);
	if (ret)
	{
		printf("READ FACTORY CODE FAILED: RECEIVE DUMMY\n");
		return READ_FACTORY_CODE_FAILED;
	}
	FactoryCodeDelay();

	/*Step 3 —— 显式按 16 字节分包接收响应：共 2 包（状态在 offset 25，落在第2包内）*/
	for (i = 0; i < FACTORY_CODE_RESPOND_PACKETS; i++)
	{
		ret = pFunc->ISTECC512A_ReceiveOneMessage(
		          receive_data + i * FACTORY_CODE_PACKET_LEN,
		          FACTORY_CODE_PACKET_LEN);
		if (ret)
		{
			printf("READ FACTORY CODE FAILED: RECEIVE RESPOND (PACKET %d)\n", i);
			return READ_FACTORY_CODE_FAILED;
		}
		FactoryCodeDelay();
	}

	hex_dump(receive_data, FACTORY_CODE_RESPOND_LEN, 16, "ReadFactoryCodeState rx:");

	/*FIX:check the respond frame header and length field BEFORE
	  trusting the state byte.The SPI respond header must be
	  50 42 53 55(see script_deal.h).If the chip is NOT in ROM boot
	  state,the MISO line is read back as 0xFF garbage,without this
	  check the garbage would be accepted as a valid state read.*/
	if ((0x50 != receive_data[0]) || (0x42 != receive_data[1])
	 || (0x53 != receive_data[2]) || (0x55 != receive_data[3]))
	{
		printf("READ FACTORY CODE FAILED: WRONG RESPOND HEADER\n");
		return READ_FACTORY_CODE_FAILED;
	}
	respond_len = receive_data[4] + ((unsigned long)receive_data[5] << 8);
	if (respond_len < (FACTORY_CODE_STATE_INDEX + 1))
	{
		printf("READ FACTORY CODE FAILED: WRONG RESPOND LEN %lu\n", respond_len);
		return READ_FACTORY_CODE_FAILED;
	}

	/*状态字节*/
	temp_status = receive_data[FACTORY_CODE_STATE_INDEX];
	printf("FACTORY CODE STATE :%02X\n", temp_status);

	*state = temp_status;
	return READ_FACTORY_CODE_SUCCESS;
}


/****************************************************************\
* Function:			IsDownloadState
*
* Description: 		判断芯片状态是否属于「下载态」(03/04/05)。
*					对应 MCU 侧 ((tempstatus==03)||(tempstatus==04)||(tempstatus==05))。
*
* Input:
*					state	- 工厂码状态字节
*
* Return:
*					1 ：是下载态
*					0 ：不是下载态
\****************************************************************/
static int IsDownloadState(unsigned char state)
{
	if ((state == FACTORY_CODE_STATE_03) ||
	    (state == FACTORY_CODE_STATE_04) ||
	    (state == FACTORY_CODE_STATE_05))
	{
		return 1;
	}

	return 0;
}

/****************************************************************\
* Function:			TestModeExchange
*
* Description: 		One send/receive exchange of test mode,
*							equal to the MCU side:
*							SPI2SendCommand(cmd);
*							wait send_delay_ms;
*							SPI2ReceiveData(spiReadDummy,16);
*							wait recv_delay_ms;
*							SPI2ReceiveRespond(receive_data);
*
* Input:
*							pFunc			- function pointer struct from Init
*							cmd				- command buffer(padded to 16-byte multiple)
*							cmd_buf_len		- send buffer length
*							send_delay_ms		- fixed delay after the command is sent
*							recv_delay_ms		- fixed delay after the dummy is received
*							dummy				- dummy receive buffer,16 bytes
*							respond				- respond receive buffer,16 bytes
*
* Return:
*							0X00(TEST_MODE_PROCESS_SUCCESS) exchange success
*							0X01(TEST_MODE_PROCESS_FAILED)  exchange failed
\****************************************************************/
static unsigned long TestModeExchange(ISTECCFunctionPointer_t *pFunc,
                                      const unsigned char *cmd,
                                      unsigned long cmd_buf_len,
                                      unsigned long send_delay_ms,
                                      unsigned long recv_delay_ms,
                                      unsigned char *dummy,
                                      unsigned char *respond)
{
	unsigned long ret;
	unsigned long offset;

	/*发送长度必须按16字节对齐，每次调用只发送一个数据包*/
	/*The command length must be aligned to 16 bytes, and each call sends only one data packet*/
	if (cmd_buf_len % TEST_MODE_PACKET_LEN != 0)
	{
		printf("TEST MODE: INVALID COMMAND LENGTH %lu\n", cmd_buf_len);
		return TEST_MODE_PROCESS_FAILED;
	}
	for (offset = 0; offset < cmd_buf_len; offset += TEST_MODE_PACKET_LEN)
	{
		ret = pFunc->ISTECC512A_SendOneMessage(
		          (unsigned char *)(cmd + offset), TEST_MODE_PACKET_LEN);
		if (ret)
		{
			printf("TEST MODE: SEND COMMAND FAILED (PACKET %lu)\n",
			       offset / TEST_MODE_PACKET_LEN);
			return TEST_MODE_PROCESS_FAILED;
		}

		HSMMsDelay(5);
	}
	HSMMsDelay(send_delay_ms);

	/*收 dummy：16字节，1包*/
	ret = pFunc->ISTECC512A_ReceiveOneMessage(dummy, TEST_MODE_DUMMY_LEN);
	if (ret)
	{
		printf("TEST MODE: RECEIVE DUMMY FAILED\n");
		return TEST_MODE_PROCESS_FAILED;
	}
	HSMMsDelay(recv_delay_ms);

	/*收响应：16字节，1包*/
	ret = pFunc->ISTECC512A_ReceiveOneMessage(respond, TEST_MODE_RESPOND_LEN);
	if (ret)
	{
		printf("TEST MODE: RECEIVE RESPOND FAILED\n");
		return TEST_MODE_PROCESS_FAILED;
	}
	
	hex_dump(respond, TEST_MODE_RESPOND_LEN, 16, "TestMode respond:");

	return TEST_MODE_PROCESS_SUCCESS;
}

/****************************************************************\
* Function:			HSMTestModeProcess
*
* Description: 		测试态处理流程，对应 MCU 侧 02 态分支。
*					进入条件：工厂码状态为 02（已由调用方 HSMUpgradeTest() 判定）。
*					流程：
*					  1. 下发 test_1(鉴别指令 BF 45) -> 校验响应 90 00
*					  2. 下发 test_2(FT指令 BF EE)   -> 校验响应 0F 0F 90 00
*					  3. 下发 test_3(退出测试态 BF EE)-> 校验响应 90 00
*					  4. 硬件复位芯片，复位后应进入下载态(03/04/05)
*
* Input:
*					pFunc	- 已 Init 的逻辑层函数指针结构体
*
* Return:
*					0X00(TEST_MODE_PROCESS_SUCCESS) ：测试态流程执行成功
*					0X01(TEST_MODE_PROCESS_FAILED)  ：测试态流程执行失败
*
* Others:
*					任何一步校验失败都直接返回 TEST_MODE_PROCESS_FAILED，
*					对应 MCU 侧的 ExceptionAndErrorHandler()。
*					下载 bootloader 由外层流程在返回后执行，详见 TEST_MODE_INCLUDE_DOWNLOAD。
\****************************************************************/
/****************************************************************\
* Function:			HSMTestModeProcess
*
* Description: 		test mode process, corresponding to the MCU side 02 state branch.
*					input condition: factory code state is 02 (already determined by the caller HSMUpgradeTest()).	
*					flow:
*					  1. Send test_1 (authentication command BF 45) -> check response 90 00	
*					  2. Send test_2 (FT command BF EE) -> check response 0F 0F 90 00
*					  3. Send test_3 (exit test mode BF EE) -> check response 90 00
*					  4. Hardware reset the chip, after reset should enter download state (03/04/05)
*
* Input:
*					pFunc	- initialized logic layer function pointer structure
*
* Return:
*					0X00(TEST_MODE_PROCESS_SUCCESS) ：test mode process executed successfully
*					0X01(TEST_MODE_PROCESS_FAILED)  ：test mode process execution failed
*
* Others:
*					Any step validation failure will directly return TEST_MODE_PROCESS_FAILED,
*					corresponding to the ExceptionAndErrorHandler() on the MCU side.
*					Download bootloader is executed by the outer process after returning, see TEST_MODE_INCLUDE_DOWNLOAD for details.
\****************************************************************/
static unsigned long HSMTestModeProcess(ISTECCFunctionPointer_t *pFunc)
{
	/*测试态指令（缓冲区长度须为16的整数倍，有效帧后用0xFF补齐）*/
	/*Test mode commands (buffer length must be a multiple of 16, valid frames padded with 0xFF)*/
	/*test_1：40 42 53 55 | 16 00 00 00 | BF 45 02 00 08 34 61 73 18 45 04 57 02 C8 | FF×10*/
	const unsigned char test_1[TEST_MODE_CMD1_BUF_LEN] = {
		0x40, 0x42, 0x53, 0x55, 0x16, 0x00, 0x00, 0x00,
		0xBF, 0x45, 0x02, 0x00, 0x08, 0x34, 0x61, 0x73,
		0x18, 0x45, 0x04, 0x57, 0x02, 0xC8,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	/*test_2：40 42 53 55 | 0E 00 00 00 | BF EE 01 0F 02 57 | FF×2*/
	const unsigned char test_2[TEST_MODE_CMD2_BUF_LEN] = {
		0x40, 0x42, 0x53, 0x55, 0x0E, 0x00, 0x00, 0x00,
		0xBF, 0xEE, 0x01, 0x0F, 0x02, 0x57,
		0xFF, 0xFF
	};
	/*test_3：40 42 53 55 | 0E 00 00 00 | BF EE 55 88 00 86 | FF×2*/
	const unsigned char test_3[TEST_MODE_CMD3_BUF_LEN] = {
		0x40, 0x42, 0x53, 0x55, 0x0E, 0x00, 0x00, 0x00,
		0xBF, 0xEE, 0x55, 0x88, 0x00, 0x86,
		0xFF, 0xFF
	};

	unsigned char spi_read_dummy[TEST_MODE_DUMMY_LEN];
	unsigned char receive_data[TEST_MODE_RESPOND_LEN];
	unsigned char state = 0;
	unsigned long ret;

	memset(spi_read_dummy, 0x00, sizeof(spi_read_dummy));
	memset(receive_data, 0x00, sizeof(receive_data));

	/*进入条件已由调用方判定(工厂码状态 == 02)*/
	/*Input condition has been determined by the caller (factory code state == 02)*/
	printf("TEST MODE: HSM IS IN TEST MODE(02)\n");
	/*==================== Step 1：test_1 鉴别 ====================*/
	if (TestModeExchange(pFunc, test_1, TEST_MODE_CMD1_BUF_LEN,
	                     TEST_MODE_DELAY_AFTER_CMD1_MS, TEST_MODE_DELAY_AFTER_DUMMY1_MS,
	                     spi_read_dummy, receive_data))
	{
		return TEST_MODE_PROCESS_FAILED;
	}
	if ((0x90 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 0])
	 || (0x00 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 1]))
	{
		printf("TEST MODE: AUTHENTICATION FAILED (test_1)\n");
		return TEST_MODE_PROCESS_FAILED;
	}

	/*==================== Step 2：test_2 FT ====================*/
	if (TestModeExchange(pFunc, test_2, TEST_MODE_CMD2_BUF_LEN,
	                     TEST_MODE_DELAY_AFTER_CMD2_MS, TEST_MODE_DELAY_AFTER_DUMMY2_MS,
	                     spi_read_dummy, receive_data))
	{
		return TEST_MODE_PROCESS_FAILED;
	}
	if ((0x0F != receive_data[TEST_MODE_RESPOND_HDR_LEN + 0])
	 || (0x0F != receive_data[TEST_MODE_RESPOND_HDR_LEN + 1])
	 || (0x90 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 2])
	 || (0x00 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 3]))
	{
		printf("TEST MODE: FT FAILED (test_2)\n");
		return TEST_MODE_PROCESS_FAILED;
	}

	/*==================== Step 3：test_3 Exit Test Mode ====================*/
	if (TestModeExchange(pFunc, test_3, TEST_MODE_CMD3_BUF_LEN,
	                     TEST_MODE_DELAY_AFTER_CMD3_MS, TEST_MODE_DELAY_AFTER_DUMMY3_MS,
	                     spi_read_dummy, receive_data))
	{
		return TEST_MODE_PROCESS_FAILED;
	}
	if ((0x90 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 0])
	 || (0x00 != receive_data[TEST_MODE_RESPOND_HDR_LEN + 1]))
	{
		printf("TEST MODE: EXIT TEST MODE FAILED (test_3)\n");
		return TEST_MODE_PROCESS_FAILED;
	}

	/*==================== Step 4：Hard Reset + Confirm Download State ====================*/
	/*对应 MCU：ResetTarget(); tempstatus = ReadChipStatus();*/
	HSMReset();
#if (TEST_MODE_RESET_WAIT_MS > 0)
	HSMMsDelay(TEST_MODE_RESET_WAIT_MS);
#endif

	ret = ReadFactoryCodeState(pFunc, &state);
	if (ret)
	{
		printf("TEST MODE: READ STATE AFTER RESET FAILED\n");
		return TEST_MODE_PROCESS_FAILED;
	}

	if (!IsDownloadState(state))
	{
		printf("TEST MODE: HSM IS NOT IN DOWNLOAD STATE(03/04/05) AFTER RESET, STATE=%02X\n", state);
		return TEST_MODE_PROCESS_FAILED;
	}
	printf("TEST MODE: HSM ENTERED DOWNLOAD STATE, STATE=%02X\n", state);

#if TEST_MODE_INCLUDE_DOWNLOAD
	HSMReset();
#if (TEST_MODE_RESET_WAIT_MS > 0)
	HSMMsDelay(TEST_MODE_RESET_WAIT_MS);
#endif

	ret = ReadFactoryCodeState(pFunc, &state);
	if (ret)
	{
		printf("TEST MODE: READ STATE AFTER DOWNLOAD FAILED\n");
		return TEST_MODE_PROCESS_FAILED;
	}
	if (IsDownloadState(state))
	{
		printf("TEST MODE: DOWNLOAD FAILED, STILL IN STATE %02X\n", state);
		return TEST_MODE_PROCESS_FAILED;
	}
	printf("TEST MODE: DOWNLOAD SUCCESS\n");
#endif

	return TEST_MODE_PROCESS_SUCCESS;
}

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
--------------------------------------------------------------------------------------------------
修改下载流程。加入测试态状态的处理。
注意:处理的时候需要在测试态-下载态的转换之后进行芯片的硬复位。
Modify the download process. Add the processing of the test state.
Note: When processing, a hard reset of the chip is required after the conversion from test state to download state.

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
	/*LOCK THE ENTIRE ERASE AND DOWNLOAD FLOW*/
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

		/*=============================================================================
		 * 【新增】测试态处理流程：下载 BOOTLOADER.ini 之前，先读工厂码判断状态
		 *  [added] Test mode process: Before downloading BOOTLOADER.ini, first read the factory code to determine the state
		 *-----------------------------------------------------------------------------
		 * 逻辑：
		 *     读工厂码 -> 状态 == 02  -> 进入测试态流程（执行02态SPI指令 + 硬件复位）
		 *                        -> 复位后应进入 03 态，再继续下载 bootloader
		 *               状态 == 03/04/05 -> 跳过测试态流程，直接下载 bootloader
		 * Logic:
		 *     Read factory code -> state == 02 -> enter test mode process (execute 02 state SPI command + hardware reset)
		 *                        -> after reset should enter 03 state, then continue to download bootloader
		 *               state == 03/04/05 -> skip test mode process, directly download	 bootloader
		 *-----------------------------------------------------------------------------		
		 *===========================================================================*/
		{
			unsigned char factory_code_state = 0;

			/*读取工厂码并取状态字节*/
			ret = ReadFactoryCodeState(&ISTECC512AFunctionPointerStructure,
			                           &factory_code_state);
			if (ret)
			{
				printf("READ FACTORY CODE FAILED, PLEASE CHECK THE COMMUNICATION\n");
				HSMClearPMutexAndSemphre();
				HSMHardwareDeinit();
				return DOWNLOAD_BOOTLOADER_FAILED;
			}

			printf("CURRENT FACTORY CODE STATE :%02X\n", factory_code_state);

			if (factory_code_state == FACTORY_CODE_STATE_TEST)
			{
				/*02态：进入测试态流程*/
				printf("FACTORY CODE IS 02, ENTER TEST MODE PROCESS\n");

				/*测试态处理流程：执行02态SPI指令 -> 硬件复位 -> 确认进入03态*/
				ret = HSMTestModeProcess(&ISTECC512AFunctionPointerStructure);
				if (ret)
				{
					printf("TEST MODE PROCESS FAILED, PLEASE CHECK THE HSM\n");
					HSMClearPMutexAndSemphre();
					HSMHardwareDeinit();
					return DOWNLOAD_BOOTLOADER_FAILED;
				}

				//提示用户复位模块，确保模块进入下载态
				//Prompt the user to reset the module to ensure it enters the download state.
				printf("TEST MODE PROCESS SUCCESS, WILL DOWNLOAD BOOTLOADER\n");
				for(int i =0; i<10;i++)
				{
					printf("PLEASE MAKE SURE RESET THE  HSM MODLUE!\n");
					HSMMsDelay(1000);
				}
				while(1);
			}
			else if ((factory_code_state == FACTORY_CODE_STATE_03)
			      || (factory_code_state == FACTORY_CODE_STATE_04)
			      || (factory_code_state == FACTORY_CODE_STATE_05))
			{
				/*03/04/05：跳过测试态流程*/
				printf("FACTORY CODE IS %02X, SKIP TEST MODE PROCESS, GO TO DOWNLOAD BOOTLOADER\n",
				       factory_code_state);
			}
			else
			{
				/*其他状态,将状态输出并停止*/
				/*Other states, output the state and stop*/
				printf("FACTORY CODE IS %02X, NOT IN TEST MODE OR DOWNLOAD MODE, PLEASE CHECK THE HSM\n",
				       factory_code_state);
				while(1);	   
			}
		}

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
