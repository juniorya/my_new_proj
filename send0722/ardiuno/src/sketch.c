
#include "../config.h"
#include "net_reg.h"
#include "reg_alg.h"
#include "arch/arch.h"
#include "app/app.h"

//#include <avr/sleep.h>
//#include <avr/power.h>


// Arduino-specific serial port initialization
int __system_serial_init(unsigned char ucConfigurationNumber);

void arduino_debug_str(const char *pStr);

void dd(const char *pS)
{
    arduino_debug_str(pS);
}

// Arduino-specific write to eeprom
int write_id_to_eeprom(unsigned long ulID);

/*
volatile unsigned char ucCounterOn = 0;
volatile unsigned short usCounter = 0;

void counterISR() {
    if (ucCounterOn) usCounter++;
}

void counterCtrl(unsigned char ucOn)
{
// ? maybe disable interrupts
    if (ucOn) {
        usCounter = 0;
        ucCounterOn = 1;
    } else {
        ucCounterOn = 0;
    }
}

unsigned short counterCount(void)
{
    return usCounter;
}
*/

void setup() {
    void *pSerial = 0;
    char *pcStr = "Started\r\n";
    unsigned uLen = 9; // strlen Started

//    unsigned char x;

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(5, OUTPUT);
//    pinMode(8, OUTPUT);
//    pinMode(13, OUTPUT);
//    digitalWrite(13, HIGH);
  
//    pinMode(LED, OUTPUT);
//    pinMode(LED2, OUTPUT);
//    pinMode(A4, OUTPUT);

//    digitalRead(A4);
    pinMode(A5, OUTPUT);

//    pinMode(A4, OUTPUT);

//    pinMode(A0, OUTPUT);
//    pinMode(2, OUTPUT);
//    pinMode(1, OUTPUT);
//    pinMode(A3, OUTPUT);
    
// TO RADIO arduino-specific (?)
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);

// for TIMER2_SLEEP
//M    ADCSRA = 0;

    // turn off everything we can
//  power_adc_disable ();
//  power_spi_disable();
//  power_twi_disable();
//  power_timer0_disable();
//  power_timer1_disable();
//  power_usart0_disable();

    // full power-down doesn't respond to Timer 2
/*M    set_sleep_mode (SLEEP_MODE_PWR_SAVE);
    sleep_enable();
    
    TIMSK2 = 0;
    ASSR = bit (AS2);
    TCNT2 = 0;
    OCR2A = 1;
    TCCR2A = bit (WGM21);                             // CTC
    TCCR2B = bit (CS20);
//    TCCR2B = 0;

    while (ASSR & (_BV(TCN2UB) | _BV(TCR2BUB)));
    TIMSK2 |= bit (OCIE2A);
    while (ASSR & (_BV(TCN2UB) | _BV(TCR2BUB)));
M*/

// EoF TIMER2_SLEEP

/*
digitalWrite(8, 1);
//digitalWrite(A5, 1);
//digitalWrite(A4, 1);
__system_micros_sleep(2000000);
//digitalWrite(A4, 0);
//digitalWrite(A5, 0);
digitalWrite(8, 0);
*/

//    write_id_to_eeprom(1);


    regAlgInitialize();

    if (!netInit()) {
        arduino_debug_str("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true) {
          delay(1); // do nothing, no point running without Ethernet hardware
        }
    }
/*
    pinMode(3, INPUT);
    attachInterrupt(1, counterISR, RISING);
*/

    __system_serial_init(APP_CONSOLE_SERIAL);

    app_initialize();

    pSerial = __system_serial_get(APP_CONSOLE_SERIAL);
    if (__system_serial_write(pSerial, (unsigned char*)pcStr, &uLen) == 1) {
        while(!__system_serial_ready(pSerial, 2 /*SERIAL_OPERATION_TX*/)) {};
    }


/*    __print_console("Started\r\n");
    __modem_configure(pSerial);
    __print_console("s1_1\r\n");
    __radio_configure();
    __print_console("s2\r\n");

// TODO: stock to define or smth
    __transport_configure((STOCK)? 1 : 0, pbTableBuf, sizeof(pbTableBuf));

    for(x = 0; x < APP_BUFFERS_COUNT; x++) pPool[x].ucControl = 0;

    gucHbtState = 1;
    gucHbtItem = 0xFF;
    gucStockItem = 0;
    gulHbtTO = __micros() + 5000000;


//    core_start_proto_thread(sender, &pt1);

    core_start_proto_thread(__transport_proc, &pt_transport);
    core_start_proto_thread(__modem_transport_proc, &pt_modem_transport);
    core_start_proto_thread(stock_io, &pt_stock_io);
//    core_start_proto_thread(weight_proc, &pt_weight);
    core_start_proto_thread(application, &pt1);

    __print_console("Started\r\n");
*/
}

void loop() {
//    core_schedule();
    app_cycle_tick();
//PORTC |= _BV(PC5);
//    __system_micros_sleep(1000);
//delay(1);
//PORTC &= ~(_BV(PC5));
//    __system_micros_sleep(999000);
//delay(999);
}