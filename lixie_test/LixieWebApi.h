#ifndef _LixieWebApi_h
#define _LixieWebApi_h

#include "Arduino.h"
#include "WiFi.h"

class LixieWebApi {
    public:
        WiFiServer server;

        LixieWebApi();
};

#endif
