/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
#ifndef __SCRIPT_DEAL_H__
#define __SCRIPT_DEAL_H__
#include<stdint.h>
/*SUPPORT SEND_ONE_MESSAGE AND REC_ONE_MESSAGE FUNCTION*/
#include "hsm_logic_level.h"

/*基础测试-读取脚本文件*/
int test_read_script(char * file_name);
/*基础测试-读取简本文件并逐行解析*/
int test_exec_script(char * file_name);

/*解析读取的脚本行
注意,除了9000等SW状态,7816协议的指令如果是读取返回值,那么最后一个字节为LE.
SPI线上发送数据:
头:40 42 53 55
长:LEN 00 00 00  头+长+数+尾
数:指令部分 
尾:头+长+数的XOR
SPI线上返回数据:
头:50 
*/
// int analysis_one_line_script(char * script_line,char *send,char *rec_buff);
// /**/
// int send_script_and_rec_return_value(char *send, int send_len,char *compare_return_value,int rec_len,ISTECCFunctionPointer_t * p);
/*评估需要的功能表
1.脚本文件读取
2.SPI协议解析
3.
*/
#define RESPOND_COMPARE_ERROR 	0X01
#define CMD_XOR_ERROR  		0X02
#define RESPOND_HEAD_ERROR 0X03
#define RESPOND_COMPARE_SUCCESS 0X00

int analysis_one_line_script(char * script_line,char *send,int * send_len, char *compare,int * compare_len);
int receive_script_respond(char *compare_return_value,ISTECCFunctionPointer_t * p);
int send_script_cmd(char *send, int send_len,ISTECCFunctionPointer_t * p);

int script_analysis(char * file_name,char comapre_en);
#endif
