//
//
//

#include "config.h"
#include "proto.h"
#include "net.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define STRING_INT(s) #s
#define STRING(s) STRING_INT(s)

// parse arguments
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

int print_usage()
{
    printf("USAGE...\r\n");
    return 1;
}

// check presence in memory / daemonize / exit

int main(int argc, char *argv[])
{
    // TODO: remove from here
    char pcAnswer[256];
    unsigned uSize = sizeof(pcAnswer);

    pid_t pID;
    int rc, n, iFD, iRegSocket, iCtrlSocket;
    unsigned char ucOption, ucParam;

/*
    if (argc < 2) {
        print_usage();
        return 0;
    }

    if (parse_command_line(argc, argv, &ucOption, &ucParam) != 1) {
        printf("Bad options. Run without parameters to get usage.\r\n");
        return 1;
    }
*/
    ucOption = 1; ucParam = 1;

    switch(ucOption) {
        case 1:
            // if daemon, try to daemonize
            iRegSocket = net_create_socket_to_listen(inet_addr(STRING(REG_ENDPOINT_ADDR)), REG_ENDPOINT_PORT);
            if (iRegSocket <= 0) {
                printf("Error create listening socket at %s:%u. Maybe daemon already started.\r\n", STRING(REG_ENDPOINT_ADDR), REG_ENDPOINT_PORT);
                return 2;
            }
            iCtrlSocket = net_create_socket_to_listen(inet_addr(STRING(CTRL_ENDPOINT_ADDR)), CTRL_ENDPOINT_PORT);
            if (iCtrlSocket <= 0) {
                printf("Error create listening socket at %s:%u. Maybe daemon already started.\r\n", STRING(CTRL_ENDPOINT_ADDR), CTRL_ENDPOINT_PORT);
                return 2;
            }

            pID = 0; //fork();
            if (pID == 0) {
                // go process net
                rc = net_run(iRegSocket, iCtrlSocket, proto_reg_handler, proto_ctrl_handler);
            } else
            if (pID < 0) {
                printf("Fork error %d\r\n", pID);
                return 2;
            } else {
                printf("%s", "Started\r\n");
            }
            break;
        case 2:
            // if exit, sends exit command through control socket
            iFD = net_connect(inet_addr(STRING(CTRL_ENDPOINT_ADDR)), CTRL_ENDPOINT_PORT);
            if (iFD <= 0) {
                printf("Error connect to send exit command. Maybe the server is not started.\r\n");
                return 2;
            }
            if (net_send(iFD, (unsigned char *)(char *)"EXIT", 4) < 0) {
                close(iFD);
                printf("Error send exit command.\r\n");
                return 2;
            }
            // TODO: read "OK"

            close(iFD);
            printf("%s", "call exit\r\n");
            break;

        case 3:
            // if status, proto_command(request_topology"), dump the topology line by line
            iFD = net_connect(inet_addr(STRING(CTRL_ENDPOINT_ADDR)), CTRL_ENDPOINT_PORT);
            if (iFD <= 0) {
                printf("Error connect to send status command. Maybe the server is not started.\r\n");
                return 2;
            }
            if (net_send(iFD, (unsigned char *)(char *)"STATUS", 6) < 0) {
                close(iFD);
                printf("Error send status command.\r\n");
                return 2;
            }
            // TODO: read answer
            n = read(iFD, pcAnswer, uSize-1);
            if ((n > 0) && (n < uSize)) {
                pcAnswer[n] = 0;
                printf("Response: %s\r\n", (char *)pcAnswer);
            } else {
                printf("%s", "Error read answer.\r\n");
            }
            close(iFD);
            break;

        case 4:
            // if command, sends a command to net
                break;
    }

    return rc;
}