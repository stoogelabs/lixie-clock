#ifndef _LixieWebApi_h
#define _LixieWebApi_h

#include "Arduino.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include "HTTPSServer.hpp"
#include "HTTPServer.hpp"
#include "SSLCert.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "cert.h"
#include "private_key.h"

// The HTTPS Server comes in a separate namespace. For easier use, include it here.
using namespace httpsserver;

class LixieWebApi {
    public:
        WiFiServer server;

        LixieWebApi();

    private:
        void onRootRequest(HTTPRequest * req, HTTPResponse * res);
        void onResourceNotFound(HTTPRequest * req, HTTPResponse * res);
};

#endif
