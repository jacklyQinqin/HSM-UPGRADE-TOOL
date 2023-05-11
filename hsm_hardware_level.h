/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
/**
  ******************************************************************************
  * File name: __HSM_HARDWARE_LEVEL__
  * Description: 
  * Auth : qin xiaodong 
  * Email: qinxd@istecc.com
  * API VERSION: 0.4
  * Time : 2023-5-10
  * note : HSM_ means high secure module.
  * This file provides hardware - specific initialization capabilities. SPI interface initialization, and reset, BUSY, SS three IO initialization.
  * The following functions are used by users:
  * Initialize required hardware (SPI, RESET,BUSY,SS)
  * Gets the BUSY signal status of the module.
  * Pull it down for 100 milliseconds and then pull it up again to reset the module.
  * Release all initialization resources.
  *  
  * History: 
  * V0.4
  * 

  ******************************************************************************/
#ifndef __HSM_HARDWARE_LEVEL__
#define __HSM_HARDWARE_LEVEL__
#include <stdint.h>
#include <stdlib.h>


/**
Define the transmission mode of THE NXP master(imx8mq.)
8bit transmission is the common mode
32-bit transmission is word mode. 8bit transmission mode is recommended
**/
#define BYTE_TR_MODE 8
#define WORD_TR_MODE 32
#define DEFAULT_MODE BYTE_TR_MODE


/*
WARING:
The following sections are what users need to implement
1.
*/

/**
 * @brief 
 * init the HSM module's hardware part. 
 * Contains the following parts
 * SPI interface (including SPI-SS)
 * Busy PIN (Input mode)
 * Reset PIN (output mode, default high rating)
 * If a separate SS is required
 * SS-1 PIN (output mode, high level by default)
 * Call at the start.
 * @param in_speed 
 * @return unsigned int 
 */
extern unsigned int  HSMHardwareInit(unsigned long in_speed);

/**
 * @brief 
 * Release occupied I/O and SPI resources
 * Call at the end
 * @return unsigned int 
 */
extern unsigned int  HSMHardwareDeinit(void);

/**
 * @brief 
 * Reset the module, lower the reset pin for 100ms, and then raise it.
 * @return unsigned int 
 */
extern unsigned int  HSMReset(void);

/**
 * @brief 
 * The SPI interface of the master sends data of a specified length.
 * @param tx 
 * @param tx_len 
 * @return unsigned int 
 */
extern unsigned int HSMWrite(unsigned char * tx,unsigned int tx_len);
/**
 * @brief 
 * The SPI interface of the master master reads the specified length of data.
 * @param rx 
 * @param rx_len 
 * @return unsigned int 
 */
extern unsigned int HSMRead(unsigned char * rx,unsigned int rx_len);

/**
 * @brief 
 * 
 * @return unsigned int 
 */
extern unsigned int HSMGetBusystatus(void);

/**
 * @brief 
 * us delay.
 * @param us_delay 
 * @return int 
 */
extern int HSMUsDelay(uint32_t us_delay);
/**
 * @brief 
 * us delay.
 * @param ms_delay 
 * @return int 
 */
extern int HSMMsDelay(uint32_t ms_delay);
/**
 * @brief 
 * debug print fucntion for linux.
 * @param src 
 * @param length 
 * @param line_size 
 * @param prefix 
 */
extern void hex_dump(const void *src, size_t length, size_t line_size, char *prefix);
#endif
