/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>     
#include <sys/stat.h>       
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include "hsm_gpio.h"
#define HSM_GPIO_DEBUG 0

int gpio_export(GpioMessageTyepdef* GpioX)
{  
	char name[16] = {0};  
	int fd = -1;
	int len = 0;  
	int ret = 0;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		printf("HSM-DEBUG: gpio_export open failed, fd: %4d\n",fd);
		return(-1);
	}
	sprintf(name, "%d", GpioX->pin);  
	len = strlen(name);
	ret = write(fd, name, len);
	close(fd);

	if (ret < 0) 
	{
		/* GPIO errno=16, Device or resource busy, already exist*/
		if (errno == 16)
		{
			printf("HSM-LOG:export's fd: %4d Device or resource busy,error %4d,ret is %4d\n",fd,errno,ret);
			return 0;
		}
		else
		{
			printf("HSM-DEBUG:gpio_export's fd: %4d write fail,error %4d,ret is %4d\n",fd,errno,ret);
			return -1;  
		}
	}  
	return 0;  
} 



int gpio_direction(GpioMessageTyepdef* GpioX)
{  
	char path[128] = {0};  
	int fd = -1;
	int res = -1;
	char * dir;
	if(GpioX->direction & GPIO_IN)
		dir = "in";

	if(GpioX->direction & GPIO_OUT)
		dir = "out";
	
	sprintf(path,"/sys/class/gpio/gpio%d/direction", GpioX->pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		printf("HSM-DEBUG: gpio_direction open failed ,fd is 4%d\n",fd);
		return -1;  
	} 
	printf("HSM-DEBUG:goio_direction open success ,fd is 4%d\n",fd);
	res = write(fd, dir, sizeof(dir));
	close(fd);  
	if (res <= 0) {
		printf("HSM-DEBUG: gpio_direction write failed ,res is 4%d\n",res);
		return -1;
	}
	#if(HSM_GPIO_DEBUG == 1)
	printf("HSM-DEBUG: write direction success ,res is 4%d\n",res);
	#endif

	return 0;
}

int gpio_read(GpioMessageTyepdef * GpioX)
{
	char path[128] = {0};
	char value_str[4] = {0};
	int value = -1;
	int fd = -1;
	int res;
	sprintf(path, "/sys/class/gpio/gpio%d/value", GpioX->pin);  
	fd = open(path, O_RDONLY);
	if (fd < 0) {  
		printf("HSM-DEBUG:gpio_read open failed ,fd is 4%d\n",fd);
		return -1;  
	}
	#if(HSM_GPIO_DEBUG == 1)
	printf("HSM-DEBUG:goio_read open  success ,fd is 4%d\n",fd);
	#endif
	if (read(fd, value_str, 3) < 0) {  
		printf("HSM-DEBUG: read gpio value failed ,fd is 4%d\n",fd);
		close(fd);  
		return -1;
	}

	value = atoi(value_str);
	#if(HSM_GPIO_DEBUG == 1)
	printf("HSM-DEBUG: read gpio value success ,value is 4%d\n",value);
	#endif
	close(fd);
	return value;
}

int gpio_write(GpioMessageTyepdef * GpioX, int value)
{  
	char path[128] = {0};  
	int fd = -1;  
	char value_str[16] = {0};
	
	sprintf(path, "/sys/class/gpio/gpio%d/value", GpioX->pin);
	sprintf(value_str, "%d", value);

	fd = open(path, O_RDWR);
	if (fd < 0) {  
		printf("HSM-DEBUG:gpio_write open failed ,fd is %4d\n",fd);
		return -1;  
	}
		printf("HSM-DEBUG: gpio_write  open success\n");

	int res = write(fd, value_str, strlen(value_str));
	if (res <= 0) {
		close(fd);  
		printf("HSM-DEBUG:gpio_write  failed\n,fd id %d ,res is %4d",fd , res);
		return -1;
	}
	close(fd);  
	return 0;  
}

int gpio_unexport(GpioMessageTyepdef* GpioX)  
{  
	char name[4] = {0};  
	int len = 0;  
	int fd = -1;  
	
	fd = open("/sys/class/gpio/unexport", O_WRONLY);  
	if (fd < 0) {  
		printf("HSM-DEBUG:gpio_export open failed: %d,gpio is %4d\n", fd,GpioX->pin);
		return -1;  
	}  

	len = sprintf(name, "%d", GpioX->pin);
	if (write(fd, name, len) < 0) {  
		close(fd);  
		printf("HSM-DEBUG:gpio_unexport write failed fd is %d,gpio is %4d\n", fd,GpioX->pin);
		return -1;  
	}  
	
	close(fd);  
	#if(HSM_GPIO_DEBUG == 1)
	printf("HSM-DEBUG:gpio_unexport success: fd is %4d,gpio is %4d\n", fd,GpioX->pin);
	#endif

	return 0;  
}