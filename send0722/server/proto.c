//
// proto
//

#include "proto.h"
#include "config.h"
#include "net.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <stdio.h>
#include <time.h>

#define STRING_INT(s) #s
#define STRING(s) STRING_INT(s)


int exchange()
{
    int iFD;
    // connect to our control addr/port
    if ((iFD = net_connect(inet_addr(STRING(CTRL_ENDPOINT_ADDR)), CTRL_ENDPOINT_PORT)) > 0) {
        // consctruct control packet (request net state, transmit command to hives

//        net_send(); // send

//        while() {
//            net_receive
//        }

        net_shutdown_socket(iFD);
    }
    return 1;
}

static unsigned long ulong_from_buf_lsb(unsigned char *pbBuffer)
{
    unsigned long ul = pbBuffer[3];
    ul <<= 8; ul += pbBuffer[2];
    ul <<= 8; ul += pbBuffer[1];
    ul <<= 8; ul += pbBuffer[0];
    return ul;
}

static unsigned long ulong_from_buf(unsigned char *pbBuffer)
{
    unsigned long ul = pbBuffer[0];
    ul <<= 8; ul += pbBuffer[1];
    ul <<= 8; ul += pbBuffer[2];
    ul <<= 8; ul += pbBuffer[3];
    return ul;
}

void process_file(FILE *pFile, FILE *pTarget)
{
    unsigned char c = 0;
    unsigned long ulTime = 0, ulPrev = 0;
    unsigned long ulTmp, ulD;
    int n;

/*    if (uFAddr) {
        if (fseek(pFILE, uFAddr, SEEK_SET) != 0) {
            printf("Error positioning file to %u.\n", uFAddr);
            return 0;
        }
    }
*/
    do {
        n = fread(&c, 1, 1, pFile);
        if (n <= 0) break;

        if ((c & 0xc0) == 0x80) {
            // DOWN
            ulTmp = (c & 0x3f);
            n = fread(&c, 1, 1, pFile);
            if (n <= 0) break;

            ulTmp <<= 8; ulTmp += c;
            ulTmp += ulTime;
            ulD = (ulTmp >= ulPrev)? (ulTmp - ulPrev) : (ulTmp + 0x3fff - ulPrev);
            fprintf(pTarget, "D %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
            ulPrev = ulTmp;
        } else
        if ((c & 0xc0) == 0x40) {
            // UP
            ulTmp = (c & 0x3f);
            n = fread(&c, 1, 1, pFile);
            if (n <= 0) break;

            ulTmp <<= 8; ulTmp += c;
            ulTmp += ulTime;
            ulD = (ulTmp >= ulPrev)? (ulTmp - ulPrev) : (ulTmp + 0x3fff - ulPrev);
            fprintf(pTarget, "U %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
            ulPrev = ulTmp;
        } else {
            ulTmp = c;
            ulTmp *= 0x3fff;
            ulTime += ulTmp;
        }

/*
        if (!eeprom_write_chunk(pM, ucSlaveID, usReg, ulAddr, pcChunk, n)) {
            printf("Error write to EEPROM.\n");
            return 0;
        }

        ulAddr += n;
*/
    } while (!feof(pFile));
}


void proto_reg_handler(int fd, unsigned char ucEvent, void **pCtx, unsigned char *pbBuffer, unsigned uSize)
{
    connection_item *pI;
    mode_t mode;
    if (ucEvent == NET_EVENT_RECEIVE) {
        pI = (connection_item *)(*pCtx);
        if (pI->iFileDescr) {
            write(pI->iFileDescr, pbBuffer, uSize);
//            printf("Got %u bytes\n", uSize);
        } else
            printf("Got %u bytes but not save\n", uSize);
    } else
    if (ucEvent == NET_EVENT_CONNECT) {
        *pCtx = malloc(sizeof(connection_item));
        pI = (connection_item *)(*pCtx);
        pI->iFileDescr = 0;
        pI->pcFileName = 0;

        pI->pcFileName = tempnam("/tmp", "mg_p_");
        if (pI->pcFileName) {
            mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            pI->iFileDescr = open(pI->pcFileName, O_CREAT | O_RDWR, mode);
            printf("Connected client, %s\n", pI->pcFileName);
        }

    } else
    if (ucEvent == NET_EVENT_CLOSE) {
        pI = (connection_item *)(*pCtx);
        printf("Disconnected client, %s\n", pI->pcFileName);

        char buf[128];
        strcpy(buf, pI->pcFileName); strcat(buf, ".txt");
        FILE *pFile = fopen(pI->pcFileName, "rb");
        FILE *pTarget = fopen(buf, "w");
        process_file(pFile, pTarget);
        fclose(pFile); fclose(pTarget);

        if (pI->pcFileName) free(pI->pcFileName);
        if (pI->iFileDescr) close(pI->iFileDescr);

        free(pI);
    }


/*
    char pcBuffer[100];
    unsigned uLen = sizeof(pcBuffer);
    if (uSize < 14) return;

    unsigned long ulW = ulong_from_buf_lsb(pbBuffer + 6); //*(unsigned long *)(pbBuffer + 6);

    FILE *pFile = fopen("hiveslog.txt", "a");
    if (pFile) {

        time_t t = time(NULL);
          struct tm tm = *localtime(&t);
          fprintf(pFile, "%d-%02d-%02d %02d:%02d:%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        unsigned long ulAddr = ulong_from_buf(pbBuffer + 1); //*(unsigned long *)(pbBuffer + 6);
        unsigned long ulTS = ulong_from_buf_lsb(pbBuffer + 10); //*(unsigned long *)(pbBuffer + 10);

        fprintf(pFile, "0x%08lx %c %lu %lu\n", ulAddr, pbBuffer[5], ulW, ulTS);
        fclose(pFile);

    }

    sprintf(pcBuffer, "http://feed.beesystem.ru/report?artefact=eaa6aecc-bcb3-11ea-afea-8731deeb30b0&adc_raw=%lu", ulW);
    callurl_slow(pcBuffer, pcBuffer, &uLen);
*/
}

void proto_ctrl_handler(int fd, unsigned char ucEvent, void **pCtx, unsigned char *pbBuffer, unsigned uSize)
{
    if (ucEvent == NET_EVENT_RECEIVE) {
        if ((uSize == 4) && (!memcmp(pbBuffer, "EXIT", 4))) {
            net_set_time_to_exit();
            net_send(fd, (unsigned char *)(char *)"OK", 2);
        } else {
            net_send(fd, (unsigned char *)(char *)"HELLO", 5);
        }
    }
}
