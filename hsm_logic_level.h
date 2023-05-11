/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
/**
 ******************************************************************************
 * File name: hsm_logic_level.c
 * Description:
 * Auth : qin xiaodong
 * Email: qinxd@istecc.com
 * API VERSION: 0.3
 * Time : 2022-7-22
 * note : HSM_ means : high secure module.
 ******************************************************************************/
#ifndef __HSM_LOGIC_LEVEL_H__
#define __HSM_LOGIC_LEVEL_H__

#define LINUX_SYS       1
#define KEIL_MDK        2
#define CURRENT_SYS LINUX_SYS
/*error code*/
#define LEN_OVER_ERROR          0X03   /*The max length of send data is  1036.Plese don't send the big length*/
#define LEN_ALIGN_ERROR         0X04  /*Align error. All of the command and date 4 byte align*/
#define TIME_OUT_ERROR          0X10   /*Time out error*/
#define MODE_NOT_SUPPORT        0X20 /*This command is not suppot*/

#define ISTECC_MODULE_MAX_MESSAGE_LEN 1036 /*Max length of data*/

#define SM4_ENCRYPT_MODE 0X01 /*SM4 encrypt mode macro*/
#define SM4_DECRYPT_MODE 0X02 /*SM4 decrypt mode macro*/
#define SM2_ENCRYPT_MODE 0X01 /*SM2 encrypt mode macro*/
#define SM2_DECRYPT_MODE 0X02 /*SM2 decrypt mode macro*/

// E hash verify. E = HASH(ZA || DataInput)  and ZA = HASH(ENTLA || IDA|| .......)
// RS Signature  R:32 byte S:32byte
// M  Abbreviation  of  message. Original text
// PUBKEY public key of SM2, 64 byte,it's a pointer (Gx,Gy)
// IDA 

#define IS32U512A_SM2_MODULE_CMD_LEN          6           /*Length of command */
#define IS32U512A_SM4_KEY_LEN                 0X10        /*Length of SM4  algorithm's key.*/
#define IS32U512A_SM2_DATA_LEN_H_OFFSET       3           /* Parameters of the command . Don't care about it */
#define IS32U512A_SM2_DATA_LEN_L_OFFSET       2           /* Parameters of the command . Don't care about it */
#define IS32U512A_SM2_MODULE_CMD_INDEX_OFFSET 4           /*the command of index offset*/
#define IS32U512A_SM2_PUBKEY_LEN              64          /*The public  key of sm2*/
#define IS32U512A_SM2_GX_LEN                  32
#define IS32U512A_SM2_GY_LEN                  32  
#define IS32U512A_SM2_PRIKEY_LEN              32          /*The private key of sm2*/


/*Signature extension private key index and KDF pri is in 14-2*/
#define IS32U512A_CONST_SIGN_FACTOR_INDEX    14  
/*encrypt extension private key  index, and KDF pri is in 15-2*/
#define IS32U512A_CONST_DECRYPT_FACTOR_INDEX 15

/*error code 2022-7-25 */
#define ERROR_DECOMPRESS_MODE_NO_SUPPORT     0X04

/*Result of SM2 signature. 64 byte
Part1   R   32Byte
PART2   S	32Byte*/
#define SM2_R_LEN 32
#define SM2_S_LEN 32

/*cal result*/
#define sucess 0
#define fail 1

/*Unit: us*/
#define VERIFY_DELAY 400

/**
 * @brief
 *  Don't care about it .
 */
// typedef enum{
// 	ISTECC_VERIFY_E_RS = 0,   //
// 	ISTECC_VERIFY_M_RS = 1,
// 	ISTECC_VERIFY_E_RS_PUBKEY = 2,
// 	ISTECC_VERIFY_M_RS_PUBKEY = 3,
// 	ISTECC_VERIFY_M_RS_PUBKEY_ID = 4, //有可能是最多的用法。传入M + RS + PUBKEY + ID
// }ISTECVerifyMode_t;
/**
 * @brief
 * Don't care about it .
 */
// typedef struct
// {
// 	unsigned char * message; 	//消息或者预处理值的指针
// 	unsigned int    message_len;//消息的长度
// 	unsigned char * id;			//ID指针
// 	unsigned int    id_len; 	//ID的长度
// 	unsigned char * rs; 		//签名结果
// 	unsigned char * pubkey; 	//公钥
// 	ISTECVerifyMode_t    	verify_mode;//验签模式
// 	unsigned int    		result; 	//验签结果.暂时不用
// }ISTECCVerifyMessageInfo_t, * ISTECCVerifyMessageInfoPointer_t;

typedef enum
{
  ISTECC_POINT_DECOMPRESS_2 = 2,
  ISTECC_POINT_DECOMPRESS_3 = 3
} ISTECCPointDecompressMode_t;

/*Define all function pointer of sm2-module */
typedef struct
{
  /*The index of SM2 KEY is (0-5)*/
  /*import the public key of SM2 . G(x,y).. total 64 byte.*/
  unsigned long (*ISTECC512A_SM2ImportPubkey)(unsigned long index, unsigned char *pubkey_x, unsigned char *pubkey_y);
  /*import the private key of SM2  32byte*/
  unsigned long (*ISTECC512A_SM2ImportPrikey)(unsigned long index, unsigned char *prikey_d);
  /*message  signature*/
  unsigned long (*ISTECC512A_SM2SignMessage)(unsigned long prikey_index, unsigned char *p_org_data, unsigned long org_len, unsigned char *p_sign_data);
  /*message verify*/
  unsigned long (*ISTECC512A_SM2SingleVerifyMessage)(unsigned long pubkey_index, unsigned char *p_org_data, unsigned long org_len, unsigned char *p_sign_data);
  /*gen a sm2  keypair*/
  unsigned long (*ISTECC512A_SM2GenKeyPair)(unsigned long index);
  /*export sm2 public key*/
  unsigned long (*ISTECC512A_SM2ExportPubkey)(unsigned long index, unsigned char *pubkey_x, unsigned char *pubkey_y);
  /*export sm2 private key*/
  unsigned long (*ISTECC512A_SM2ExportPrikey)(unsigned long index, unsigned char *prikey_d);
  /* set IDa*/
  unsigned long (*ISTECC512A_SM2SetID)(unsigned long index, unsigned char *sm2_id, unsigned char length);
  /*export IDa*/
  unsigned long (*ISTECC512A_SM2ExportID)(unsigned long index, unsigned char *sm2_id, unsigned char length);

  /* read app version. first relese verison is 1.8.7. if you update suceess .
  *  the current verison is .1.8.8  2022-7-25 10:55*/
  unsigned long (*ISTECC512A_CosVersionRead)(unsigned char *version);

  /*PIN COMFIRM*/
  unsigned long (*ISTECC512A_PinConfirm)(unsigned char *pin_value, unsigned long len_of_pin);
  /*PIN cancel confirm*/
  unsigned long (*ISTECC512A_PinConfirmCancel)(unsigned char *pin_value, unsigned long len_of_pin);
  /*PIN change */
  unsigned long (*ISTECC512A_PinChange)(unsigned char *old_pin_value, unsigned char *new_pin_value, unsigned long len_of_pin);

  /* the app is a FSM.(finite state machine). it have three state : (1)receive instruction(Mster send date)->(2)deal-->(3)ready to return result(Master read date). 
  * If you can't confirm the current state.execute this fuction will take you to  state(1). If this program fails, it is generally a communication problem.*/
  unsigned long (*ISTECC512A_StatusSync)(void);

  /*SM2加密.加密结果长度为 原文长度+96 */
  unsigned long (*ISTECC512A_SM2Encrypt)(unsigned long index, unsigned char *p_org_data, unsigned long len, unsigned char *encrypt_result);
  /*SM2解密,传入参数len为 加密后包含C1和C3的长度 解密结果为和原文对照的长度*/
  /*SM2 a,传入参数len为 加密后包含C1和C3的长度 解密结果为和原文对照的长度*/
  unsigned long (*ISTECC512A_SM2Decrypt)(unsigned long index, unsigned char *encrypt_result, unsigned long len, unsigned char *p_org_data);
  /*read uuid 13byte*/
  unsigned long (*ISTECC512A_FactoryNumberRead)(unsigned char *number);

  /*SM4导入秘钥 .is32u512a reserved*/
  unsigned long (*ISTECC512A_SM4ImportKey)(unsigned long index, unsigned char *key);
  /*SM4加密 .is32u512a reserved*/
  unsigned long (*ISTECC512A_SM4Encrypt)(unsigned long index, unsigned char *in, unsigned long len, unsigned char *out);
  /*SM4解密  .is32u512a reserved*/
  unsigned long (*ISTECC512A_SM4Decrypt)(unsigned long index, unsigned char *in, unsigned long len, unsigned char *out);

  /*erase current app. if you reset the module.it will return to bootloaer.now you can execute .ISTECC512A_APPUpdate
  * this function need you pass ISTECC512A_PinConfirm */
  unsigned long (*ISTECC512A_APPErase)(void);
  /*if you has execute  ISTECC512A_APPErase success. now you can update it . the spi speed < 1Mbps. 
  * the flow is 
  * 1> ISTECC512A_PinConfirm
  * 2> ISTECC512A_APPErase
  * 3> reset the modlue. and set spi speed < 1Mbps.
  * 4> ISTECC512A_APPUpdate
  * */
  unsigned long (*ISTECC512A_APPUpdate)(void);
  /*pointer decompress.*/
  unsigned long (*ISTECC512A_SM2PointerDecompress)(unsigned char *gx, ISTECCPointDecompressMode_t mode,unsigned char *pubkey);
    /*complete privite key derivative. the c value get from PCA.*/
   unsigned long (*ISTECC512A_SM2CompleteKDF)( unsigned long in_index, unsigned long out_index, unsigned long i, unsigned long j,unsigned char * ks,unsigned char *c);
  /*encrypt key derivative and  sign key derivative.*/
   unsigned long (*ISTECC512A_SM2EncryptSignKeyKDF)( unsigned long in_index, unsigned long out_index, unsigned long i, unsigned long j,unsigned char * ke);

  /*the new add funciton.1.8.8.5*/

  unsigned long (*ISTECC512A_DeviceKeyDeriveFlowInit)(unsigned char *kS,unsigned char *kE,unsigned char *A, unsigned char *P);
  unsigned long (*ISTECC512A_GetRandom)(unsigned char *buff,unsigned long len);
  unsigned long  (*ISTECC512A_KDFGetbij)(unsigned char *kS,unsigned long i , unsigned long j);
  unsigned long  (*ISTECC512A_KDFGetqij)(unsigned char *kE,unsigned long i , unsigned long j);
   unsigned long (*ISTECC512A_KDFGetsij)(unsigned char * seed_a,
      unsigned char *kS,unsigned int iINT,
      unsigned int jINT,char * c,
      unsigned char * sij);
  //unsigned long  (*ISTECC512A_KDFGetsij)(char *sm4,char * in,char *out);
  unsigned long (*ISTECC512A_ModAdd)(unsigned char *bij,unsigned char *c,unsigned char *out_sij);
  /*********resever************/
  /*Data store  reserved*/

  /*Data read reserved*/

  /*2022-11-8 15:51 add   provide E+RS verify mode*/
  unsigned long (*ISTECC512A_SM2VerifyEValueWithPubkeyIndex)(unsigned long pubkey_index,unsigned char *e,unsigned char *rs);

    /*2022-11-23 add  e value sign*/
  unsigned long (*ISTECC512A_SM2SignEValue)(unsigned long prikey_index, unsigned char *e, unsigned char *p_sign_data);

   /*2023-5-9 add one key restore.*/
  unsigned long (*ISTECC512A_Restore)(void);

 /*2023-5-10 add SendOneMessage and RecOneMessage for Upgrade tools*/
 unsigned long (*ISTECC512A_SendOneMessage)(unsigned char *send, unsigned long send_len);
 unsigned long (*ISTECC512A_ReceiveOneMessage)(unsigned char * rec,unsigned long rec_len);

} ISTECCFunctionPointer_t;

/*Don't care about it. The funciton has init the pointer .*/
unsigned long FunctionPointerInit(ISTECCFunctionPointer_t *p);



int HSMSempohreInit(void);
int  HSMThreadMutexInit(void);
int HSMSempohreDeInit(void);
#endif
