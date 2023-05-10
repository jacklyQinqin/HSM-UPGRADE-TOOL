/*
 * @Author: qinxd qinxiaodong_ly@163.com
 * @Date: 2023-03-15 10:40:42
 * @LastEditors: qinxd qinxiaodong_ly@163.com
 * @LastEditTime: 2023-03-15 14:50:52
 * @FilePath: \HSM-TEST-DEMO\hsm_gpio.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */

#ifndef _HSM_GPIO_H_
#define _HSM_GPIO_H_

#include <stdlib.h>  
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define GPIO_IN	  		0x80
#define GPIO_OUT	 	0x40


typedef struct {
	int pin;  // pin number  base_group *32 + Pinx
	int value;// 0 or 1
	unsigned char direction;//direction of pin .
} GpioMessageTyepdef;

int gpio_unexport(GpioMessageTyepdef* GpioX);
int gpio_read(GpioMessageTyepdef* GpioX);
int gpio_write(GpioMessageTyepdef* GpioX, int value);
int gpio_direction(GpioMessageTyepdef* GpioX);
int gpio_export(GpioMessageTyepdef* GpioX);

#endif
