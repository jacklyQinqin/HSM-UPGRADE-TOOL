#	IS32U512A-SM2 MODULE DRIDER AND TEST CASE.

## 1.8.8.4-01
AUTHOR:QINXD
TIME:2022/7/20
ADD SM2 KDF AND POINTER DECOMPRESS.
2022/7/25

## 1.8.8.4-02
COS VERSION 1.8.8.4
SOC VERISON V2
更正了密钥衍生部分的说明文档和注释。
KE和KS是同一个流程
完整密钥或者是一个流程。

修改后的名字是：
ISTECC512A_SM2EncryptSignKeyKDF     生成衍生的签名或者加密密钥
ISTECC512A_SM2CompleteKDF           生成完整私钥Si.


## 1.8.8.4-03
COS VERSION 1.8.8.4
SOC VERISON 第三版
加入完整的秘钥衍生例程


## 1.8.8.5-01
为了保持格式兼容，将logic_level的所有int型变量转为unsigned long
char转换为unsinged char

## 1.8.8.6
add mod add api.
INS: 0X3B

## 1.8.8.7 
The test routine of SM4CCM is added.
Added a separate SM2 encryption and decryption routine.
Added separate mod_ Add interface.

## 1.8.9.1
add e+rs verify mode. and test data.


## 1.8.9.2
新的COS更正了HSM模块的点解压的问题。
新的测试例程更正了点解压的测试例程。分别进行MODE2,MODE3的测试。
修改了hex_dump()程序。只进行简单的打印信息输出。界面更加清晰。
将DownloadFile.c 和DownloadFile.h更新为1.8.9.2版本。
更正了导入私钥函数中错误的长度(64+6).更正为32+6.
ImportSM2Prikey
更正了导出私钥函数中错误的长度。


## 1.8.9.4
增加了E值签名的功能和测试DEMO.
修改了APP update 流程。支持在线升级

## 1.8.9.5
增加了直接获取sij的功能。将导入秘钥和导出秘钥等操作包含在内。只用调用一次即可。


## verison: 1.8.9.6
updated based  on verison 1.8.9.5  
The original verison is 1.8.9.5. ccm.c ccm.h sm4.c sm4.h files were removed on this basis.
and the gpio.c gpio.h was rewritten.The gpio control structure has beed redefiend. 
Removed some test code.  unsigned long ISTECCSm4CcmTest(void)
Removed the  include "ccm.h"  and  include "sm4.h"
MIT's license description was added at the beginning of each file.


## verison: 1.8.9.6(01)
add some debug message in gpio.c 
the function SM2EValueVerify()  (in hsm_logic_level.c)
add hsmdelay(10) below 
    HSMMsDelay(1);
    HSMMsDelay(10);
    while (HSMGetBusystatus())

## verison: 1.8.9.6(02)
1. Add thread locks hsm_logic_level.c.

2. Increase the delayed waiting time of functions such as signature/signature verification.

3. Increase the amount of semaphores between processes.

4. Delete the contents of HSMGetBusystatus and the operation of acquiring busy signals, and change all waiting times to delayed waiting.

5. Update the multithreaded test routine. And test in multiple cases. You can do it separately in both terminals
HSMSelfTestWithMultithreading() and  HSMSelfTestWithMulProcess()  of tests.


## verison: 1.8.9.7
1. Add function : One key to destory all key's of hsm.

## verison: 1.8.9.8
1.  Add SendOneMessage and RecOneMessage funciton pointer for Upgrade funciton.
2.  Modify the HSMVSemphre ,remove the static .then they can be called extern.

## verison 1.8.9.8(3)
MERGE HSM-TEST-DEMO AND HSM-UPGRADE-TOOL.SO WE WILL GET  UNIFORM DRIVER.

1.  Adjusted the HSM upgrade process,
2.  Modify some  detail for main. If you pass in six parameters according to the previous Upgrade Code tool, the HSM upgrade will be performed. If 2 parameters are passed in, then the test will be performed.
3.  Abstracts the upgrade functionality into a separate source file hsm_upgrade_test.c and .h
4.  Modified the HSMHardwareInit function. add  HSMSempohreInit() and HSMThreadMutexInit(); And try 10000 times to get the value of the semaphore. if all of them 0, then setSemphre.
## verison 1.8.9.8(4)
1. Add a New HardwareSemphre  to record the status of the initialization. I think it's a better way.

## verison 1.8.9.8(5)
1. Modify the delay times of every fucntions. add fix tiem MACRO define.
2. Most of delays is fixed.And Some need to automatically judge the delay based on the length.
3. Notes: I modify the flow of Generate key pairs. Because the public key is filtered when the key is generated . This period is not fixed(most of the time it's 3-20ms).
   I give enough delay when delaying(50ms). But to prevent a very small probalility ,I added the following judgment and tried to re-read the result when I could't get the correct respond.

      
    /*GerKeypair success*/
    if (rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
    {
        HSMVSemphre();
        pthread_mutex_unlock(&hsm_mutex_pthread);
        return sucess;
    }
    else if (rx_buff[0] == 0x69 && rx_buff[1] == 0x85)/*GerKeypair fail*/
    {
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
        #if (HSM_LOGIC_LINIX_DEBUG_ON == 1)
        hex_dump(rx_buff, rx_buff_len, 16, "GenKeyPair rx:");
        #endif
        if (ret != 0)
        {
            HSMVSemphre();
            pthread_mutex_unlock(&hsm_mutex_pthread);
            return fail;
        }      
        /*GerKeypair success*/
        if (rx_buff[0] == 0x90 && rx_buff[1] == 0x00)
        {
            HSMVSemphre();
            pthread_mutex_unlock(&hsm_mutex_pthread);
            return sucess;
        }
        else
        {
            HSMVSemphre();
            pthread_mutex_unlock(&hsm_mutex_pthread);
            return fail; 
        }
        
    }

## verison 1.8.9.8(6)
Checked the API for errors.
Unified HSMRead processing,Print information only at the wrong time.Now they look at this:

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

## verison 1.8.9.9(01)
### API UPGRADE INSTRUCTIONS:
	1.Modify the ModAdd Function
	from 
	unsigned long ModAdd(unsigned char *bij,unsigned char *c,unsigned char *out_sij)
	to 
	unsigned long ModAdd(unsigned  int  bij_index,unsigned  char  *c,unsigned  char  *out_sij)
	you need to change the key deriver flow.
	just pass the bij_index to ModAdd,don't need export prikey.
	
	2.Deltel the export prikey API. And some testing  code was be modify to fit it.
	3.Modify the delay of ImportSM2Prikey  to 20ms.The max result of the test is 15ms.
	4.Add new upgrade file DownloadFile18909.ini

	NOTE: YOU NEED UPGRADE THE HSM CODE TO FIT HE NEW API.
### HSM UPGRADE INSTRUCTIONS:
	1.Modify the ModAdd function. You need pass bij_index to hsm not bij .
	2.Modify the setId  funciton. Not stored to flash.(This's not needed.)
## verison 1.8.9.9(02)
	1.Base 1.8.9.9 (01) Pushed some missing files.
## verison 1.8.9.9(03)

    1.
    Because the new HSM FW upgrade method is mainly in the script_deal.c file and upgrade test is hsm_upgrade_test.c.
    so these test were removed(in hsm_test_task.c):
    ISTECCEraseAPPTest() 
    ISTECCEraseAPPTest2()
    ISTECCUpdateTest() 
    and The API p->ISTECC512A_APPUpdate  were removed.(We don't need it)
    #include "Downloadfile.h" in hsm_logic_level.c were remoded.
    and DownloadFile.h  DownloadFile.c  were removed. we use separate upgrade file  (for example DownloadFile18909.ini).
    2.  
    We delete some commentted code. we don't need then now. 
    Some funciton declarations and external referenees were added. complier warings are eliminated. 

## verison 1.9.0.1(01)
	1. Added bootloader.Support the function of retaining SM2 keys when upgrading.
	2. Adjusted the process of upgrading the function. The upgrade process of HSM is not interrupted by other processes and threads.
	If the HSM chip does not have a bootloader. The bootloader will be downloaded and the new version of the firmware will be downloaded through the bootloader.
	If the HSM chip has an older version of the firmware. The firmware will be erased, and the bootloader will be downloaded, and the new version of the firmware will be downloaded through the bootloader.
	If the HSM already has a bootloader. The new version of the firmware will be downloaded directly.
	If the HSM already has bootlaoder and firmware, it will erase the firmware and download the new version of the firmware via bootloader.
	3. Note: The format of the upgrade file for the new version and the old version is different. and will be unified into a bootloader version.
	4. The new version of the firmware is 1.9.0.1.
	The version of bootloader is 1.0.3 + the string 'spiloader' is used to distinguish firmware from bootloader.
	
## verison 1.9.0.1(02)
    1.Modify the return value of   HSMHardwareInit  and HSMHardwareDeinit (from (unsigned int) to (int))
    2.Modify the test code of multiple pthread. HSMSelfTestWithMultithreading() .  Hardware initalizaiton before threads are created. and Deinit after exit pthread.
    3.Modify the function HSMHardwareInit(). add the extern errno; if open device faied. will printf the error number.
    4.Modify the type of fd. from (static int fd;) to (int  fd;);

 
## verison 1.9.0.1(03)

MODIFY THE HARDWARE INIT  HSMREAD AND HSMWRITE.
    1.Modify the  HSMRead and HSMWrite.  HSMSpiInit  and close(fd)  will be executed every time. 
    and add  memset(&tr,0x00,sizeof(tr)); (referce to the nxp community).
    2.Move the spi init from HSMHardwareInit  to  static int HSMSpiInit(unsigned long in_speed);
    3.Modify the 
    ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
    ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
    to 
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
