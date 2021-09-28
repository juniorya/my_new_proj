/////////////////////
//
//  ...
//
/////////////////////


#ifndef __REG_ALGOTITHM_1_H__
#define __REG_ALGOTITHM_1_H__

#ifdef __cplusplus
extern "C" {
#endif

void regAlgInitialize(void);

void counterCtrl(unsigned char ucOn);
unsigned short counterCount(void);

unsigned char regAlgGetBufferToSend(unsigned char **ppBuf, unsigned short *pusLen);

#ifdef __cplusplus
}
#endif

#endif /* __REG_ALGOTITHM_1_H__ */
