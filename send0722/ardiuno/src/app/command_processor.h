//
// Command processor for pulser
//

#ifndef __COMMAND_PROCESSOR_H_20210621__
#define __COMMAND_PROCESSOR_H_20210621__

#include "../system/console.h"
#include "../system/data_supplier.h"

#define COMP_PROC_BUFFER_SIZE 128

typedef struct comp_process_ctx__ {
    unsigned char ucState;
    union {
        struct {
            unsigned short usTime; // in seconds
            unsigned long ulGot;
            unsigned long ulTO;
            unsigned char ucCommandResult;
        } meas;
        struct {
            unsigned long ulFreq;
            unsigned short usCount;
//            unsigned char ucCtrl;
        } gen;
        const char *pcMsg;
        struct {
            unsigned short usCode;
            const char *pcPrefix;
            const char *pcPostfix;
        } err;
    } data;
} comp_process_ctx;

typedef struct comp_ctx__ {
    //console_ctx *pCurrentConsole;
    data_supplier *pDrain;
    unsigned long ulPeriodicTO;
    comp_process_ctx processingCtx;
    unsigned short usBufferSize;
    unsigned char ucProcessing;
    unsigned char pbBuffer[COMP_PROC_BUFFER_SIZE];
} comp_ctx;

//typedef int (*console_command_line_handler)(void *pCompContext, struct console_ctx__ *pConsole, unsigned char *pbBuffer, unsigned uSize);
int comp_init(comp_ctx *pCtx, data_supplier *pDrain);
int comp_get_command(void *pCompContext, console_ctx *pConsole, unsigned char *pbBuffer, unsigned uSize);
int comp_tick(comp_ctx *pCtx);


#endif /* __COMMAND_PROCESSOR_H_20210621__ */

