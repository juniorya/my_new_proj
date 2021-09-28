//
//
//

#include "console.h"
#include "serial.h"
#include <string.h>

int __console_init(console_ctx *pCtx,  /* with line discipline or not */ console_command_line_handler fCommandLineHandler, void *pHandlerContext)
{
    pCtx->pDevice = 0;
    pCtx->fCommandLineHandler = fCommandLineHandler;
    pCtx->pHandlerContext = pHandlerContext;
    pCtx->usRxBufPos = 0;
    pCtx->usTxBufPos = 0;
    pCtx->usTxBufLast = 0;
    pCtx->uTxNow = 0;
    pCtx->uRxNow = 0;
    pCtx->uRxAvail = 0;
    pCtx->ucState = 0;

    return 1;
}

int __console_attach_serial(console_ctx *pCtx, void *pSerial)
{
    pCtx->pDevice = pSerial;

// TODO: check, but I think it is reasonable
    pCtx->ucState = 0;
    return 1;
}

#define MAX_TX_CHUNK_SIZE 16

int __console_int_process_tx(console_ctx *pCtx)
{
    int rc = 0;
    unsigned uToTx = 0;
    switch(pCtx->ucState & 0x80) {
        case 0: // sending not started
ciptx_1:
            if (pCtx->usTxBufPos != pCtx->usTxBufLast) {
                uToTx = (pCtx->usTxBufPos < pCtx->usTxBufLast)? 
                    pCtx->usTxBufLast - pCtx->usTxBufPos : (sizeof(pCtx->pTxBuffer) + pCtx->usTxBufLast - pCtx->usTxBufPos);

                if (uToTx > MAX_TX_CHUNK_SIZE) uToTx = MAX_TX_CHUNK_SIZE;

                if ((pCtx->usTxBufPos + uToTx) >= sizeof(pCtx->pTxBuffer)) uToTx = sizeof(pCtx->pTxBuffer) - pCtx->usTxBufPos;

                pCtx->uTxNow = uToTx;
                rc = serial_write(pCtx->pDevice, pCtx->pTxBuffer + pCtx->usTxBufPos, &(pCtx->uTxNow));
                if (rc == 1) {
                    pCtx->ucState |= 0x80;
                } else return rc;
             } else return 0;
        case 0x80:
            rc = serial_ready(pCtx->pDevice, SERIAL_OPERATION_TX);
            if (rc != 0) {
                pCtx->ucState &= 0x7F;
                if (rc == 1) {
                    pCtx->usTxBufPos += pCtx->uTxNow;
                    if (pCtx->usTxBufPos >= sizeof(pCtx->pTxBuffer))
                        pCtx->usTxBufPos -= sizeof(pCtx->pTxBuffer);
                    goto ciptx_1;
                } else {
// TODO: push data to log
                    return rc;
                }
            }
    }
    return 0;
}

// Attention! sending must fit to buffer completely.
int __console_print_pb(void *pV, const unsigned char *pbBuf, unsigned uLen)
{
    console_ctx *pCtx = (console_ctx *)pV;
    __console_int_process_tx(pCtx);

    unsigned uAvail, uMaxLen = CONSOLE_BUF_SIZE - 1;
    if (uLen > uMaxLen) return -1;

    uAvail = (pCtx->usTxBufPos < pCtx->usTxBufLast)? sizeof(pCtx->pTxBuffer) : 0;
    uAvail = uAvail + pCtx->usTxBufPos - pCtx->usTxBufLast - 1;
    if (uLen > uAvail) return 0;

    if ((pCtx->usTxBufLast + uLen) <= sizeof(pCtx->pTxBuffer)) {
        memcpy(pCtx->pTxBuffer + pCtx->usTxBufLast, pbBuf, uLen);
        pCtx->usTxBufLast += uLen;
        if (pCtx->usTxBufLast >= sizeof(pCtx->pTxBuffer)) pCtx->usTxBufLast -= sizeof(pCtx->pTxBuffer);
    } else {
        unsigned uChunk = sizeof(pCtx->pTxBuffer) - pCtx->usTxBufLast;
        memcpy(pCtx->pTxBuffer + pCtx->usTxBufLast, pbBuf, uChunk);
        pCtx->usTxBufLast = uLen - uChunk;
        memcpy(pCtx->pTxBuffer, pbBuf + uChunk, pCtx->usTxBufLast);
    }
    __console_int_process_tx(pCtx);
    return 1;
}

// Attention! string must fit to buffer completely.
int __console_print(console_ctx *pCtx, const char *psStr)
{
    // try to push string
    unsigned uLen = strlen(psStr);
    return __console_print_pb(pCtx, (const unsigned char *)psStr, uLen);
}

#define RX_STATE(pCtx, state) { pCtx->ucState &= 0xF8; pCtx->ucState |= state; }

// read from source (serial etc)
// process symbols, send echo
// invoke callback (command processor)
int console_int_process_input(console_ctx *pCtx)
{
    int i, rc = 0;
    unsigned char c, pc[3];

    if (pCtx->pDevice) {
        switch(pCtx->ucState & 0x7) {
            case 0: // start to check availability
cipi_0:
                pCtx->uRxAvail = 0;
                i = serial_available(pCtx->pDevice, &(pCtx->uRxAvail));
                if (i <= 0) break;
                RX_STATE(pCtx, 1);
            case 1:
                i = serial_ready(pCtx->pDevice, SERIAL_OPERATION_AVAIL);
                if (i == 0) break;
                if (i < 0) { RX_STATE(pCtx, 0); break; }
                RX_STATE(pCtx, 2);
            case 2:
cipi_1:
                if (pCtx->uRxAvail == 0) { RX_STATE(pCtx, 0); rc = 1; break; }
                pCtx->uRxNow = 1;
                i = serial_read(pCtx->pDevice, pCtx->pRxBuffer + pCtx->usRxBufPos, &(pCtx->uRxNow));
                if (i == 0) break;
                if (i < 0) {
                    RX_STATE(pCtx, 0);  break;
                }
                RX_STATE(pCtx, 3);
            case 3:
                i = serial_ready(pCtx->pDevice, SERIAL_OPERATION_RX);
                if (i == 0) break;
                if (i < 0) { RX_STATE(pCtx, 0); break; }

                RX_STATE(pCtx, 2);
                if (pCtx->uRxAvail > 0) pCtx->uRxAvail--;
                c = pCtx->pRxBuffer[pCtx->usRxBufPos];

                if ((c == '\r') || (c == '\n')) {
                    pc[0] = '\r'; pc[1] = '\n';

                    __console_print_pb(pCtx, pc, 2);
                    if (pCtx->fCommandLineHandler) {
                        if (!(pCtx->fCommandLineHandler(pCtx->pHandlerContext, pCtx, pCtx->pRxBuffer, pCtx->usRxBufPos))) {
                            RX_STATE(pCtx, 4); break;
                        }
                    }
                    pCtx->usRxBufPos = 0;
                } else
                if (c == 8) { // backspace
                    if (pCtx->usRxBufPos > 0) {
                        pc[0] = c; pc[1] = ' '; pc[2] = 8;
                        pCtx->usRxBufPos--;
                        __console_print_pb(pCtx, pc, 3);
                    }
                } else {
                    if (pCtx->usRxBufPos < sizeof(pCtx->pRxBuffer)) {
                        pCtx->usRxBufPos++;
                        __console_print_pb(pCtx, &c, 1);
                    }
                }
// TODO: granularity, maximum iAvail for more smooth cooperative tasks scheduling
                if (pCtx->uRxAvail & 1) goto cipi_1;
                rc = 1;
                break;
            case 4:
                    if (pCtx->fCommandLineHandler) {
                        if (!(pCtx->fCommandLineHandler(pCtx->pHandlerContext, pCtx, pCtx->pRxBuffer, pCtx->usRxBufPos))) break;
                    }
                    pCtx->usRxBufPos = 0;
                    RX_STATE(pCtx, 0); goto cipi_0;

        }
    }
    return rc;
}

int __console_tick(console_ctx *pCtx)
{
    // TODO: return with understanding the process highly probably can sleep...
     __console_int_process_tx(pCtx);
    return console_int_process_input(pCtx);
}
