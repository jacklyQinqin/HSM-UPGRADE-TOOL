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


V0.1
The first verison. and test pass.


V0.3
Remove the ccm.c  and ccm.h 
sm4.c sm4.h 
Remove irrelevant  test code
ReWrite the  code part of GPIO operation.
add MIT license.

-------------------------------------------------------------
TIME      :2023-4-19
ADD NEW UPGRADE FILE . NAME:   DownloadFile18907-new.ini
Added a constraint on generating keys, now only public keys with a public key whose first byte is less than 0X80.
