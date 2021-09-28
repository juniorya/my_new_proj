/////////////////////
//
//  ...
//
/////////////////////


#ifndef __NET_REG_H__
#define __NET_REG_H__


#define RA_NET_CONNECT    1
#define RA_NET_TICK       2
#define RA_NET_FINISH     3


#ifdef __cplusplus
extern "C" {
#endif

unsigned char netInit(void);
unsigned char netEvent(unsigned char ucEvent);

#ifdef __cplusplus
}
#endif


#endif /* __NET_REG_H__ */
