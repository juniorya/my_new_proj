
//#include "arch.h"
#include <config.h>
#include <Arduino.h>
#include <Wire.h>


// i2c communications

static char ci2c_process = 0;


extern "C" void *i2c_get(unsigned char ucNum)
{
    return &ci2c_process;
}

extern "C" unsigned char i2c_master_send(void *pInstance, unsigned short usAddr, unsigned char *pbBuffer, unsigned short usLen, unsigned char usFinal)
{
    unsigned char ucRes = 0;
    if (!ci2c_process) {
        Wire.beginTransmission((unsigned char)(usAddr >> 1));
        ci2c_process = 1;
    }
    Wire.write(pbBuffer, usLen);
    if (usFinal) {
        ucRes = Wire.endTransmission();
        ci2c_process = 0;
    }
    return ucRes;
}

extern "C" unsigned char i2c_master_receive(void *pInstance, unsigned short usAddr, unsigned char *pbBuffer, unsigned short usLen, unsigned char usFinal)
{
    unsigned char x, ucRes = Wire.requestFrom((unsigned char)(usAddr >> 1), usLen, usFinal);
    if (ucRes != usLen) return 0xFF;

    for(x = 0; x < ucRes; x++) pbBuffer[x] = Wire.read();
    return 0;
}

extern "C" unsigned char i2c_io_ready(void *pInstance)
{
    return 1;
}

