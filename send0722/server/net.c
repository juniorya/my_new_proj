//
//
//

#include "net.h"

#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#define log_error(...) syslog(LOG_ERR, __VA_ARGS__)

#define SOCK_ARR_LEN 10
#define SOCK_CHUNK 512

int net_create_socket_to_listen(in_addr_t addr, unsigned uPort)
{
//    struct addrinfo hints;
    struct sockaddr_in conn_addr;
    struct sockaddr srv_addr;

//    memset(&hints,0,sizeof(hints));
    memset(&conn_addr,0,sizeof(srv_addr));
    memset(&srv_addr,0,sizeof(srv_addr));

//    hints.ai_family=AF_UNSPEC;
//    hints.ai_socktype=SOCK_DGRAM;
//    hints.ai_protocol=0;
//    hints.ai_flags=AI_PASSIVE|AI_ADDRCONFIG;
//    hints.ai_addr = (struct sockaddr *)&conn_addr;
//    hints.ai_addrlen = sizeof(conn_addr);
    conn_addr.sin_port = htons(uPort);
    conn_addr.sin_family = AF_INET;
    conn_addr.sin_addr.s_addr = addr; //inet_addr("127.0.0.1");

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        log_error("socket %s\n",strerror(errno));
    } else {

        int optval = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

        if (bind(fd, (struct sockaddr *)&conn_addr, sizeof(conn_addr)) == -1) {
            log_error("bind %s\n",strerror(errno));
            close(fd); fd = -1;
        } else {
            fcntl(fd, F_SETFL, O_NONBLOCK);
        }
    }
    return fd;
}

// client part
int net_connect(in_addr_t addr, unsigned uPort)
{
    int fd;
    struct sockaddr_in si_conn;

    // returns connected socket
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        log_error("socket %s",strerror(errno));
        return -1;
    }

    memset((char *)&si_conn, 0, sizeof(si_conn));
    si_conn.sin_family = AF_INET;
    si_conn.sin_port = htons(uPort);
    si_conn.sin_addr.s_addr = addr;

    if(connect(fd, (struct sockaddr *)&si_conn, sizeof(si_conn)) < 0)  {
        log_error("Error : Connect Failed");
        close(fd);
        return -1;
    }

    return fd;
}

// server part

typedef struct {
    int iFD;
    void *pCtx;
} datasocket;

static int iRegFD, iCtrlFD;
//static int regConnected[SOCK_ARR_LEN];
static datasocket regConnected[SOCK_ARR_LEN];
static unsigned uRegCount = 0;
//static int ctrlsConnected[SOCK_ARR_LEN];
static datasocket ctrlsConnected[SOCK_ARR_LEN];
static unsigned uCtrlsCount = 0;
static net_callbackfunc fCtrl = 0, fReg = 0;
static unsigned char gTimeToExit = 0;
static unsigned char gRcBuffer[SOCK_CHUNK];

#define DISCONNECTED_HIVES 1
#define DISCONNECTED_CTRL  2

// maybe in separate thread? - hope no, all events have to be buffered
int net_cycle()
{
    struct sockaddr srv_addr;
    socklen_t addrlen = 0;
    fd_set readfds, writefds;
    struct timeval tv;
    int rc = 0, s, n, iMax;
    unsigned u = 0, w;
    unsigned char ucFlags = 0;

//    struct sockaddr srv_addr;

// fill sockets with
    // 2 listening
    // all control
    // all to-reg

    while(!gTimeToExit) {
        ucFlags = 0;
        FD_ZERO(&readfds); FD_ZERO(&writefds);
        tv.tv_sec = 0;
        tv.tv_usec = 50; // maybe to parameters

        FD_SET(iRegFD, &readfds); iMax = iRegFD;
        FD_SET(iCtrlFD, &readfds); if (iMax < iCtrlFD) iMax = iCtrlFD;

        for(u = 0; u < uRegCount; u++) {
            FD_SET(regConnected[u].iFD, &readfds);
            if (regConnected[u].iFD > iMax) iMax = regConnected[u].iFD;
        }
        for(u = 0; u < uCtrlsCount; u++) {
            FD_SET(ctrlsConnected[u].iFD, &readfds);
            if (ctrlsConnected[u].iFD > iMax) iMax = ctrlsConnected[u].iFD;
        }

        s = select(iMax + 1, &readfds, &writefds, NULL, &tv);
        if ((s == 0) || ((s == -1) && (errno == EINTR)))
            continue;
        if (s == -1) {
            log_error("Select() error. Code %d.", errno);
            rc = 0;
            break;
        }

        // check listening sockets for accept
        if (FD_ISSET(iRegFD, &readfds)) {
            if (uRegCount < SOCK_ARR_LEN) {
                regConnected[uRegCount].iFD = accept(iRegFD, (struct sockaddr *) &srv_addr, &addrlen);
// TODO: maybe log connection from srv_addr

                if (regConnected[uRegCount].iFD > 0) {
                    regConnected[uRegCount].pCtx = 0;
                    fReg(regConnected[uRegCount].iFD, NET_EVENT_CONNECT, &(regConnected[uRegCount].pCtx), 0, 0);
                    uRegCount++;
                } else {
                    log_error("Error accepting new reg - %s", strerror(errno));
                }
            } else {
                log_error("Can not accept new reg connection - too many sockets");
            }
        }

        if (FD_ISSET(iCtrlFD, &readfds)) {
            // new control connection.
            if (uCtrlsCount < SOCK_ARR_LEN) {
                ctrlsConnected[uCtrlsCount].iFD = accept(iCtrlFD, (struct sockaddr *) &srv_addr, &addrlen);
// TODO: maybe log connection from srv_addr

                if (ctrlsConnected[uCtrlsCount].iFD > 0) {
                    fCtrl(ctrlsConnected[uCtrlsCount].iFD, NET_EVENT_CONNECT, &(ctrlsConnected[uCtrlsCount].pCtx), 0, 0);
                    uCtrlsCount++;
                } else {
                    log_error("Error accepting new ctrl - %s", strerror(errno));
                }
            } else {
                log_error("Can not accept new ctrl connection - too many sockets");
            }
        }

        // process reading from net
        for(u = 0; u < uRegCount; u++) {
            if (FD_ISSET(regConnected[u].iFD, &readfds)) {

                // if read return 0, we have to close the socket. BUT:
                // 1. set it 0 in regConnected
                // 2. set "NEED cleanup reg"

                // notify logic level

                n = read(regConnected[u].iFD, gRcBuffer, sizeof(gRcBuffer));
                if (n <= 0) {
                    // socket error or if has to be disconnected
                    if (n < 0) {
                        log_error("Error %s on socket %d", strerror(errno), ctrlsConnected[u].iFD);
                    }

                    fReg(regConnected[u].iFD, NET_EVENT_CLOSE, &(regConnected[u].pCtx), 0, 0);
                    close(regConnected[u].iFD);
                    regConnected[u].iFD = 0;
                    ucFlags |= DISCONNECTED_HIVES;
                } else {
                    fReg(regConnected[u].iFD, NET_EVENT_RECEIVE, &(regConnected[u].pCtx), gRcBuffer, n);
                }

            }
        }

        for(u = 0; u < uCtrlsCount; u++) {
            if (FD_ISSET(ctrlsConnected[u].iFD, &readfds)) {
                n = read(ctrlsConnected[u].iFD, gRcBuffer, sizeof(gRcBuffer));
                if (n <= 0) {
                    // socket error or if has to be disconnected
                    if (n < 0) {
                        log_error("Error %s on socket %d", strerror(errno), ctrlsConnected[u].iFD);
                    }
                    fCtrl(ctrlsConnected[u].iFD, NET_EVENT_CLOSE, &(ctrlsConnected[u].pCtx), 0, 0);
                    close(ctrlsConnected[u].iFD);
                    ctrlsConnected[u].iFD = 0;
                    ucFlags |= DISCONNECTED_CTRL;
                } else {
                    fCtrl(ctrlsConnected[u].iFD, NET_EVENT_RECEIVE, &(ctrlsConnected[u].pCtx), gRcBuffer, n);
                }
            }
        }

        if (ucFlags & DISCONNECTED_CTRL) {
            // purge some
            for(u = 0, w = 0; w < uCtrlsCount; w++) {
                if (ctrlsConnected[w].iFD > 0) {
                    if (u != w) {
                        ctrlsConnected[u].iFD = ctrlsConnected[w].iFD;
                    }
                    u++;
                }
            }
            uCtrlsCount = u;
        }

        if (ucFlags & DISCONNECTED_HIVES) {
            // purge some
            for(u = 0, w = 0; w < uRegCount; w++) {
                if (regConnected[w].iFD > 0) {
                    if (u != w) {
                        regConnected[u].iFD = regConnected[w].iFD;
                    }
                    u++;
                }
            }
            uRegCount = u;
        }

    }

    // clean all... release all, close all the sockets
    return rc;
}

int net_send(int iFD, unsigned char *pbBuffer, unsigned uLen)
{
    unsigned uPos = 0, uChunk, uLeft = uLen;
    ssize_t s;
    while(uLeft) {
        uChunk = uLeft; if (uChunk > SOCK_CHUNK) uChunk = SOCK_CHUNK;
        s = write(iFD, pbBuffer + uPos, uChunk);
        if (s < 0) {
            log_error("Error write to socket. %s", strerror(errno));
            return -1;
        }
        uLeft -= s; uPos += s;
    }
    return (int)uLen;
}

int net_run(int iRegSocket, int iCtrlSocket, net_callbackfunc reg_func, net_callbackfunc ctrl_func)
{
    if (listen(iRegSocket, 5)) {
        syslog (LOG_ERR, "Error listen reg endpoint.");
        return 0;
    }
    if (listen(iCtrlSocket, 5)) {
        syslog (LOG_ERR, "Error listen ctrl endpoint.");
        return 0;
    }
    iRegFD = iRegSocket;
    iCtrlFD = iCtrlSocket;

    // set callbacks
    fReg = reg_func;
    fCtrl = ctrl_func;

    return net_cycle();
}


int net_shutdown_socket(int iFD)
{
    // invoke shutdown on socket; in case we're server, remove it from active control / reg
}

int shutdown_all()
{
    // shutdons all include listening
}

void net_set_time_to_exit()
{
    gTimeToExit = 1;
}