//
// proto
//

#ifndef __PROTO_H_20200615__
#define __PROTO_H_20200615__

typedef struct connection_item_t {
    char *pcFileName;
    int iFileDescr;
} connection_item;

/*
struct topology_item_t;

typedef struct topology_item_t {
    unsigned long ulFullAddress;
    unsigned long ulSerial;
    unsigned uLocalAddress;
    unsigned char ucLedStatus;
    unsigned long ulLastTimestamp;

    struct topology_item_t *pChildren;
    struct topology_item_t *pNext;
} topology_item;

int topology_request(topology_item_t **ppRes);

int topology_free(topology_item_t *pRes);
*/

//reg_callback
// registers data packet
void proto_reg_handler(int fd, unsigned char ucEvent, void **pCtx, unsigned char *pbBuffer, unsigned uSize);

//control_callback
// receives a command.
void proto_ctrl_handler(int fd, unsigned char ucEvent, void **pCtx, unsigned char *pbBuffer, unsigned uSize);

#endif /* __PROTO_H_20200615__ */

