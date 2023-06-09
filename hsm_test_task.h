/*
 * 	Copyright (c) 2023 qinxd
 *  qinxd
 *  email:qinxd@istecc.com
 *  Licensed under MIT License.
 * 
 */
#ifndef __HSM_TEST_TASK__
#define __HSM_TEST_TASK__

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
unsigned long ISTECCMODADDTest(void);
#endif
