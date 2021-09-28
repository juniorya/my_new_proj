//
//
//

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


void process_file(FILE *pSrc, FILE *pDst)
{
    unsigned char c = 0, bFirst = 1;
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
    fprintf(pDst, "{ \"data\": [");
    do {
        n = fread(&c, 1, 1, pSrc);
        if (n <= 0) break;

        if ((c & 0xc0) == 0x80) {
            // DOWN
            ulTmp = (c & 0x3f);
            n = fread(&c, 1, 1, pSrc);
            if (n <= 0) break;

            ulTmp <<= 8; ulTmp += c;
            ulTmp += ulTime;
            ulD = (ulTmp >= ulPrev)? (ulTmp - ulPrev) : (ulTmp + 0x3fff - ulPrev);
//            printf("D %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
            if (bFirst) bFirst = 0; else fprintf(pDst, ",");
            fprintf(pDst, "[%lu.%06lu,1],[%lu.%06lu,0]", ulTmp / 1000000, ulTmp % 1000000, ulTmp/1000000, ulTmp % 1000000 );
            ulPrev = ulTmp;
        } else
        if ((c & 0xc0) == 0x40) {
            // UP
            ulTmp = (c & 0x3f);
            n = fread(&c, 1, 1, pSrc);
            if (n <= 0) break;

            ulTmp <<= 8; ulTmp += c;
            ulTmp += ulTime;
            ulD = (ulTmp >= ulPrev)? (ulTmp - ulPrev) : (ulTmp + 0x3fff - ulPrev);
//            printf("U %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
            if (bFirst) bFirst = 0; else fprintf(pDst, ",");
            fprintf(pDst, "[%lu.%06lu,0],[%lu.%06lu,1]", ulTmp / 1000000, ulTmp % 1000000, ulTmp/1000000, ulTmp % 1000000 );
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
    } while (!feof(pSrc));
    fprintf(pDst, "]}\n");
}
