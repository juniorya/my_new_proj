
#ifndef __HIVES_ARCH_ARDUINO__
#define __HIVES_ARCH_ARDUINO__

#undef __micros

#include <Arduino.h>

// define arduino micros FOR NON-TIMER2
#ifndef CONFIG_ARDUINO_SLEEP_TIMER2

#define __MICROS_DEFINED
#define __micros() micros()

#else
#if CONFIG_ARDUINO_SLEEP_TIMER2 == 0

#define __MICROS_DEFINED
#define __micros() micros()

#endif
#endif


//static inline unsigned long __micros() {
//    return micros();
//}

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    unsigned char spcr;
    unsigned char spsr;
} arduino_spi_instance;

#ifdef __cplusplus
}
#endif

#define spi_instance arduino_spi_instance

typedef unsigned char irq_save_t;

#define __irqdisable(tosave) \
tosave=SREG; cli()

#define __irqrestore(saved) \
SREG=saved

#endif