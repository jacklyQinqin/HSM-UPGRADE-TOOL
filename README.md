# HSM-UPGRADE-TOOL
This is hsm‘s upgrade tool source code。licensed on MIT License。
#IS32U512A-SM2 MODULE DRIDER AND TEST CASE.
AUTHOR:QINXD
TIME:2023/2/08

THIS IS A TOOL FOR UPGRADE HDXA HSM MODULE.

FUNCITN:
1.CHECK VERISON.
2.ERASE HSM-HARDWARE.
3.UPGRADE NEW HARDWARE.
4.SIMPLE TEST SELF.

NOTE:
YOU NEED GIVE APP A NAME OF SPI DEVICE.

HOW TO USE IT?
1.GET THE NEW UPGRADE FILE FROM GITHUB.
2.REPLACE THE DOWNLOADFILE.C AND DOWNLOADFILE.H.
3.REPLACE THE busyPin AND resetPin at hsm_hardware_level.h
4.COMPILE THE PROJECT WITH YOUR TOOLCHAIN 

EXECUTE THE PROGRAM AND PASS THE SPIDEV NAME AS PARAMETERS 2.
FOR EXAMPLE(IN MY ENVIRONMENT):
# ./SKF-IMX8-IS32U512A-UPGRADE-TOOL_test  /dev/spidev32766.0


##	V0.1
The first verison. and test pass.


##	V0.3
Remove the ccm.c  and ccm.h 
sm4.c sm4.h 
Remove irrelevant  test code
ReWrite the  code part of GPIO operation.
add MIT license.

<<<<<<< HEAD
##	V0.7
TIME:2023/4/11
HERE ARE UPDATES:
###	Call the ExportGpioAndInit() function separately.
###	Adjusted the order of initialization.

    ExportGpioAndInit();
    HSMReset();
    HSMHardwareInit(spi_frequency);
    
### The test results in my environment are as follows:
  	

       # ./HSM-UPGRADE-TOOL_test /dev/spidev32766.0 98 96 DownloadFile18906.ini 2000000

    This is a tool for upgrade hdxa sm2 module 
    
    Please Input below paremeters. 
    
    paremeter0:.exe_name 
    
    paremeter1:.device name 
    
    paremeter2:.gpio_reset number. If you don't have reset Pin, Please user power on again instand of HSMReset() 
    
    paremeter3:.gpio_busy  number 
    
    paremeter4:.upgrade file name (xxxx.ini) 
    
    paremeter5:.SPI-Frequency 
    
    LINK THIS(MY ENVIRONMENT): 
    
    ./HSM_UPGRADE-TOOL_test  /dev/spidev32766.0  98 96 DwonloadFile18907.ini  1000000 
    
    Please note: 
    
    1.Erase or after download file sucess,you need RESET the HSM. or POWER ON AGAIN 
    
    2.I recommend the frequency is 1M~4M,Not too slow,not too fast. 
    
    VERSION : 0.0.7 
    
    TIME:  2023-4-11 
    
    SPI_DEV_NAME IS/dev/spidev32766.0 
    
    PAR0: ./HSM-UPGRADE-TOOL_test 
    
    PAR1: /dev/spidev32766.0 
    
    PAR2: 98 
    
    PAR3: 96 
    
    PAR4: DownloadFile18906.ini 
    
    PAR5: 2000000 
    
    your spi_frequency is 2000000 
    
    CURRENT STEP :   1 
    
    hsm_hardware_init and the speed is 2000000 
    
    spi mode: 0x0 
    
    bits per word: 8 
    
    sync success 
    
    IS32U512A module's verison is   1  8  9  6 
    
    READ APP VERISON SUCCESS 
    
    CURRENT STEP :   2 
    
    hsm_hardware_init and the speed is 2000000 
    
    spi mode: 0x0 
    
    bits per word: 8 
    
    sync success 
    
    pin is 8 byte password.! 
    
    pin confirm success! 
    
    Erase app success! 
    
    ERASE HSM APP SUCCESS.PLEASE POWER BACK OR RESET HSM MODULE! 
    
    THE NEXT STEP WILL DOWNLOAD NEW HSM APP 
    
    CURRENT STEP :   3 
    
    hsm_hardware_init and the speed is 2000000 
    
    spi mode: 0x0 
    
    bits per word: 8 
    
    the file is DownloadFile18906.ini 
    
    microsecond interval:    7544 MS 
    
    total line  132 
    
    compare enable and success. 
    
    start guide hsm 
    
    fclose fp and you neet to reset HSM or power again 
    
    DOWNLOAD NEW HSM APP SUCCESS! 
    
    THE NEXT STEP WILL CHECK NEW VERISON 
    
    CURRENT STEP :   4 
    
    hsm_hardware_init and the speed is 2000000 
    
    spi mode: 0x0 
    
    bits per word: 8 
    
    sync success 
    
    IS32U512A module's verison is   1  8  9  6 
    
    READ APP VERISON SUCCESS 
    
    UPGRADE SUCCESS . CONGRATULATIONS!


=======
-------------------------------------------------------------
TIME      :2023-4-19
ADD NEW UPGRADE FILE . NAME:   DownloadFile18907-new.ini
Added a constraint on generating keys, now only public keys with a public key whose first byte is less than 0X80.
>>>>>>> 7abc5ebc4b80a6d2e05df6f9860c05f82a409a7f
