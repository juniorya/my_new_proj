//
//
//

#ifndef __NET_H_20200615__
#define __NET_H_20200615__

#include <arpa/inet.h>

#define NET_EVENT_CONNECT  1
#define NET_EVENT_RECEIVE  2
#define NET_EVENT_CLOSE    3

typedef void (*net_callbackfunc)(int fd, unsigned char ucEvent, void **pCtx, unsigned char *pbBuffer, unsigned uSize);

int net_create_socket_to_listen(in_addr_t addr, unsigned uPort);

int net_connect(in_addr_t addr, unsigned uPort);

int net_send(int iFD, unsigned char *pbBuffer, unsigned uLen);

int net_run(int iRegSocket, int iCtrlSocket, net_callbackfunc reg_func, net_callbackfunc ctrl_func);

int net_shutdown_socket(int iFD);

void net_set_time_to_exit();

#endif /* __NET_H_20200615__ */
