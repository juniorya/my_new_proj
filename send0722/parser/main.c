//
//
//

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


// parse arguments
/*
int parse_command_line(int argc, char *argv[], unsigned char *pucOption, unsigned char *pucParam)
{
    int rc = 1;
    const char *pcO = argv[1];
    if (strcmp(pcO, "daemon") == 0) *pucOption = 1; else
    if (strcmp(pcO, "exit") == 0) *pucOption = 2; else
    if (strcmp(pcO, "status") == 0) *pucOption = 3; else
    if (strcmp(pcO, "send") == 0) {
// netaddr, command
// later
//        *pucOption = 4;
        rc = 0;
    } else
        rc = 0;

    return rc;
}
*/
int print_usage()
{
    printf("parser [filename]\r\n");
    return 1;
}


void process_file(FILE *pFile)
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
            printf("D %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
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
            printf("U %lu.%06lu %lu.%06lu\n", ulTmp / 1000000, ulTmp % 1000000, ulD/1000000, ulD % 1000000 );
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

int main(int argc, char *argv[])
{
    int rc = 1;
    FILE *pFile;
    char *pcFileName;

    unsigned char ucFileExists = 0;

    if (argc < 2) {
        print_usage();
        return 0;
    }

    pcFileName = argv[1];

    pFile = fopen(pcFileName, "rb");
    if (pFile) {
        process_file(pFile);
        fclose(pFile);
    } else {
        printf("Error file not exists\n");
        return 2;
    }
/*

    if (parse_command_line(argc, argv, &ucOption, &ucParam) != 1) {
        printf("Bad options. Run without parameters to get usage.\r\n");
        return 1;
    }
*/
    return rc;
}