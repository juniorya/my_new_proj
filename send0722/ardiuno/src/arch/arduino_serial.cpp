
//#include "arch.h"
#include <config.h>
#include <Arduino.h>

extern "C"
{
#include "../system/serial.h"
}

#ifdef __ARCH_WITH_SERIAL

// serial port communications

#define ARDUINO_SERIALS  1

// all this stuff is just to try to write non-blocking
typedef struct arduino_serial_ctx_t__ {
    unsigned char ucFlags;
    unsigned char ucToWriteHi;
    unsigned char ucToWriteLow;
    unsigned char *pbTX;
    Stream *pSerial;
} arduino_serial_ctx;

// Never set advanced TX on software serial!!!
// It means using availableForWrite which is not implemented in Software serial
#define ARDUINO_SERIAL_ADVANCED_TX 1
#define ARDUINO_SERIAL_BUSY_TX     2

#if ARDUINO_SERIALS < 2
static arduino_serial_ctx arduinoSerial = {
    ARDUINO_SERIAL_ADVANCED_TX, 0, 0, 0, &Serial
};
static serial_instance arduinoSerialInstance = {
    SERIAL_DRV_SYSTEM, 0, 0, &arduinoSerial
};
#else
static arduino_serial_ctx arduinoSerials[ARDUINO_SERIALS];
static serial_instance arduinoSerialInstances[ARDUINO_SERIALS];
#endif


#ifdef CONFIG_ARDUINO_DEBUG

#include <SoftwareSerial.h>

//SoftwareSerial arduino_debug(A6, A5); // RX TX
//SoftwareSerial arduino_debug(3, 2); // RX TX

#define arduino_debug Serial
#endif

extern "C" void arduino_debug_str(const char *pStr)
{
#ifdef CONFIG_ARDUINO_DEBUG
    arduino_debug.println(pStr);
#endif
}

extern "C" void arduino_debug_sym(const char c)
{
#ifdef CONFIG_ARDUINO_DEBUG
    arduino_debug.write(c);
#endif
}

extern "C" void arduino_debug_l(long l)
{
#ifdef CONFIG_ARDUINO_DEBUG
    arduino_debug.print(l);
#endif
}

extern "C" void *__system_serial_get(unsigned char ucNum)
{
#if ARDUINO_SERIALS < 2
    return (&arduinoSerialInstance);
#else
    return ((ucNum >= 0) && (ucNum < ARDUINO_SERIALS))? arduinoSerialInstances[ucNum] : 0;
#endif
}

extern "C" int __system_serial_init(unsigned char ucConfigurationNumber)
{
    pinMode(0, INPUT);
    Serial.begin(SERIAL_0_BAUDRATE);

//Serial.println("Krugly");

#ifdef CONFIG_ARDUINO_DEBUG
pinMode(2, OUTPUT);
//!arduino_debug.begin(19200);
#endif

#if ARDUINO_SERIALS >= 2
//    if (pInstance) {
//        ((serial_instance *)pInstance)->ucDrvSelector = SERIAL_DRV_SYSTEM;
//        ((serial_instance *)pInstance)->pDrvExtra = &(arduinoSerial);
//    }
//#else
//    ((serial_instance *)pInstance)->ucDrvSelector = SERIAL_DRV_SYSTEM;
//    ((serial_instance *)pInstance)->pDrvExtra = &(arduinoSerials[ucConfigurationNumber]);

    if ((ucConfigurationNumber >= 0) && (ucConfigurationNumber < ARDUINO_SERIALS)) {
        serial_instance *pI = &(arduinoSerialInstances[ucConfigurationNumber]);
        arduino_serial_ctx *pE = &(arduinoSerials[ucConfigurationNumber]);

        pI->ucDrvSelector = SERIAL_DRV_SYSTEM;
        pI->fCustomRXCb = 0;
        pI->pDrvExtra = pE;

        pE->ucFlags = ARDUINO_SERIAL_ADVANCED_TX; // TODO: configurable
        pE->ucToWriteHi = 0;
        pE->ucToWriteLow = 0;
        pE->pbTX = 0;
        pE->pSerial = &Serial; // TODO: serial 0, 1, 2 etc...
#endif

    return 1;
}

extern "C" int __system_serial_available(void *pInstance, unsigned *puLen)
{
#if ARDUINO_SERIALS < 2
    *puLen = arduinoSerial.pSerial->available();
//arduino_debug_str((*puLen)? "A\r\n" : "N\r\n");
    return 1;
#else
    *puLen = ((arduino_serial_ctx *)(((serial_instance *)pInstance)->pDrvExtra))->pSerial->available();
    return 1;
#endif
}

extern "C" int __system_serial_read(void *pInstance, unsigned char *pbBuffer, unsigned *puLen)
{
#if ARDUINO_SERIALS < 2
#define ctx(p) (&arduinoSerial)
#else
#define ctx(p) ((arduino_serial_ctx *)(((serial_instance *)p)->pDrvExtra))
#endif

    unsigned usMax = ctx(pInstance)->pSerial->available(); if ((*puLen) < usMax) usMax = (*puLen);
    unsigned char ucMax = (usMax > 255)? 255 : usMax, x;
    (*puLen) = ucMax;
    for(x = 0 ; x < ucMax; x++) {
        pbBuffer[x] = ctx(pInstance)->pSerial->read();
    }
    return 1;
}

// 1 - all ready
// 0 - just sent
// -1 - error

static int send_one(arduino_serial_ctx *pC)
{
    if ((pC->ucToWriteLow == 0) && (pC->ucToWriteHi == 0)) return 1;

    if (!(pC->pSerial->write(*(pC->pbTX)))) return -1;
    pC->pbTX++;
    if (pC->ucToWriteLow == 0) {
        pC->ucToWriteLow = 255;
        pC->ucToWriteHi--;
    } else {
        pC->ucToWriteLow--;
    }

    return ((pC->ucToWriteLow == 0) && (pC->ucToWriteHi == 0))? 1 : 0;
}

extern "C" int __system_serial_write(void *pInstance, unsigned char *pbBuffer, unsigned *puLen)
{
#if ARDUINO_SERIALS < 2
#define ctx(p) (&arduinoSerial)
#else
#define ctx(p) ((arduino_serial_ctx *)(((serial_instance *)p)->pDrvExtra))
#endif

    register unsigned char ucFlags = ctx(pInstance)->ucFlags;

    if (ucFlags & ARDUINO_SERIAL_BUSY_TX) return 0;
    ctx(pInstance)->ucFlags |= ARDUINO_SERIAL_BUSY_TX;

    ctx(pInstance)->pbTX = pbBuffer;
    ctx(pInstance)->ucToWriteHi = ((*puLen) >> 8);
    ctx(pInstance)->ucToWriteLow = ((*puLen) & 0xFF);

    if (ucFlags & ARDUINO_SERIAL_ADVANCED_TX) {
// push to queue as much as we can (have)
        unsigned char xMax = ctx(pInstance)->pSerial->availableForWrite();
        while(xMax > 0) {
            if (send_one(ctx(pInstance)) != 0) break;
            xMax--;
        }
    } else {
    // send one by one
        send_one(ctx(pInstance));
    }

//    int n = Serial.write(pbBuffer, *puLen);
//    Serial.flush();
//    if (n >= 0) *puLen = (unsigned)n;
//    return n;
    return 1;
}

extern "C" int __system_serial_ready(void *pInstance, unsigned char ucOperation)
{
    if (!(ucOperation & SERIAL_OPERATION_TX)) return 1;

#if ARDUINO_SERIALS < 2
#define ctx(p) (&arduinoSerial)
#else
#define ctx(p) ((arduino_serial_ctx *)(((serial_instance *)p)->pDrvExtra))
#endif
    register unsigned char ucFlags = ctx(pInstance)->ucFlags, x;
    if (!(ucFlags & ARDUINO_SERIAL_BUSY_TX)) return 1;

    if (ucFlags & ARDUINO_SERIAL_ADVANCED_TX) {
// push to queue as much as we can (have)
        unsigned char xMax = ctx(pInstance)->pSerial->availableForWrite();
        while(xMax > 0) {
            if ((x = send_one(ctx(pInstance))) != 0) break;
            xMax--;
        }
    } else {
    // send one by one
        x = send_one(ctx(pInstance));
    }
    if (x == 1) {
        ctx(pInstance)->ucFlags &= (~ARDUINO_SERIAL_BUSY_TX);
    }
    return 0;
}

#endif /* __ARCH_WITH_SERIAL */
