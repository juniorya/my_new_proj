//
// Data supplier - item to provide anynchronous data pushing to console(s)
//

#ifndef __DATASUPPLIER_H_20200610__
#define __DATASUPPLIER_H_20200610__

#define SUPPLIER_CHUNK_SIZE 8
#define PUSHERS_AMOUNT 5

#define PUSHER_CHAR           0
#define PUSHER_CHAR_LZ_HEX    1
#define PUSHER_CHAR_LZ0X_HEX  2
#define PUSHER_STR            3
#define PUSHER_HEXBYTES       4
#define PUSHER_HEXBYTES_SP    5
#define PUSHER_HEXWORDS_LZ    6
#define PUSHER_LONG           7
#define PUSHER_LONG_LZ_HEX    8
#define PUSHER_SHORT          9
#define PUSHER_SHORT_LZ_HEX   10
#define PUSHER_SHORT_LZ0X_HEX 11
#define PUSHER_HEXSHORTS      12
#define PUSHER_HEXSHORTS_LZ0X 13

// be careful,
// 1. shorts in arrays are MSB-first
// 2. size of array in bytes
// in case of odd array size, last value will be output as byte ( like 0x1234 0x56 )

#define PUSHER_STR_WITH_IOCTRL            0x83
#define PUSHER_HEXBYTES_WITH_IOCTRL       0x84
#define PUSHER_HEXBYTES_SP_WITH_IOCTRL    0x85
#define PUSHER_HEXWORDS_LZ_WITH_IOCTRL    0x86


typedef struct data_pusher_t {
    union {
        const unsigned char *pbV;
        char cV;
        short sV;
        unsigned char ucV;
        unsigned long ulV;
    } data;
//    void *pData;
    unsigned short usSize;
    unsigned char ucType;
} data_pusher;

typedef int (*push_data_func)(void *pHandler, const unsigned char *pbBuffer, unsigned uSize);

typedef struct data_supplier_t {
    push_data_func fStream;
    void *hStream;
    unsigned short usCurrentPos;
    unsigned char pbChunk[SUPPLIER_CHUNK_SIZE];
    unsigned char ucChunkSize;
    unsigned char ucCurrentPusher;
    unsigned char ucAllocated;
    unsigned char ucSupplierState;
    unsigned char ucCurrentState;
 
    data_pusher pushers[PUSHERS_AMOUNT];
} data_supplier;

int data_supplier_init(data_supplier *pCtx);

#define data_supplier_available(pCtx) \
(pCtx->ucAllocated < PUSHERS_AMOUNT)

#define data_supplier_empty(pCtx) \
(pCtx->ucAllocated == 0)

int data_supplier_attach_current_stream(data_supplier *pCtx, push_data_func fStream, void *hStream);

int data_supplier_detach_current_stream(data_supplier *pCtx);

int data_supplier_push(data_supplier *ctx, unsigned char ucType, const void *pData, unsigned short usSize);

// Wrappers to more comfort operation

#define data_supplier_push_static_string(pCtx, pcStr) \
data_supplier_push(pCtx, PUSHER_STR, (void *)pcStr, 0)

#define data_supplier_push_char_num(pCtx, ucVal) \
data_supplier_push(pCtx, PUSHER_CHAR, &(ucVal), 1)

#define data_supplier_push_static_buffer(pCtx, pbBuffer, usLen) \
data_supplier_push(pCtx, PUSHER_HEXBYTES_SP, pbBuffer, usLen)

int data_supplier_tick(data_supplier *pCtx);

#endif /* __DATASUPPLIER_H_20200610__ */

