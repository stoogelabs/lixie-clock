#include "DebugServer.h"

WiFiClient DebugServer::clients[MAX_SRV_CLIENTS];

WiFiServer DebugServer::server(DEBUG_SERVER_PORT);

void DebugServer::setup() {
    DebugServer::server.begin();
    DebugServer::server.setNoDelay(true);
}

void DebugServer::print(const char *str) {
    for (byte i = 0; i < MAX_SRV_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            clients[i].write(str);
        }
    }
}

size_t DebugServer::printf(const char *format, ...) {
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = new char[len+1];
        if(temp == NULL) {
            return 0;
        }
    }
    len = vsnprintf(temp, len+1, format, arg);

    for (byte i = 0; i < MAX_SRV_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            clients[i].write(temp, len);
        }
    }
    va_end(arg);
    if(len >= sizeof(loc_buf)){
        delete[] temp;
    }
    return len;
}

void DebugServer::listen() {
    uint8_t i;
    //check if there are any new clients
    if (server.hasClient()) {
        for (i = 0; i < MAX_SRV_CLIENTS; i++) {
            //find free/disconnected spot
            if (!clients[i] || !clients[i].connected()) {
                if(clients[i])
                    clients[i].stop();
                clients[i] = DebugServer::server.available();
                clients[i].write("Connected to Stooge Labs DebugServer!\n");
                continue;
            }
        }
        //no free/disconnected spot so reject
        WiFiClient serverClient = DebugServer::server.available();
        serverClient.stop();
    }
}
