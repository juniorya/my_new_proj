//
//
//

// to config:

//#define SERIAL_DRV_XM24M        1
//#define SERIAL_INSERT_DRIVER_1  xr24m

#include "serial.h"
#include "../arch/arch.h"

#define FINT(prefix, func) prefix##func
#define FUNC(prefix, func) FINT(prefix, func)
//#define FUNC(prefix, func) #func

#ifdef SERIAL_INSERT_DRIVER_1
#ifndef SERIAL_INSERT_DRIVER_1_DRV
#define SERIAL_INSERT_DRIVER_1_DRV 1
#endif
int FUNC(SERIAL_INSERT_DRIVER_1, _serial_configure)(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int FUNC(SERIAL_INSERT_DRIVER_1, _serial_available)(void *pInstance, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_1, _serial_read)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_1, _serial_write)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_1, _serial_ready)(void *pInstance, unsigned char ucOperation);
#endif

#ifdef SERIAL_INSERT_DRIVER_2
#ifndef SERIAL_INSERT_DRIVER_2_DRV
#define SERIAL_INSERT_DRIVER_2_DRV 2
#endif
int FUNC(SERIAL_INSERT_DRIVER_2, _serial_configure)(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int FUNC(SERIAL_INSERT_DRIVER_2, _serial_available)(void *pInstance, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_2, _serial_read)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_2, _serial_write)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_2, _serial_ready)(void *pInstance, unsigned char ucOperation);
#endif

#ifdef SERIAL_INSERT_DRIVER_3
#ifndef SERIAL_INSERT_DRIVER_3_DRV
#define SERIAL_INSERT_DRIVER_3_DRV 3
#endif
int FUNC(SERIAL_INSERT_DRIVER_3, _serial_configure)(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int FUNC(SERIAL_INSERT_DRIVER_3, _serial_available)(void *pInstance, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_3, _serial_read)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_3, _serial_write)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_3, _serial_ready)(void *pInstance, unsigned char ucOperation);
#endif

#ifdef SERIAL_INSERT_DRIVER_4
#ifndef SERIAL_INSERT_DRIVER_4_DRV
#define SERIAL_INSERT_DRIVER_4_DRV 4
#endif
int FUNC(SERIAL_INSERT_DRIVER_4, _serial_configure)(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int FUNC(SERIAL_INSERT_DRIVER_4, _serial_available)(void *pInstance, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_4, _serial_read)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_4, _serial_write)(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int FUNC(SERIAL_INSERT_DRIVER_4, _serial_ready)(void *pInstance, unsigned char ucOperation);
#endif

int serial_set_rx_callback(void *pInstance, serial_rx_callback fRXC, void *pHandle)
{
    if (pInstance) {
        ((serial_instance *)pInstance)->fCustomRXCb = fRXC;
        ((serial_instance *)pInstance)->pHandle = pHandle;
        return 1;
    }
    return 0;
}

int serial_configure(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops)
{
#ifdef __ARCH_WITH_SERIAL
    if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_DRV_SYSTEM) {
        return __system_serial_configure(pInstance, ulBaudrate, ucBits, ucParity, ucStops);
    }
#else
    if (0) {
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_1
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_1_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_1, _serial_configure)(pInstance, ulBaudrate, ucBits, ucParity, ucStops);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_2
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_2_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_2, _serial_configure)(pInstance, ulBaudrate, ucBits, ucParity, ucStops);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_3
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_3_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_3, _serial_configure)(pInstance, ulBaudrate, ucBits, ucParity, ucStops);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_4
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_4_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_4, _serial_configure)(pInstance, ulBaudrate, ucBits, ucParity, ucStops);
    }
#endif
    return -1;
}

int serial_available(void *pInstance, unsigned *puLen)
{
#ifdef __ARCH_WITH_SERIAL
    if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_DRV_SYSTEM) {
        return __system_serial_available(pInstance, puLen);
    }
#else
    if (0) {
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_1
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_1_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_1, _serial_available)(pInstance, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_2
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_2_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_2, _serial_available)(pInstance, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_3
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_3_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_3, _serial_available)(pInstance, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_4
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_4_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_4, _serial_available)(pInstance, puLen);
    }
#endif
    return -1;
}

int serial_read(void *pInstance, unsigned char *pbBuffer, unsigned *puLen)
{
#ifdef __ARCH_WITH_SERIAL
    if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_DRV_SYSTEM) {
        return __system_serial_read(pInstance, pbBuffer, puLen);
    }
#else
    if (0) {
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_1
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_1_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_1, _serial_read)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_2
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_2_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_2, _serial_read)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_3
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_3_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_3, _serial_read)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_4
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_4_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_4, _serial_read)(pInstance, pbBuffer, puLen);
    }
#endif
    return -1;

}

int serial_write(void *pInstance, unsigned char *pbBuffer, unsigned *puLen)
{
#ifdef __ARCH_WITH_SERIAL
    if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_DRV_SYSTEM) {
        return __system_serial_write(pInstance, pbBuffer, puLen);
    }
#else
    if (0) {
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_1
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_1_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_1, _serial_write)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_2
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_2_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_2, _serial_write)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_3
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_3_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_3, _serial_write)(pInstance, pbBuffer, puLen);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_4
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_4_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_4, _serial_write)(pInstance, pbBuffer, puLen);
    }
#endif
    return -1;
}


int serial_ready(void *pInstance, unsigned char ucOperation)
{
#ifdef __ARCH_WITH_SERIAL
    if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_DRV_SYSTEM) {
        return __system_serial_ready(pInstance, ucOperation);
    }
#else
    if (0) {
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_1
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_1_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_1, _serial_ready)(pInstance, ucOperation);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_2
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_2_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_2, _serial_ready)(pInstance, ucOperation);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_3
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_3_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_3, _serial_ready)(pInstance, ucOperation);
    }
#endif
#ifdef SERIAL_INSERT_DRIVER_4
    else if (((serial_instance *)pInstance)->ucDrvSelector == SERIAL_INSERT_DRIVER_4_DRV) {
        return FUNC(SERIAL_INSERT_DRIVER_4, _serial_ready)(pInstance, ucOperation);
    }
#endif
    return -1;
}

