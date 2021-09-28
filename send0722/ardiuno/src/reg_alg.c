//
//
//

#include "reg_alg.h"
#include "net_reg.h"
#include <Arduino.h>

// Buffer size must let us to have two (at least two) buffers
// (maybe round-buffer later)
#define BUFFER_SIZE 384


volatile unsigned char pbSendBuffer1[BUFFER_SIZE];
volatile unsigned short usBuf1Pos = 0;
volatile unsigned char pbSendBuffer2[BUFFER_SIZE];
volatile unsigned short usBuf2Pos = 0;
volatile unsigned char ucActiveBuffer = 0;
volatile unsigned char ucTimeWritten = 0;

volatile unsigned char ucCounterOn = 0;
volatile unsigned short usCounter = 0;


void counterISR(void);
static inline void init_timer1(void);


void regAlgInitialize(void)
{
    pinMode(5, OUTPUT);
//    init_timer1();

    pinMode(3, INPUT);
    attachInterrupt(1, counterISR, CHANGE);
}

static inline void init_timer1(void)
{
    TIMSK1 = 0;
    TCCR1A = bit (COM1A1);  // clear on compare OC1A match
    TCCR1B = bit (WGM12) | bit (CS11); // CTC mode (OCR1A top); /8 prescaler (2 Mhz)

    OCR1A = 0x7fff; /* really we need 0x3fff if we could tick at 1 Mhz. But we're ticking on 2 Mhz, so need to shift 1 bit left */
    TCNT1 = 0;

    TIMSK1 |= bit (OCIE1A);
}

static inline void stop_timer1(void)
{
    TIMSK1 = 0;
}

void counterCtrl(unsigned char ucOn)
{
// ? maybe disable interrupts
    if (ucOn) {
        usBuf1Pos = 0;
        usBuf2Pos = 0;
        ucActiveBuffer = 0;
        ucTimeWritten = 0;

        init_timer1();
        usCounter = 0;
        ucCounterOn = 1;
        netEvent(RA_NET_CONNECT);
    } else {
        stop_timer1();
        ucCounterOn = 0;
        netEvent(RA_NET_FINISH);
    }
}

unsigned short counterCount(void)
{
    return usCounter;
}

unsigned char regAlgGetBufferToSend(unsigned char **ppBuf, unsigned short *pusLen)
{
    unsigned char ucRes = 0;
    uint8_t oldSREG = SREG;
    cli();
    if (!ucActiveBuffer) {
        // 1 buffer is active
        if (usBuf1Pos) {
            ucActiveBuffer = 1;
            usBuf2Pos = 0;
            ucTimeWritten = 0;
            *pusLen = usBuf1Pos;
            *ppBuf = pbSendBuffer1;
            ucRes = 1;
        }
    } else {
        // 2 buffer is active
        if (usBuf2Pos) {
            ucActiveBuffer = 0;
            usBuf1Pos = 0;
            ucTimeWritten = 0;
            *pusLen = usBuf2Pos;
            *ppBuf = pbSendBuffer2;
            ucRes = 1;
        }
    }
    SREG = oldSREG;
    return ucRes;
}

void counterISR() {
    unsigned char ucState;
    unsigned short usTimer1;
    volatile unsigned char *p;
    volatile unsigned short *pusL;

    if (ucCounterOn) {
        ucState = (PIND & _BV(PD3))? 1 : 0;
        usTimer1 = (TCNT1 >> 1);

        if (!ucActiveBuffer) {
            pusL = &usBuf1Pos;
            p = pbSendBuffer1 + usBuf1Pos;
        } else {
            pusL = &usBuf2Pos;
            p = pbSendBuffer2 + usBuf2Pos;
        }

        if (*pusL < (BUFFER_SIZE + 1)) {
            usTimer1 |= ((ucState)? 0x4000 : 0x8000);
            *p = (usTimer1 >> 8);
            p++;
            *p = (usTimer1 & 0xFF);
            ucTimeWritten = 0;

            (*pusL) += 2;
        } else {
// PANIC
        }

        if (ucState) usCounter++;
    }
}

ISR(TIMER1_COMPA_vect)
{
    volatile unsigned char *p;
    volatile unsigned short *pusL;
    PORTD ^= _BV(PD5);

    if (!ucActiveBuffer) {
        pusL = &usBuf1Pos;
        p = pbSendBuffer1 + usBuf1Pos;
    } else {
        pusL = &usBuf2Pos;
        p = pbSendBuffer2 + usBuf2Pos;
    }

    if (!ucTimeWritten) {
        if (*pusL < BUFFER_SIZE) {
            *p = 1; // one overflow of timer
            (*pusL)++;
            ucTimeWritten = 1;
        } else {
// PANIC
        }
    } else {
        if ((*p) <= 62) {
            (*p)++;
        } else {
            *p = 1; // one overflow of timer
            (*pusL)++;
        }
    }

    // Timer works incorrectly without this NOP!!! I do not know why. But delays are currupted.
    __asm("nop");
}

