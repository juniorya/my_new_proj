//
//
//

#include "data_supplier.h"
#include <string.h>

const unsigned char ucMaxStaticChunkSize = 64;

int data_supplier_init(data_supplier *ctx)
{
    ctx->fStream = 0;
    ctx->hStream = 0;
    ctx->ucChunkSize = 0;
    ctx->ucCurrentPusher = 0;
    ctx->ucAllocated = 0;
    ctx->ucSupplierState = 0;
    ctx->usCurrentPos = 0;
    ctx->ucCurrentState = 0;
    return 1;
}

int data_supplier_push(data_supplier *ctx, unsigned char ucType, const void *pData, unsigned short usSize)
{
    unsigned char ucPos;
    if (ctx->ucAllocated >= PUSHERS_AMOUNT) return 0;

    ucPos = ctx->ucCurrentPusher + ctx->ucAllocated; if (ucPos >= PUSHERS_AMOUNT) ucPos -= PUSHERS_AMOUNT;
    ctx->ucAllocated++;

    ctx->pushers[ucPos].ucType = ucType;
    switch(ucType) {
        case PUSHER_CHAR:
        case PUSHER_CHAR_LZ_HEX:
        case PUSHER_CHAR_LZ0X_HEX:
            ctx->pushers[ucPos].data.ucV = *(const unsigned char*)pData;
            ctx->pushers[ucPos].usSize = sizeof(unsigned char);
            break;
        case PUSHER_STR:
            ctx->pushers[ucPos].data.pbV = (const unsigned char*)pData;
            ctx->pushers[ucPos].usSize = (usSize)? usSize : strlen((char *)pData);
            break;
        case PUSHER_HEXBYTES:
        case PUSHER_HEXBYTES_SP:
        case PUSHER_HEXBYTES_WITH_IOCTRL:
        case PUSHER_HEXBYTES_SP_WITH_IOCTRL:
            ctx->pushers[ucPos].data.pbV = (const unsigned char*)pData;
            ctx->pushers[ucPos].usSize = usSize;
            break;
/*define PUSHER_HEXWORDS_LZ    6 */
        case PUSHER_LONG:
        case PUSHER_LONG_LZ_HEX:
            ctx->pushers[ucPos].data.ulV = *(const unsigned long*)pData;
            ctx->pushers[ucPos].usSize = sizeof(unsigned long);
            break;
        case PUSHER_STR_WITH_IOCTRL:
            ctx->pushers[ucPos].data.pbV = (const unsigned char*)pData;
            ctx->pushers[ucPos].usSize = (usSize)? usSize : strlen(((char *)pData) + 1);
            break;
        case PUSHER_SHORT:
        case PUSHER_SHORT_LZ_HEX:
        case PUSHER_SHORT_LZ0X_HEX:
            ctx->pushers[ucPos].data.sV = *(const short*)pData;
            ctx->pushers[ucPos].usSize = sizeof(short);
            break;
        case PUSHER_HEXSHORTS:
        case PUSHER_HEXSHORTS_LZ0X:
            ctx->pushers[ucPos].data.pbV = (const unsigned char*)pData;
            ctx->pushers[ucPos].usSize = usSize;
            break;

/*define PUSHER_HEXWORDS_LZ_WITH_IOCTRL    0x86 */

        default:
            return -1;
    }
    return 1;
}

int data_supplier_attach_current_stream(data_supplier *ctx, push_data_func fStream, void *hStream)
{
    // check fsm state
    if (ctx->ucSupplierState == 0) {
        ctx->fStream = fStream;
        ctx->hStream = hStream;
        return 1;
    }
    return 0;
}


int data_supplier_detach_current_stream(data_supplier *ctx)
{
    // check state
    if (ctx->ucSupplierState == 0) {
        ctx->fStream = 0;
        ctx->hStream = 0;
        return 1;
    }

    ctx->ucSupplierState |= 0x80;

//!    tick, check tick

    // set fsm state we want to exit
    return 0;
}


// some forwards for _tick
int ds_int_sym_to_chunk(data_supplier *ctx, data_pusher *pP);
int ds_int_add_to_chunk(data_supplier *ctx);
int ds_int_prepare_chunk(data_supplier *ctx);

unsigned char t = 48;

int data_supplier_tick(data_supplier *ctx)
{
    int rc;
    unsigned char s;
    const unsigned char *pB;
    data_pusher *pP;
    if (!(ctx->fStream)) return -1;

    if ((ctx->ucAllocated == 0) && (ctx->ucChunkSize == 0)) return 0;

    while(1) {
        s = (ctx->ucSupplierState & 3);
        rc = 0;

        if (!s) {
            ds_int_prepare_chunk(ctx);
        } else
        if (s == 1) {
            ds_int_add_to_chunk(ctx);
        } else {
            // means we already have static chunk, nothing to do with it
        }

        s = (ctx->ucSupplierState & 3); // again, as it've been changed in prepare/add

        if ((s != 0) && (ctx->fStream)) {
            pP = &(ctx->pushers[ctx->ucCurrentPusher]);
            // for static strings use their direct address without copying to chunk
            pB = (s == 1)? ctx->pbChunk : pP->data.pbV + ctx->usCurrentPos;

            rc = ctx->fStream(ctx->hStream, pB, ctx->ucChunkSize);
            if (rc < 0) {
                // error 
            } else
            if (rc == 0) {
                // return 0
            } else {
                // chunk is sent.
                if (s == 2) {
                    // static chunk - set chunk sent
                    ctx->usCurrentPos += ctx->ucChunkSize;
                    if (ctx->usCurrentPos >= pP->usSize) {
                        ctx->ucCurrentPusher++; if (ctx->ucCurrentPusher >= PUSHERS_AMOUNT) ctx->ucCurrentPusher -= PUSHERS_AMOUNT;
                        if (ctx->ucAllocated) ctx->ucAllocated--;
                        ctx->usCurrentPos = 0; ctx->ucCurrentState = 0;
                    }
                }
                ctx->ucSupplierState &= 0xFC; ctx->ucChunkSize = 0;
            }
        }
        if (rc <= 0) break;
    }
    return rc;
}

// valid only for empty or buffered chunk
int ds_int_add_to_chunk(data_supplier *ctx)
{
    data_pusher *pP;
    unsigned char s = (ctx->ucSupplierState & 3);
    if (ctx->ucAllocated == 0) return 0;

    pP = &(ctx->pushers[ctx->ucCurrentPusher]);
    while(ctx->ucChunkSize < sizeof(ctx->pbChunk)) {
        if (ds_int_sym_to_chunk(ctx, pP) == 1) {
            ctx->ucCurrentPusher++;
            if (ctx->ucCurrentPusher >= PUSHERS_AMOUNT) ctx->ucCurrentPusher -= PUSHERS_AMOUNT;
            ctx->usCurrentPos = 0; ctx->ucCurrentState = 0;
            if ((--(ctx->ucAllocated)) == 0) break;
            pP = &(ctx->pushers[ctx->ucCurrentPusher]);
        }
    }
    if (ctx->ucChunkSize > 0) {
        s = (ctx->ucSupplierState & 0xFC);
        ctx->ucSupplierState = (s | 1);
        return 1;
    }
    return 0;
}

int ds_int_prepare_chunk(data_supplier *ctx)
{
    int rc = 1;
    unsigned short x;
    data_pusher *pP;
    ctx->ucSupplierState &= 0xFC;
    ctx->ucChunkSize = 0;

    if (ctx->ucAllocated == 0) return 1;

    pP = &(ctx->pushers[ctx->ucCurrentPusher]);
    if ((pP->ucType == PUSHER_STR) || (pP->ucType == PUSHER_STR_WITH_IOCTRL)) {
        x = pP->usSize - ctx->usCurrentPos;
        // use static, if long enough
        if (x >= SUPPLIER_CHUNK_SIZE) {
            ctx->ucSupplierState |= 2;
            ctx->ucChunkSize = (x > ucMaxStaticChunkSize)? ucMaxStaticChunkSize : x;
        } else {
// Maybe just remove as this is implemented in add_to_chunk...
            ctx->ucSupplierState |= 1;
            ctx->ucChunkSize = (x > sizeof(ctx->pbChunk))? sizeof(ctx->pbChunk) : x;
            memcpy(ctx->pbChunk, ((pP->data.pbV) + ctx->usCurrentPos +
                ((pP->ucType == PUSHER_STR_WITH_IOCTRL)? 1 : 0)), ctx->ucChunkSize);
            // TODO: put flag to IOCTL if finished
            if (ctx->ucChunkSize == x) {
                // completed. we can remove this pusher from the queue
                ctx->ucCurrentPusher++; if (ctx->ucCurrentPusher >= PUSHERS_AMOUNT) ctx->ucCurrentPusher -= PUSHERS_AMOUNT;
                ctx->ucAllocated--;
                ctx->usCurrentPos = 0; ctx->ucCurrentState = 0;
            } else {
                ctx->usCurrentPos += ctx->ucChunkSize;
            }
        }
    }

    if (((ctx->ucSupplierState & 3) != 2) && (ctx->ucChunkSize < sizeof(ctx->pbChunk)))
        rc = ds_int_add_to_chunk(ctx);
    return rc;
}

// following returns 1 if the pusher is pushed completely, 0 otherwise
//
int ds_int_sym_to_chunk(data_supplier *ctx, data_pusher *pP)
{
    int rc = 0;
    unsigned char c, d, n, s = ctx->ucCurrentState, *pDst = ctx->pbChunk + ctx->ucChunkSize;
    short w;
    unsigned short usD = (pP->ucType & 0x80)? 1 : 0; // delta to account IOCtrl byte
// TODO: special case when pP->pData == 0

    switch(pP->ucType) {
        case PUSHER_CHAR:
            if (ctx->usCurrentPos > 0) return -1;
            c = pP->data.ucV;
            if ((s == 0) && (c & 0x80)) {
                *pDst = '-'; s = 1; break;
            } else if (c & 0x80) c = (0 - c);

            if (s < 3) {
                if (c >= 100) {
                    if (s < 2) {
                        *pDst = '1'; s = 2; break;
                    } else {
                        c = c - 100;
                        *pDst = (c / 10) + 48; s = 3; break;
                    }
                } else
                if (c >= 10) {
                    *pDst = (c / 10) + 48; s = 3; break;
                }
            }

            if (c >= 10) c = (c % 10);
            *pDst = c + 48; s = 4; ctx->usCurrentPos++; rc = 1;
            break;

/*define PUSHER_CHAR_LZ_HEX    1
define PUSHER_CHAR_LZ0X_HEX  2
*/
        case PUSHER_STR:
        case PUSHER_STR_WITH_IOCTRL:
            if (ctx->usCurrentPos >= pP->usSize) return -1;
            c = *(pP->data.pbV + ctx->usCurrentPos + usD);
            *pDst = c; ctx->usCurrentPos++;
            if (ctx->usCurrentPos == pP->usSize) rc = 1;
            break;

        case PUSHER_HEXBYTES:
        case PUSHER_HEXBYTES_SP:
        case PUSHER_HEXBYTES_WITH_IOCTRL:
        case PUSHER_HEXBYTES_SP_WITH_IOCTRL:
            if ((ctx->usCurrentPos + usD) >= pP->usSize) return -1;
            c = *(pP->data.pbV + ctx->usCurrentPos + usD);
            switch(s) {
                case 0: if ((ctx->usCurrentPos > 0) && ((pP->ucType == PUSHER_HEXBYTES_SP) || (pP->ucType == PUSHER_HEXBYTES_SP_WITH_IOCTRL))) {
                            *pDst = ' '; s = 1; break;
                        }
                case 1: *pDst = ((c >> 4) & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 2;
                        break;
                case 2: *pDst = (c & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 0;
                        ctx->usCurrentPos++;
                        if (ctx->usCurrentPos == pP->usSize) rc = 1;
                        break;
            }
            break;

        case PUSHER_SHORT:
            if (ctx->usCurrentPos > 0) return -1;
            w = pP->data.sV; n = 0;
            if (w < 0) {
                n = 1; // means the value is negative
                if (s == 0) {
                    *pDst = '-'; s = 1; break;
                } else w = -w;
            }

            // max number of a digit in the value (from zero)
            d = (w < 1000)? ((w < 10)? 0 : ((w < 100)? 1 : 2)) : ((w < 10000)? 3 : 4);

            if (s - n > d) {
                return -1; // out of digits in the value
            } else if ((s - n) == d) {
                rc = 1; // this is the last digit
            }

            d = d - s + n; // the digit we need
            *pDst = (((d > 1)? ((d > 3)? (w / 10000) : ((d > 2)? (w / 1000) : (w / 100))) : ((d > 0)? (w / 10) : w)) % 10) + 48;
            s++;
            break;
        case PUSHER_SHORT_LZ_HEX:
        case PUSHER_SHORT_LZ0X_HEX:
            if (ctx->usCurrentPos > 0) return -1;
            w = pP->data.sV; n = 0;
            switch(s) {
                case 0:
                case 1:
                        if (pP->ucType == PUSHER_SHORT_LZ_HEX) {
                            s = 2;
                        } else {
                            *pDst = (s == 0)? '0' : 'x'; s++; break;
                        }
                case 2: *pDst = ((w >> 12) & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 3;
                        break;
                case 3: *pDst = ((w >> 8) & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 4;
                        break;
                case 4: *pDst = ((w >> 4) & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 5;
                        break;
                case 5: *pDst = (w & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 0;
                        ctx->usCurrentPos++; rc = 1;
                        break;
            }
            break;

        case PUSHER_HEXSHORTS:
        case PUSHER_HEXSHORTS_LZ0X:
            if ((ctx->usCurrentPos + usD) >= pP->usSize) return -1;
            c = *(pP->data.pbV + ctx->usCurrentPos + usD);
            switch(s) {
                case 0:
                        s = 1;
                        if ((ctx->usCurrentPos > 0) && ((ctx->usCurrentPos & 1) == 0)
//                             && ((pP->ucType == PUSHER_HEXBYTES_SP) || (pP->ucType == PUSHER_HEXBYTES_SP_WITH_IOCTRL)))
                        ) {
                            *pDst = ' '; break;
                        }
                case 1:
                case 2:
                        if ((pP->ucType == PUSHER_HEXSHORTS_LZ0X) && ((ctx->usCurrentPos & 1) == 0)) {
                            *pDst = (s == 1)? '0' : 'x'; s++; break;
                        } else {
                            s = 3;
                        }
                case 3: *pDst = ((c >> 4) & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 4;
                        break;
                case 4: *pDst = (c & 0xF); (*pDst) += (((*pDst) > 9)? 87 : 48);
                        s = 0;
                        ctx->usCurrentPos++;
                        if (ctx->usCurrentPos == pP->usSize) rc = 1;
                        break;
            }
            break;

/*
define PUSHER_HEXWORDS_LZ    5
define PUSHER_LONG           6
define PUSHER_LONG_LZ_HEX    7
define PUSHER_HEXWORDS_LZ_WITH_IOCTRL    0x85
*/

    }
    ctx->ucCurrentState = s;
    ctx->ucChunkSize++;
    return rc;
}

