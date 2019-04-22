#ifndef _DebugServer_h
#define _DebugServer_h

#include "WiFi.h"

#define MAX_SRV_CLIENTS 2
#define DEBUG_SERVER_PORT 23

class DebugServer {
    private:
        static WiFiServer server;

        static WiFiClient clients[MAX_SRV_CLIENTS];

        DebugServer();
    public:
        static void setup();

        static void print(const char *str);

        static size_t printf(const char * format, ...);

        static void listen();
};

#endif
