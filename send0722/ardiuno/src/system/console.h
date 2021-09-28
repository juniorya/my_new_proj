//
// Console. 
//

#ifndef __CONSOLE_H_20200510__
#define __CONSOLE_H_20200510__

//typedef handler (to command processor)
struct console_ctx__;
typedef int (*console_command_line_handler)(void *pHandlerContext, struct console_ctx__ *pConsole, unsigned char *pbBuffer, unsigned uSize);

#define CONSOLE_BUF_SIZE 64
//#define CONSOLE_BUF_SIZE 2048
//#define CONSOLE_BUF_SIZE 255

typedef struct console_ctx__ {
    unsigned char ucConsoleType;
    void *pDevice;
    unsigned char pRxBuffer[CONSOLE_BUF_SIZE];
    unsigned char pTxBuffer[CONSOLE_BUF_SIZE];
    unsigned short usRxBufPos;
    unsigned short usTxBufPos;
    unsigned short usTxBufLast;
    unsigned uTxNow;
    unsigned uRxNow;
    unsigned uRxAvail;
    unsigned char ucState;

    console_command_line_handler fCommandLineHandler;
    void *pHandlerContext;
} console_ctx;

int __console_init(console_ctx *pCtx,  /* with line discipline or not */ console_command_line_handler fCommandLineHandler, void *pHandlerContext);
int __console_attach_serial(console_ctx *pCtx, void *pSerial);
int __console_print(console_ctx *pCtx, const char *psStr);
// I need first parameter void as I need to use it in data drain
int __console_print_pb(void *pCtx, const unsigned char *pbBuf, unsigned uLen);
int __console_tick(console_ctx *pCtx);

#endif /* __CONSOLE_H_20200510__ */

