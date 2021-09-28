//
//
//

#include "net_reg.h"
#include "reg_alg.h"
#include <Arduino.h>
#include <Ethernet.h>


///////////////////////////////////////////////////////////////
//

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(169, 254, 0, 200);

#define TCP_PORT 19191

// Enter the IP address of the server you're connecting to:
IPAddress server(169, 254, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 23 is default for telnet;
// if you're using Processing's ChatServer, use port 10002):
EthernetClient client;


unsigned char netInit(void)
{
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    pinMode(10, OUTPUT);
    Ethernet.init(10);
    Ethernet.begin(mac, ip);

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        return 0;
    }
    return 1;
}

unsigned char netEvent(unsigned char ucEvent)
{
    unsigned char *pBuf;
    unsigned short usBufSize;
    unsigned char ucRes = 0;
    if (ucEvent == RA_NET_TICK) {

        if (client.connected() && (client.availableForWrite())) {
            if (!regAlgGetBufferToSend(&pBuf, &usBufSize)) return 1;

            if (!(client.write(pBuf, usBufSize))) {
Serial.println("PANIC!!!");
                return 0;
            } else
                ucRes = 1;
        }
    } else
    if (ucEvent == RA_NET_CONNECT) {
        if (Ethernet.linkStatus() == LinkOFF) return 0;

        // if you get a connection, report back via serial:
        if (client.connect(server, TCP_PORT)) ucRes = 1;

// TODO: send header

    } else
    if (ucEvent == RA_NET_FINISH) {
        client.stop();
    }
    return ucRes;
}

