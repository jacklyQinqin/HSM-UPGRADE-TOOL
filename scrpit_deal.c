/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
//提供计时操作,用来统计算法效率
#include <sys/time.h>

#include "script_deal.h"


#include "hsm_hardware_level.h"
#define DEBUG_ON 0
/*
0x9000
0x6500  
鉴别数据错误
0x6985
未配置算法库+BF46000A00
下载态执行测试态的指令
BFEE0300050008000000	0x6985
*/
extern unsigned int spi_frequency;
/*协议的指令头和错误码*/
const char head[4]  = {0x40,0x42,0x53,0x55};
const char len[4]  = {0x16,0x00,0x00,0x00};
/************************************************************************
*	FunctionName : atohex
*	Descriptor : 将一个char[2]转为一个16进制unsigned char
*	Parameter :
*		1. char* a : 要转换的2位字符串
*	ReturnValue :	
*		unsigned char类型的16进制数
************************************************************************/

static unsigned char atohex(char* a)
{
	unsigned char chRet;
	unsigned char low;
	unsigned char high;
	char cHigh = a[0];
	char cLow = a[1];

	//	高位
	if (cHigh>='0' && cHigh<='9')
	{
		high = (unsigned char)(cHigh-48);
	}
	else if (cHigh>='A' && cHigh<='F')
	{
		high = cHigh-55;
	}
	else if (cHigh>='a' && cHigh<='f')
	{
		high = cHigh-87;
	}

	//	低位
	if (cLow>='0' && cLow<='9')
	{
		low = (unsigned char)(cLow-48);
	}
	else if (cLow>='A' && cLow<='F')
	{
		low = cLow-55;
	}
	else if (cLow>='a' && cLow<='f')
	{
		low = cLow-87;
	}

	chRet = high;
	chRet = chRet<<4;
	chRet+= low;

	return chRet;
}

/************************************************************************
*	FunctionName : str_to_hex
*	Parameter :	字符串转换为16进制数,顺序转换
*	Parameter :
		1. char* chArray	: 带转换的字符串
		2. int Length		: 字符串长度
		3. unsigned char* cHex	: 转换后的16进制数
*	ReturnValue : 16进制数组的实际操作长度
************************************************************************/
static int str_to_hex(char* chArray,int Length,unsigned char* cHex)
{
	char chTemp[2];
	int i;
	for (i=0; i<Length/2; i++)
	{
		memcpy(chTemp,&chArray[i*2],2);
		cHex[i] = atohex(chTemp);
	}

	return Length/2;
}

static void hex_to_str(unsigned char *cHex,int len,char *chArray)
{
	unsigned char temp,temp_l,temp_h;
	
	while(len--)
	{
		temp = *cHex++;
		temp_h = (temp>>4)&0x0F;
		if((temp_h>=0x0)&&(temp_h<=0x09))
			*chArray++ = '0'+temp_h;
		else if((temp_h>=0x0A)&&(temp_h<=0x0F))
			*chArray++ = 'A'+(temp_h-0x0A);
		
		temp_l = temp&0x0F;
		if((temp_l>=0x0)&&(temp_l<=0x09))
			*chArray++ = '0'+temp_l;
		else if((temp_l>=0x0A)&&(temp_l<=0x0F))
			*chArray++ = 'A'+(temp_l-0x0A);
		
		*chArray++ = ' ';
	}
	
	*chArray++	= '\n';
	*chArray = '\0';
	
}

static void hex_to_str2(unsigned char *cHex,int len,char *chArray)
{
	unsigned char temp,temp_l,temp_h;
	
	while(len--)
	{
		temp = *cHex++;
		
		temp_h = (temp>>4)&0x0F;
		if((temp_h>=0x0)&&(temp_h<=0x09))
			*chArray++ = '0'+temp_h;
		else if((temp_h>=0x0A)&&(temp_h<=0x0F))
			*chArray++ = 'A'+(temp_h-0x0A);
		
		temp_l = temp&0x0F;
		if((temp_l>=0x0)&&(temp_l<=0x09))
			*chArray++ = '0'+temp_l;
		else if((temp_l>=0x0A)&&(temp_l<=0x0F))
			*chArray++ = 'A'+(temp_l-0x0A);
		
		*chArray++ = ' ';
	}

	*chArray = '\0';	
}


static int usb_format_to_spi_format(char * usb_format_cmd,int * cmd_len, char *spi_format_cmd)
{
    int ret = 0;
    int  i = 0;
	/*最大的命令长度为257*/
    char cmd[512] =  {0x00,0x00};
    const char head[4] = {0x40,0x42,0x53,0x55};
    char  len[4] = {0x00,0x00,0x00,0x00};
    char xor = 0;
    /*
    头40425355
    长度0e000000
    指令bf44010000
    异或值f0
    */
   
    (* cmd_len) += 9;
    len[0] = (*cmd_len) & 0xff;
    len[1] = (*cmd_len) >> 8 & 0xff;
    len[2] = (*cmd_len) >> 16 & 0xff;
    len[3] = (*cmd_len) >> 24 & 0xff;
   
    memcpy(cmd,head,4);
    memcpy(&cmd[4],len,4);
    memcpy(&cmd[8],usb_format_cmd,*cmd_len-9);
    for(i=0;i<(*cmd_len);i++)
        xor ^= cmd[i];
	
	#if (DEBUG_ON != 0)
	printf("XOR is %d\n",xor);
	printf("cmd_len is %d\n",*cmd_len);
	#endif

    cmd[*cmd_len-1] = xor;

	#if (DEBUG_ON != 0)
	hex_dump(cmd,256,16,"SPI_FORMAT_CMD:");
	#endif

    memcpy(spi_format_cmd,cmd,*cmd_len);
    return 0;
}

/*测试读取脚本*/
int test_read_script(char * file_name)
{
    FILE  * fp;
    int count = 0;
    char string[]  = "THIS IS A TEST";
    
    char line[1024];
	
    #if(DEBUG_ON != 0)
    printf("the file is %s\n",file_name);
    #endif

    fp = fopen(file_name,"r");
    if(fp ==NULL)
    {
        printf("fp is NULL");
        return 1;
    }

    count = 0;
    memset(line,0x00,sizeof(line));
    while(fgets(line,100,fp) != NULL)
    {
        printf("line %4d is %s\n",count,line);
        count++;
    }
    fclose(fp);
    return 0;
}

/*解析一行脚本并将脚本的发送值/发送长度  接收对比值/接收对比值长度分别输出到指定的值
/*script_line :输入的一行脚本:例如 APDU=bf450200081392375533957469,0x90000
 *send        :输出的SPI接口的发送值
 *rec_buff    :响应值也就是需要做对比的值.
 *此函数的功能是去除 APDU=  和 "," 和 "0X",将其转换为纯的文本格式.
 *并转换为HEX格式
 *并转换为SPI格式 
 *输入脚本行
 *输出 SPI格式命令 ，SPI格式命令长度
       SPI的响应，   SPI响应的对比值长度
*/
int analysis_one_line_script(char * script_line,char *send,int * send_len, char *compare,int * compare_len)
{
    int ret;
    char buff[1024] = {0x00};
    char str_cmd[1024];
    char hex_cmd[1024];
    char str_compare[1024];
    char hex_compare[1024];

    char division[] = ",";
    char * ptr;
    int len = strlen(script_line);

    memset(buff,0x00,strlen(buff));
    memset(str_cmd,0x00,strlen(str_cmd));
    memset(hex_cmd,0x00,strlen(hex_cmd));
    memset(str_compare,0x00,strlen(str_compare));
    memset(hex_compare,0x00,strlen(hex_compare));
    
    #if(DEBUG_ON != 0)
    printf("SCRIPT_LEN is %4d\n",len);
    #endif 
    
    memcpy(buff,script_line,len);

    /*去除可能的\r\n*/
    if((buff[len-1] == '\r') || ((buff[len-1] == '\n')))
    {
        buff[len-1] = 0x00;
        len-=1;
    }
    if((buff[len-1] == '\r') || ((buff[len-1] == '\n')))
    {
        buff[len-1] = 0x00;
        len-=1;
    }

    #if(DEBUG_ON != 0)
    printf("SCRIPT_LEN is %4d\n",len);
    #endif 
    /*
    *以APDU=bf450200081392375533957469,0x9000为例
    *1.根据,将APDU和返回值进行拆分
    *  指令:APDU=bf450200081392375533957469
    *  返回值:,0x9000
    *  去掉APDU= 和 ,0x 保留数据部分
    * */
    #if(DEBUG_ON != 0)
    /*打印读取到的脚本信息*/
    printf("SCRIPT IS %s\n",buff);
    #endif 
    /*获取 , 分割符的地址,将指令和接收值分解为2个部分*/
    ptr = strstr(buff,division);
    #if(DEBUG_ON != 0)
    printf("The ptr is at position: %p\n", ptr);
    printf("The buff is at position: %p\n", &buff[5]);
    #endif
    /**/
    memcpy(str_cmd,&buff[5],ptr - &buff[5]);
    /*返回值从,开始,0x 三个字符*/
    memcpy(str_compare,ptr+3, &buff[len-1] - (ptr+2));    
    #if(DEBUG_ON != 0)
    printf("CMD: %s  LEN: %4d\n",str_cmd,ptr - &buff[5]);
    printf("REC: %s  LEN: %4d\n",str_compare,strlen(str_compare));
    #endif
    /*将其转换为HEX格式*/
    str_to_hex(str_cmd,strlen(str_cmd),hex_cmd);
	* send_len = (ptr - &buff[5]) >> 1;
	
	str_to_hex(str_compare,strlen(str_compare),hex_compare);
	* compare_len = strlen(str_compare) >> 1;

    memcpy(compare,hex_compare,*compare_len);
    
    /*将发送数据加头和XOR.*/
	usb_format_to_spi_format(hex_cmd,send_len,send);
    
    #if(DEBUG_ON != 0)
    printf("the send len  is %4d and the compare len is %4d\n",*send_len,*compare_len);
    #endif

    ret = 0;
    return ret;
}

/*解析一行脚本并将脚本的发送值/发送长度  接收对比值/接收对比值长度分别输出到指定的值
/*script_line :输入的一行脚本:例如 APDU=bf450200081392375533957469,0x90000
bootloader 格式的直接发送和比较即可。
*/
int analysis_one_line_script_for_bootloader(char * script_line,char *send,int * send_len, char *compare,int * compare_len)
{
    int ret;
    char buff[2048] = {0x00};
    char str_cmd[2048];
    char hex_cmd[2048];
    char str_compare[2048];
    char hex_compare[2048];

    char division[] = ",";
    char * ptr;
    int len = strlen(script_line);

    memset(buff,0x00,strlen(buff));
    memset(str_cmd,0x00,strlen(str_cmd));
    memset(hex_cmd,0x00,strlen(hex_cmd));
    memset(str_compare,0x00,strlen(str_compare));
    memset(hex_compare,0x00,strlen(hex_compare));
    
    #if(DEBUG_ON != 0)
    printf("SCRIPT_LEN is %4d\n",len);
    #endif 
    
    memcpy(buff,script_line,len);
    /*去除可能的\r\n*/
    if((buff[len-1] == '\r') || ((buff[len-1] == '\n')))
    {
        buff[len-1] = 0x00;
        len-=1;
    }
    if((buff[len-1] == '\r') || ((buff[len-1] == '\n')))
    {
        buff[len-1] = 0x00;
        len-=1;
    }
    /*计算bootloader下载数据的长度*/
    // len = strlen(buff);

    #if(DEBUG_ON != 0)
    printf("SCRIPT_LEN is %4d\n",len);
    #endif 
  
    /*
    *以APDU=bf450200081392375533957469,0x9000为例
    *1.根据,将APDU和返回值进行拆分
    *  指令:APDU=bf450200081392375533957469
    *  返回值:,0x9000
    *  去掉APDU= 和 ,0x 保留数据部分
    * */
    #if(DEBUG_ON != 0)
    /*打印读取到的脚本信息*/
    printf("SCRIPT IS %s\n",buff);
    #endif 
    /*获取 , 分割符的地址,将指令和接收值分解为2个部分*/
    ptr = strstr(buff,division);
    #if(DEBUG_ON != 0)
    printf("The ptr is at position: %p\n", ptr);
    printf("The buff is at position: %p\n", &buff[5]);
    #endif
    /**/
    memcpy(str_cmd,&buff[5],ptr - &buff[5]);
    /*返回值从,开始,0x 三个字符*/
    memcpy(str_compare,ptr+3, &buff[len-1] - (ptr+2));    
    #if(DEBUG_ON != 0)
    printf("CMD: %s  LEN: %4d\n",str_cmd,ptr - &buff[5]);
    printf("REC: %s  LEN: %4d\n",str_compare,strlen(str_compare));
    #endif
    /*将其转换为HEX格式*/
    str_to_hex(str_cmd,strlen(str_cmd),hex_cmd);
	* send_len = (ptr - &buff[5]) >> 1;
	
	str_to_hex(str_compare,strlen(str_compare),hex_compare);
	* compare_len = strlen(str_compare) >> 1;

    memcpy(compare,hex_compare,*compare_len);
    
    /*将发送数据加头和XOR.*/
	//usb_format_to_spi_format(hex_cmd,send_len,send);
    memcpy(send,hex_cmd,*send_len);
    #if(DEBUG_ON != 0)
    printf("the send len  is %4d and the compare len is %4d\n",*send_len,*compare_len);
    #endif
    ret = 0;
    return ret;
}



int compare_respond_value(char * respond, char *comapare)
{
	int ret = 0;
	char xor = 0;
	int len =  0;
	int i =  0;
	const char cmd_xor_error[]  = {0x63 ,0x62 ,0x63 ,0x65 ,0x09 ,0x00 ,0x00 ,0x00 ,0x0e};
	len = respond[4] + respond[5] * 0X100;

	/*
	计算XOR
	*/
	for(i=0;i<len-1;i++)
		xor ^= respond[i];
	if (xor == respond[len-1])
	{
		;
	}  
	else
	{
		printf("xor error is %d\n",xor);
		return CMD_XOR_ERROR;
	}

	if(len == 9)
	{
		ret = memcmp(respond,comapare,9);
		if(ret)
		{
			printf("RESPOND_COMPARE_ERROR!\n");
			return RESPOND_COMPARE_ERROR;
		}
		else{
			printf("RESPOND_COMPARE_SUCCESS !\n");
			return RESPOND_COMPARE_SUCCESS;
		}
			

	}
	else
	{
		ret = memcmp(&respond[len-3],comapare,2);
		if(ret)
		{
			printf("RESPOND_COMPARE_ERROR!\n");
			hex_dump(&respond[len-3],2,2,"respond");

			return RESPOND_COMPARE_ERROR;
		}
		else{
			return RESPOND_COMPARE_SUCCESS;
		}
	}
	return  0;
}


long receive_script_respond(unsigned char *receive,ISTECCFunctionPointer_t * p)
{
    long ret  = 0; 
    long time = 0;
    long len  = 0;
    long i  = 0;
    long time_out_count  = 6; //The most delay 3*10 + 100 * 2 = 230ms
    const char dummy[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    const char rec_right[4] = {0x50,0x42,0x53,0x55};
    const char rec_error[4] = {0x63,0x62,0x63,0x65};
    struct timeval tv;
	struct timeval tv2;
    long delta;

    gettimeofday(&tv,NULL);
    /*读取第一包返回值
    判断返回值头是否正确
    如果正确-根据指令长度进行读取
    如果是63626365 后面没有数据了直接推出即可
    如果是00000000 说明指令在执行，返回的进行读取即可
    如果循环执行完毕--超时*/
    for(time=0;time<time_out_count;time++)
    {
        p->ISTECC512A_ReceiveOneMessage(receive,16);
        if(time > 3)
        {
            HSMMsDelay(300);
        }
        else
        {
            HSMMsDelay(2);
        }

        if(0 == memcmp(receive,rec_right,4))
        {
            ;
            printf("rec right .,break\n");
            break;
        }
        else if(0 == memcmp(receive,rec_error,4))
        {
            printf("return state error.!\n");
            hex_dump(receive,16,16,"rec_error");
            ret = 1;
            return ret;
        }
    }

    if(time == time_out_count)
    {
        printf("Timeout and don't get the right respoond!\n");
        ret =  2;
    }

    len = receive[4] + receive[5] * 0X100;
    time = (len + 15) / 0x10;

    #if 0
    printf("receive 4 is %d\n",receive[4]);
    printf("receive 5 is %d\n",receive[5]);
    printf("len is %d\n",len);
    printf("time is %d\n",time);
    #endif 

    if(len == 0)
    {
        printf("返回值长度错误,不能为0!\n");
        ret = 3;
    }
    else if(time == 1)
    {
        ret = 0;
    }
    else if( time > 1)
    {
        time -= 1; //减去首次读取的包
        i = 1;
        
        while(time--)
        {           
            p->ISTECC512A_ReceiveOneMessage(&receive[16*i],16);
            HSMUsDelay(1);
            i += 1;

            #if  0
            printf("time is %d\n",time);
            #endif

        }
         ret = 0;
    }

    //hex_dump(receive,len,16,"SPI_FORAMT_REC:");

    return ret;
}

long send_script_cmd(unsigned char *send, unsigned long send_len,ISTECCFunctionPointer_t * p)
{
    int ret  = 0; 
    /*SPI发送指令*/
    ret = p->ISTECC512A_SendOneMessage(send,send_len);
    return ret;
}
/*测试执行脚本-并判断其返回值
*输入脚本路径
*返回状态
*/
int test_exec_script(char * file_name)
{
    FILE  * fp;
    ISTECCFunctionPointer_t p;
    int count = 0;
    int ret; 
    char string[]  = "THIS IS A TEST";
    char line[1024];
    char send[1024];
	char compare[1024];
    int send_len;
    int compare_len;
    printf("the file is %s\n",file_name);

    FunctionPointerInit(&p);
    fp = fopen(file_name,"r");
    if(fp ==NULL)
    {
        printf("fi is NULL");
        return 1;
    }

    count = 0;
    /*读取一行数据*/
    memset(line,0x00,sizeof(line));
    while(fgets(line,100,fp) != NULL)
    {
        /*从,开始将脚本切为2个部分,指令+返回值*/
        /*执行操作:1.发送指令   2判断返回值是否和脚本一致*/
        memset(send,0x00,sizeof(send));
        memset(compare,0x00,sizeof(compare));
        printf("line %4d is %s\n",count,line);

        analysis_one_line_script(line,send,&send_len,compare,&compare_len);
        #if(DEBUG_ON != 0)
        printf("指令:%s\n",send);
        printf("响应比对:%s\n",compare_len);
        #endif
        count++;
    }
    fclose(fp);

    return 0;
}


/*analysis the script.
Modify the way of the script is parsed .Parse one line at one time,and send one .
This eliminates the need to read all the data at once,saving memory. 
*/
int script_analysis(char * file_name,char comapre_en)
{
	FILE * fp;
    int count = 0;
	int i = 0;
    int ret; 
    char string[]  = "script_analysis_test\n";
    
    /*Modify to a smaller memory.*/
    char line[1024];
    char send[1024];
	char receive[1024];
	char compare[1024];
    int send_len;
    int compare_len;

	struct timeval tv;
	struct timeval tv2;
    long delta;
	/*Create a pointer struct and Init it. */
  	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
 	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
    
    printf("the file is %s\n",file_name);
    unsigned char cod_guide[]={0x40,0x42,0x53,0x55,0x0e,0x00,0x00,0x00,0xbf,0x49,0x00,0x00,0x00,0xfc};
    fp = fopen(file_name,"r");
    if(fp ==NULL)
    {
        printf("fp is NULL");
        return 1;
    }

	gettimeofday(&tv,NULL);
	i = 0;
    memset(line,0x00,sizeof(line));
	while(fgets(line,1024,fp))
    {   	
		/*解析一行脚本将其拆分为发送值和返回对比值--发送值为SPI格式+头+长度+XOR*/
		analysis_one_line_script(line,send,&send_len,compare,&compare_len);
		/*发送指令*/
		send_script_cmd(send,send_len,&ISTECC512AFunctionPointerStructure);
		HSMUsDelay(5);
		/*接收响应值*/
		receive_script_respond(receive,&ISTECC512AFunctionPointerStructure);

		/*compare the respond*/
		if(comapre_en)
		{
			ret = compare_respond_value(receive,compare);
			if(ret!= RESPOND_COMPARE_SUCCESS)
			{
				printf("WRONG RESPOND!\n");
				hex_dump(receive,(receive[4] + receive[5] * 256),16,"receive:");
				hex_dump(compare,16,16,"COMPARE:");
				printf("SCRIPT IS %4d,COMPARE FAILED!\n",i);
				exit(1);
			}
		}
		i++;
        memset(line,0x00,sizeof(line));
    }
    
	gettimeofday(&tv2,NULL);
	delta = (tv2.tv_sec*1000000 + tv2.tv_usec) - (tv.tv_sec*1000000 + tv.tv_usec);
	printf("microsecond interval:%8ld MS\n",delta/1000);  //ms = us*1000
	printf("total line %4d\n",i);
	if(comapre_en)
	{
		printf("compare enable and success.\n");
	}

    printf("start guide hsm\n");
    send_script_cmd(cod_guide,sizeof(cod_guide),&ISTECC512AFunctionPointerStructure);
    HSMMsDelay(60);

    fclose(fp);
    return 0;
}



/*ADD NEW FUNCITON FOR NEW MODE APP*/
int script_analysis_for_bootloader(char * file_name,char comapre_en)
{
	FILE * fp;
    int count = 0;
	int i = 0;
    int ret; 
    char string[]  = "script_analysis_for_bootloader\n";
    
    /*Modify to a smaller memory.*/
    char line[2048];
    char send[2048];
	char receive[2048];
	char compare[2048];
    int send_len;
    int compare_len;

	struct timeval tv;
	struct timeval tv2;
    long delta;
	/*Create a pointer struct and Init it. */
  	ISTECCFunctionPointer_t ISTECC512AFunctionPointerStructure;
 	FunctionPointerInit(&ISTECC512AFunctionPointerStructure);
    printf("the file is %s\n",file_name);
    unsigned char cod_guide[]={0xbf,0x49,0x00,0x00,0x00};
    fp = fopen(file_name,"r");
    if(fp ==NULL)
    {
        printf("fp is NULL");
        return 1;
    }

	gettimeofday(&tv,NULL);
	i = 0;
    memset(line,0x00,sizeof(line));
	while(fgets(line,2048,fp))
    {   	
        printf("TEST READ LIEN SUCCESS\n");
		/*解析一行脚本将其拆分为发送值和返回对比值--发送值为SPI格式+头+长度+XOR*/
		analysis_one_line_script_for_bootloader(line,send,&send_len,compare,&compare_len);
		/*发送指令*/
		ISTECC512AFunctionPointerStructure.ISTECC512A_SendOneMessageOneShot(send,send_len);
        if(i==1)
		    HSMMsDelay(500);
        else
            HSMMsDelay(10);
		/*接收响应值.
        BF46 是生效COS.不需要读取返回值.也不需要比较。
        */
        if(send[1] != 0X46)
		    ISTECC512AFunctionPointerStructure.ISTECC512A_ReceiveOneMessage(receive,16);
		/*compare the respond*/
		if((comapre_en) && (send[1] != 0X46))
		{
			// ret = compare_respond_value(receive,compare);
            ret = memcmp(receive,compare,2);
			if(ret!= RESPOND_COMPARE_SUCCESS)
			{
				printf("WRONG RESPOND!\n");
				hex_dump(compare,16,16,"COMPARE:");
				printf("SCRIPT IS %4d,COMPARE FAILED!\n",i);
				exit(1);
			}
            else
            {
                printf("COMPARE SUCCESS!\n");
            }
		}
		i++;
        memset(line,0x00,sizeof(line));
        memset(send,0x00,sizeof(send));
        printf("TEST CURRENT LINE:%4d\n",i);
    }
    
	gettimeofday(&tv2,NULL);
	delta = (tv2.tv_sec*1000000 + tv2.tv_usec) - (tv.tv_sec*1000000 + tv.tv_usec);
	printf("microsecond interval:%8ld MS\n",delta/1000);  //ms = us*1000
	printf("total line %4d\n",i);
	if(comapre_en)
	{
		printf("compare enable and success.\n");
	}

    fclose(fp);
    return 0;
}
