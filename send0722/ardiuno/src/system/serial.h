//
// Serial port (UART)
//

#ifndef __SERIAL_H_20200510__
#define __SERIAL_H_20200510__

typedef int (*serial_rx_callback)(void *pHandle, unsigned char *pbBuffer, unsigned uSize);

typedef struct serial_instance__ {
    unsigned char ucDrvSelector;
    serial_rx_callback fCustomRXCb;
    void *pHandle;                   // handle to use in custom RX callback
    void *pDrvExtra;                 // driver-specific data
} serial_instance;

#define SERIAL_DRV_SYSTEM     0


#define SERIAL_OPERATION_RX   1
#define SERIAL_OPERATION_TX   2
#define SERIAL_OPERATION_CONF 4
#define SERIAL_OPERATION_AVAIL SERIAL_OPERATION_CONF


int serial_set_rx_callback(void *pInstance, serial_rx_callback fRXC, void *pHandle);
int serial_configure(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int serial_available(void *pInstance, unsigned *puLen);
int serial_read(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int serial_write(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int serial_ready(void *pInstance, unsigned char ucOperation);


#endif /*  __SERIAL_H_20200510__ */
