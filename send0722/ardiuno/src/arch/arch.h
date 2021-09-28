/////////////////////
//
//  ...
//
/////////////////////


#ifndef __ARCH_H__
#define __ARCH_H__

void __print_console(const char *pcStr);
int __rs485_transmit(unsigned char *pbBuffer, unsigned uSize);

#include <config.h>

#ifdef __ARCH_WITH_SPI

// SPI
typedef struct {
} spi_instance;

// maybe add interrupt number?

// TODO: static inline and move to header
void *spi_get(unsigned char uSPINum);
//unsigned char spi_acquire(void *pSpiInstance, unsigned long ulClock, unsigned char ulBitOrder, unsigned char ulDataMode);
#define SPI_CPOL_0 0
#define SPI_CPOL_1 1
#define SPI_CPHA_0 0
#define SPI_CPHA_1 2
void spi_begin_transaction(void *pSpiInstance, unsigned char ucMode);
void spi_end_transaction(void *pSpiInstance);
unsigned char spi_transfer(void *pSpiInstance, unsigned char *pbSendBuffer, unsigned uSSize, unsigned char *pbReadBuffer, unsigned uRSize);
unsigned char spi_io_ready(void *pSpiInstance);
#endif

void *i2c_get(unsigned char ucNum);
unsigned char i2c_master_send(void *pInstance, unsigned short usAddr, unsigned char *pbBuffer, unsigned short usLen, unsigned char usFinal);
unsigned char i2c_master_receive(void *pInstance, unsigned short usAddr, unsigned char *pbBuffer, unsigned short usLen, unsigned char usFinal);
unsigned char i2c_io_ready(void *pInstance);

#ifdef __ARCH_WITH_SERIAL

typedef int (*serial_rx_callback)(void *pInstance, unsigned char *pbBuffer, unsigned uSize);

void *__system_serial_get(unsigned char ucNum);
int __system_serial_configure(void *pInstance, unsigned long ulBaudrate, unsigned char ucBits, unsigned char ucParity, unsigned char ucStops);
int __system_serial_available(void *pInstance, unsigned *puLen);
int __system_serial_read(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int __system_serial_write(void *pInstance, unsigned char *pbBuffer, unsigned *puLen);
int __system_serial_ready(void *pInstance, unsigned char ucOperation);

#endif


#ifdef __ARCH_INCLUDE__
#include __ARCH_INCLUDE__
#endif

// COMMON
#ifndef __MICROS_DEFINED
unsigned long __micros(void);
#endif

//void __system_micros_sleep(unsigned long micros);


#endif /*  __ARCH_H__ */
    