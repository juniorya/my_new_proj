//
//
//

#include "command_processor.h"
//#include "manager.h"
//#include "modules/eeprom_module.h"
#include "../arch/arch.h"
#include "../net_reg.h"

#define UL_POSITIVE(ul) ((ul) < (((unsigned long)(-1))>>1))

#define PROCESSING_POSTMESSAGE 1
#define PROCESSING_ERROR       2

#define PROCESSING_MEAS        3

//#define PROCESSING_GET         3
//#define PROCESSING_SET         4
//#define PROCESSING_POLL        5
//#define PROCESSING_EEGET       6
//#define PROCESSING_EESET       7

// TODO: remove!
void dd(const char *pc);
void db(const unsigned char *pb, unsigned l);
void dc(unsigned char c);

const char *pcMsgBadCommand = "Unrecognized command. Use \"help\" to check.\r\n";
const char *pcMsgPeriodicStoped = "Stoped.\r\n";
const char *pcMsgHelp = "Commands: help, meas\r\n";
const char *pcRN = "\r\n";

// forwards
static int is_space(char c);
static int error_cont(comp_ctx *pCtx);
static int extract_token(unsigned char *pbBuffer, unsigned short usLen, unsigned short *pusStart, unsigned short *pusFinish);
static int check_token_by_options(unsigned char *pbToken, unsigned short usLen, char **ppOptions, unsigned char ucOptionsCount, unsigned char *pusResult);
static int check_token_digit(unsigned char *pbToken, unsigned short usLen, long *plResult);

// commands forwards
// help
// meas 60 (measure pulses within number of seconds)
//
static int command_help(comp_ctx *pCtx);

static int command_meas(comp_ctx *pCtx, unsigned char *pbBuffer, unsigned short usLen);
static int command_meas_cont(comp_ctx *pCtx);

 
int comp_init(comp_ctx *pCtx, data_supplier *pDrain)
{
    //pCtx->pCurrentConsole = 0;
    pCtx->pDrain = pDrain;
    pCtx->usBufferSize = 0;
    pCtx->ucProcessing = 0;
    return 1;
}

// (maybe even const char *)
int comp_get_command(void *pCompContext, console_ctx *pConsole, unsigned char *pbBuffer, unsigned uSize)
{
    int rc = 1;
    unsigned char ucOption;
    unsigned short usS, usF = 0;
    char *ppCommands[] = { "help", "meas" };
    comp_ctx *pCC = (comp_ctx *)pCompContext;

    if (pCC->ucProcessing) {
        if (pCC->ucProcessing == PROCESSING_POSTMESSAGE) return 0;

        pCC->ucProcessing = 0;
        //__console_print(pConsole, pcMsgPeriodicStoped);

        if (data_supplier_available(pCC->pDrain) > 0) {
            data_supplier_push_static_string(pCC->pDrain, pcMsgPeriodicStoped);
        } else {
            pCC->ucProcessing = PROCESSING_POSTMESSAGE;
            pCC->processingCtx.data.pcMsg = pcMsgPeriodicStoped;
        }
    } else {
//        if (!data_supplier_empty(pCC->pDrain)) return 0;
        if (!(data_supplier_available(pCC->pDrain))) return 0;

        if ((!extract_token(pbBuffer, uSize, &usS, &usF)) ||
            (!check_token_by_options(pbBuffer + usS, usF - usS, ppCommands, (sizeof(ppCommands) / sizeof(char *)), &ucOption))) {

            data_supplier_push_static_string(pCC->pDrain, pcMsgBadCommand);
            return 1;
        }

        // from this point we know that the command is going to process, so the function will not return 0
        switch(ucOption) {
            case 0: rc = command_help(pCC); break;
            case 1: rc = command_meas(pCC, pbBuffer + usF, uSize - usF); break;
            default:
                data_supplier_push_static_string(pCC->pDrain, ppCommands[ucOption]);
                data_supplier_push_static_string(pCC->pDrain, " got. READY!\r\n");
        }
        if (rc == 0) rc = 1;
    }
    return rc;
}

#define BREAK_ON_NA(pCC) if (data_supplier_available(pCC->pDrain) == 0) break;

unsigned char cnt = 0, cnt1 = 0;
extern console_ctx cs;

int comp_tick(comp_ctx *pCtx)
{
    int rc = 1;
    switch(pCtx->ucProcessing) {
        case PROCESSING_POSTMESSAGE:
            if (data_supplier_empty(pCtx->pDrain)) pCtx->ucProcessing = 0;
            break;
        case PROCESSING_ERROR:
            rc = error_cont(pCtx);
            break;
        case PROCESSING_MEAS:
            rc = command_meas_cont(pCtx);
            break;
        default: rc = 0;
    }
    return rc;
}

static int is_space(char c)
{
    return ((c == ' ') || (c == '\t'));
}

static int extract_token(unsigned char *pbBuffer, unsigned short usLen, unsigned short *pusStart, unsigned short *pusFinish)
{
    unsigned short u;
    for(u = 0; u < usLen; u++) {
        if (!is_space((char)(*(pbBuffer + u)))) {
            *pusStart = u;
            for(; u < usLen; u++) {
                if (is_space((char)(*(pbBuffer + u)))) {
                    *pusFinish = u;
                    return 1;
                }
            }
            *pusFinish = usLen;
            return 1;
        }
    }
    return 0;
}

static int check_token_by_options(unsigned char *pbToken, unsigned short usLen, char **ppOptions, unsigned char ucOptionsCount, unsigned char *pusResult)
{
    unsigned char x, y;
    for(x = 0; x < ucOptionsCount; x++) {
        unsigned char *pB = (unsigned char *)(ppOptions[x]);
        for(y = 0; y < usLen; y++) {
            if (pB[y] != pbToken[y]) break;
        }
        if ((y == usLen) && (pB[y] == 0)) {
            *pusResult = x;
            return 1;
        }
    }
    return 0;
}

static int check_token_digit(unsigned char *pbToken, unsigned short usLen, long *plResult)
{
    unsigned char x, state = 0, minus = 0;
    long l = 0;

    for(x = 0; x < usLen; x++) {
        unsigned char c = pbToken[x];
        switch(state) {
            case 0: // first symbol. "-", "0x" etc are vaild here
                if (c == '-') {
                    minus = 1; state = 10; break;
                } else 
                if (c == '0') {
                    state = 1; break;
                } else
                if ((c > '0') && (c <= '9')) {
                    l = (c - 48); state = 10; break;
                }
                return 0;

            case 1: // 'x' means we will parse hexadecimal. digit means we're parsing just decimal
                if (c == 'x') {
                    state = 16; break;
                } else
                if ((c >= '0') && (c <= '9')) {
                    l = (c - 48); state = 10; break;
                }
                return 0;

            case 10: // parsing decimal
                if ((c >= '0') && (c <= '9')) {
                    l = (l * 10) + (long)(c - 48); break;
                }
                return 0;

            case 16: // parsing hexadecimal
                if ((c >= '0') && (c <= '9')) {
                    l = (l << 4) + (long)(c - 48); break;
                } else
                if ((c >= 'A') && (c <= 'F')) {
                    l = (l << 4) + (long)(c - 55); break;
                } else
                if ((c >= 'a') && (c <= 'f')) {
                    l = (l << 4) + (long)(c - 87); break;
                }
                return 0;
        }

    }
    *plResult = (minus)? (-l) : l;
    return 1;
}

// TODO: create error task
static int error_extract_argument(comp_ctx *pCtx, unsigned char ucArg)
{
    pCtx->ucProcessing = PROCESSING_ERROR;
    pCtx->processingCtx.ucState = 0;
    pCtx->processingCtx.data.err.pcPrefix = "Error extract argument ";
    pCtx->processingCtx.data.err.usCode = ucArg;
    pCtx->processingCtx.data.err.pcPostfix = 0;
    return error_cont(pCtx);
}

static int bad_argument(comp_ctx *pCtx, unsigned char ucArg)
{
    pCtx->ucProcessing = PROCESSING_ERROR;
    pCtx->processingCtx.ucState = 0;
    pCtx->processingCtx.data.err.pcPrefix = "Bad argument ";
    pCtx->processingCtx.data.err.usCode = ucArg;
    pCtx->processingCtx.data.err.pcPostfix = 0;
    return error_cont(pCtx);
}

static int error_cont(comp_ctx *pCtx)
{
    switch(pCtx->processingCtx.ucState) {
        case 0: BREAK_ON_NA(pCtx);
                data_supplier_push_static_string(pCtx->pDrain, pCtx->processingCtx.data.err.pcPrefix);
                pCtx->processingCtx.ucState++;
        case 1: BREAK_ON_NA(pCtx);
                data_supplier_push_char_num(pCtx->pDrain, pCtx->processingCtx.data.err.usCode);
                pCtx->processingCtx.ucState++;
        case 2: BREAK_ON_NA(pCtx);
                data_supplier_push_static_string(pCtx->pDrain, (pCtx->processingCtx.data.err.pcPostfix)? 
                    pCtx->processingCtx.data.err.pcPostfix : pcRN);
                pCtx->ucProcessing = 0;
    }
    return 1;
}

static int command_help(comp_ctx *pCtx)
{
    data_supplier_push_static_string(pCtx->pDrain, pcMsgHelp);
    return 1;
}

#define TOKENIZE_DIGIT(lVal, ucArgNum) \
    pbP += usF; if (usF > usL) return error_extract_argument(pCtx, ucArgNum); \
    usL -= usF; \
    if (!extract_token(pbP, usL, &usS, &usF)) return error_extract_argument(pCtx, ucArgNum); \
    if (!check_token_digit(pbP + usS, usF - usS, &lVal)) return error_extract_argument(pCtx, ucArgNum)

static int command_meas(comp_ctx *pCtx, unsigned char *pbBuffer, unsigned short usLen)
{
    long lTmp;
    unsigned char *pbP = pbBuffer;
    unsigned short usL = usLen, usS, usF = 0;
    unsigned long ulNow;

    TOKENIZE_DIGIT(lTmp, 1);
    if (lTmp <= 0) return bad_argument(pCtx, 1);

    ulNow = __micros();
    counterCtrl(1);    // we're started!

    pCtx->ucProcessing = PROCESSING_MEAS;
    pCtx->processingCtx.ucState = 0x80; // means time is on!
    pCtx->processingCtx.data.meas.usTime = lTmp;
    pCtx->processingCtx.data.meas.ulGot = 0;
    pCtx->processingCtx.data.meas.ulTO = ulNow + (lTmp * 1000000);
    pCtx->usBufferSize = 0;

    pCtx->processingCtx.data.meas.ulGot = 0;

    return command_meas_cont(pCtx);
}

void counterCtrl(unsigned char ucOn);
unsigned short counterCount(void);

static int command_meas_cont(comp_ctx *pCtx)
{
    int rc = 0;
    unsigned long ulCounted;
//    unsigned uSize;

    if (pCtx->processingCtx.ucState & 0x80) {
        if (UL_POSITIVE(__micros() - pCtx->processingCtx.data.meas.ulTO)) {
            counterCtrl(0);    // stoped!
            pCtx->processingCtx.ucState &= 0x7F;
        }
    }
    switch(pCtx->processingCtx.ucState & 0x7F) {
        case 0: BREAK_ON_NA(pCtx);
                data_supplier_push_static_string(pCtx->pDrain, "GOT: ");
                pCtx->processingCtx.ucState++;
        case 1: BREAK_ON_NA(pCtx);
                data_supplier_push(pCtx->pDrain, PUSHER_SHORT, &(pCtx->processingCtx.data.meas.usTime), 2);
                pCtx->processingCtx.ucState++;
        case 2: BREAK_ON_NA(pCtx);
                data_supplier_push_static_string(pCtx->pDrain, "\r\n");
                pCtx->processingCtx.ucState++;
        case 3: BREAK_ON_NA(pCtx);
                pCtx->processingCtx.ucState++;
        case 4:
cmc_4:
                netEvent(RA_NET_TICK);
                ulCounted = counterCount();
                if (ulCounted != pCtx->processingCtx.data.meas.ulGot) {
                    pCtx->processingCtx.data.meas.ulGot = ulCounted;
                    pCtx->processingCtx.ucState++;
                } else {
                    if (!(pCtx->processingCtx.ucState & 0x80)) {
                        pCtx->processingCtx.ucState += 2;
                        goto cmc_6;
                    }
                    break;
                }
        case 5: BREAK_ON_NA(pCtx);
// TODO: lONG
//                data_supplier_push(pCtx->pDrain, PUSHER_SHORT, &(pCtx->processingCtx.data.meas.ulGot), 2);
                pCtx->processingCtx.ucState--;
                goto cmc_4;

// TODO: start measurements
/*                uSize = sizeof(pCtx->pbBuffer);
                rc = process_modbus_command(pCtx->pModbusService, pCtx->processingCtx.data.mdb.usSlave, 3, 
                    pCtx->processingCtx.data.mdb.usAddress, pCtx->processingCtx.data.mdb.usCount, pCtx->pbBuffer, &uSize);
                if (rc == 1) {
                    pCtx->usBufferSize = uSize; //?
                    // modbus returns buffer included with command and amount of bytes fields (2 bytes), so exclude them
                    data_supplier_push(pCtx->pDrain, PUSHER_HEXSHORTS_LZ0X, pCtx->pbBuffer + 2, pCtx->usBufferSize - 2);
                    pCtx->processingCtx.ucState += 2;
                    goto cgpc_3;
                } else {
                    pCtx->processingCtx.data.mdb.ucCommandResult = -rc;
                    data_supplier_push_static_string(pCtx->pDrain, "Error processing command - ");
                    pCtx->processingCtx.ucState++; // one more ++, as we'll add "\r\n" after buffer with data
                }
                rc = 0;
*/
        case 6:
cmc_6:
                BREAK_ON_NA(pCtx);
//                pCtx->processingCtx.data.meas.ucCommandResult = -1;
//                    data_supplier_push_static_string(pCtx->pDrain, "Error processing command - ");
                    data_supplier_push_static_string(pCtx->pDrain, "Complete. Got ");
                    pCtx->processingCtx.ucState++; // one more ++, as we'll add "\r\n" after buffer with data
        case 7: BREAK_ON_NA(pCtx);
//                data_supplier_push_char_num(pCtx->pDrain, pCtx->processingCtx.data.meas.ucCommandResult);
// TODO: lONG
                data_supplier_push(pCtx->pDrain, PUSHER_SHORT, &(pCtx->processingCtx.data.meas.ulGot), 2);
                pCtx->processingCtx.ucState++;
        case 8: BREAK_ON_NA(pCtx);
                data_supplier_push_static_string(pCtx->pDrain, pcRN);
                pCtx->ucProcessing = 0; rc = 1;

    }
    return rc;
}

