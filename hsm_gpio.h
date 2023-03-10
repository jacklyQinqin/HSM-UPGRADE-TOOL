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
#define GPIO_OUT	 	0x10


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
