/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
#ifndef __HSM_TEST_TASK__
#define __HSM_TEST_TASK__

#if 0
/*自签自验测试*/
unsigned int ISTECCSignVerifyTask(void);
/*测试数据存储和读取功能*/
unsigned int ISTECCSotreDataDeadData(void);
/*测试升级*/
unsigned int ISTECCEraseToUpdateCosTask(void);
/*多核验签测试*/
unsigned int ISTECCMulCoreVerifyTask(void);
/*点解压测试*/
unsigned int ISTESCCKeyDerivativeTestTask(void);
/*秘钥衍生功能*/
unsigned int ISTESCCKeyDerivativeTestTask(void);
/*SM4导入秘钥-加密-解密功能测试*/
unsigned int ISTESCCSM4EncryptAndDecodeTask(void);

/*支持INDEX的单条签名验签等功能测试*/
unsigned int ISTESCCIndexSignVerifyTask(void);

/*支持INDEX的加密解密功能*/
unsigned int ISTESCCIndexEncrpytAndDecryptTask(void);
#endif

unsigned long IS32U512AReadVerisonTest(void);
unsigned long IS32U512AFunctionTest(void);
unsigned long ISTECCEraseAPPTest(void);
unsigned long ISTECCUpdateTest(void);
unsigned long  ISTECCEraseAPPTest2(void);
unsigned long ISTECCSPointerDecompressTest(void);
unsigned long ISTECCKeyDeriveTest(void);
unsigned long ISTECCSm4CcmTest(void);
unsigned long  IS32U512ASM2EncyprtDecryptTest(void);
unsigned long  IS32U512ASm2VerifyEvalueWithPubKeyIndex(void);
/*E+RS verify and  e sign.*/
unsigned long  IS32U512ASm2SignEvalueAndVerifyEvalueWithPubKeyIndex(void);
#endif
