// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include "FastLED.h"
#include "WiFi.h"
#include "ESPmDNS.h"
#include "NTPClient.h"
// libary is called "Time" by Michael Margolls
#include "TimeLib.h"
#include "WiFiUdp.h"
#include "ArduinoOTA.h"
#include "TouchButtonManager.h"
#include "LixieDisplay.h"
// #include "BluetoothSerial.h"
//#include <EEPROM.h>
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

// Create an SSL certificate object from the files included above
SSLCert cert = SSLCert(
  example_crt_DER, example_crt_DER_len,
  example_key_DER, example_key_DER_len
);

// First, we create the HTTPSServer with the certificate created above
HTTPSServer secureServer = HTTPSServer(&cert);

// Additionally, we create an HTTPServer for unencrypted traffic
HTTPServer insecureServer = HTTPServer();

// Declare some handler functions for the various URLs on the server
void handleRoot(HTTPRequest * req, HTTPResponse * res);
void handle404(HTTPRequest * req, HTTPResponse * res);

void handleRoot(HTTPRequest * req, HTTPResponse * res) {
  res->setHeader("Content-Type", "text/html");

  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Hello World!</title></head>");
  res->println("<body>");
  res->println("<h1>Hello World!</h1>");

  res->print("<p>Your server is running for ");
  res->print((int)(millis()/1000), DEC);
  res->println(" seconds.</p>");

  // You can check if you are connected over a secure connection, eg. if you
  // want to use authentication and redirect the user to a secure connection
  // for that
  if (req->isSecure()) {
    res->println("<p>You are connected via <strong>HTTPS</strong>.</p>");
  } else {
    res->println("<p>You are connected via <strong>HTTP</strong>.</p>");
  }

  res->println("</body>");
  res->println("</html>");
}

void handle404(HTTPRequest * req, HTTPResponse * res) {
  req->discardRequestBody();
  res->setStatusCode(404);
  res->setStatusText("Not Found");
  res->setHeader("Content-Type", "text/html");
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Not Found</title></head>");
  res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
  res->println("</html>");
}

void setupWebServer() {
  // For every resource available on the server, we need to create a ResourceNode
  // The ResourceNode links URL and HTTP method to a handler function
  ResourceNode * nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode * node404  = new ResourceNode("", "GET", &handle404);

  // Add the root node to the servers. We can use the same ResourceNode on multiple
  // servers (you could also run multiple HTTPS servers)
  secureServer.registerNode(nodeRoot);
  insecureServer.registerNode(nodeRoot);

  // We do the same for the default Node
  secureServer.setDefaultNode(node404);
  insecureServer.setDefaultNode(node404);

  Serial.println("Starting HTTPS server...");
  secureServer.start();
  Serial.println("Starting HTTP server...");
  insecureServer.start();
  if (secureServer.isRunning() && insecureServer.isRunning()) {
	  Serial.println("Servers ready.");
  }
}



void pollWebServer() {
    secureServer.loop();
	insecureServer.loop();
}











const char* ssid     = "4wire";
const char* password = "laserlaser";
// WiFiServer server(80);

//EEPROMClass  MODE("eeprom0", 0x1000);

#define TIMEZONE_OFFSET_HOURS   -7

#define NTP_UPDATE_INTERVAL     86400

#define LLC   11

#define TWO_PI 6.28318530718

#define DISPLAY_MODE_ADDR  0

#define DISPLAY_MODE_COUNT  2

CRGB testColors[] = {
    0xffffff,   // white
    0xff0000,   // red
    0x10ff00,   // green
    0x1010ff,   // blue
};

byte currentColor = 0;

// BluetoothSerial SerialBT;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "us.pool.ntp.org");

uint32_t long lastTimeUpdate = 0;

bool twelveHourTime = true;

uint8_t displayMode = 0;
bool displayModeChanged = true;

TouchButtonManager buttonManager;
LixieDisplay display;

void setupLEDs() {
    Serial.println("Initializing Lixie display...");
    display.setColor(&testColors[currentColor]);
    display.setBrightnessMode(LIXIE_BRIGHT);
    display.refresh();
}

void setupWifi() {
    Serial.printf("Connecting to %s...", ssid);
    WiFi.begin(ssid, password);
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(count > 25)ESP.restart();
        count++;
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
}

void setupArduinoOTA() {
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    })
    .onEnd([]() {
        Serial.println("\nEnd");
    })
    .onProgress([](uint16_t progress, uint16_t total) {
        Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
}

void setupNtpClient() {
    Serial.println("Initializing NTP client...");
    timeClient.begin();
    timeClient.setTimeOffset(TIMEZONE_OFFSET_HOURS * 3600);
    timeClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
}

void getNtpCurrentTime() {
    Serial.println("Requesting NTP time...");
    for (byte i = 1; i < 4; i++) {
        if (timeClient.update()) {
            return;
        } else {
            Serial.printf("Retry %d\r\n", i);
            timeClient.forceUpdate();
        }
    }
    Serial.println("Failed to connect to NTP server");
}

void setup() {
    // SerialBT.begin("ESP32-test");
    Serial.begin(115200);
    Serial.println();

    pinMode(LLC, OUTPUT);
    digitalWrite(LLC, LOW);
    buttonManager.registerButtonHandler(onButtonPress);

    setupLEDs();
    setupWifi();
    setupArduinoOTA();

    setupNtpClient();
    getNtpCurrentTime();

    setupWebServer();

    // server.begin();

    // displayMode = EEPROM.read(DISPLAY_MODE_ADDR);
}


// what do the buttons actually do, bob???
void onButtonPress(byte button, button_event_t event) {
    if (event == BUTTON_PRESS_RESET) {

    } else if (button == 0) {
        if (event == BUTTON_PRESS_LONG) {
            display.nextBrightnessMode();
            Serial.printf("Changed brightness mode: %d\r\n", display.getBrightnessMode());
        } else { // event == BUTTON_PRESS_SHORT
            if (display.getBrightnessMode() == LIXIE_OFF)
                display.nextBrightnessMode();

            currentColor = (currentColor + 1) % 4;
            display.setColor(&testColors[currentColor]);
            Serial.printf("Changed color mode: %d\r\n", currentColor);
        }

        display.refresh();

    } else if (button == 1) {
        // switch to next mode
        displayMode = (displayMode + 1) % 2;
        displayModeChanged = true;
        Serial.printf("Changed display mode: %d\r\n", displayMode);
    }
}
/*
// check for new client connections
void handleWebRequets() {
    WiFiClient client = server.available();

    if (!client)
        return;
                                        // if you get a client,
    Serial.println("New Client.");      // print a message out the serial port
    String currentLine = "";            // make a String to hold incoming data from the client
    while (client.connected()) {        // loop while the client's connected
        if (client.available()) {       // if there's bytes to read from the client,
            char c = client.read();     // read a byte, then
            Serial.write(c);            // print it out the serial monitor
            if (c == '\n') {            // if the byte is a newline character

                // if the current line is blank, you got two newline characters in a row.
                // that's the end of the client HTTP request, so send a response:
                if (currentLine.length() == 0) {
                    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                    // and a content-type so the client knows what's coming, then a blank line:
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();

                    // the content of the HTTP response follows the header:
                    client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
                    client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");

                    // The HTTP response ends with another blank line:
                    client.println();
                    // break out of the while loop:
                    break;
                } else {    // if you got a newline, then clear currentLine:
                    currentLine = "";
                }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
                currentLine += c;      // add it to the end of the currentLine
            }

            // TODO: make sure displayMode is constrained properly

            // Check to see if the client request was "GET /H" or "GET /L":
            if (currentLine.endsWith("GET /H")) {
                displayMode = (displayMode + 1) % DISPLAY_MODE_COUNT;   // GET /H turns the LED on
            }
            if (currentLine.endsWith("GET /L")) {
                displayMode = (displayMode - 1) % DISPLAY_MODE_COUNT;   // GET /L turns the LED off
            }
        }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
} */

void displayTime(bool forceUpdate) {
    uint32_t now = millis() / 1000;
    if (now != lastTimeUpdate || forceUpdate) {
        lastTimeUpdate = now;

        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        int seconds = timeClient.getSeconds();
        byte hour0;
        if (twelveHourTime) {
            hours %= 12;
            hours = hours == 0 ? 12 : hours;
            hour0 = hours < 10 ? LIXIE_BLANK_DIGIT : 1;   // don't display leading 0
        } else {
            hour0 = hours / 10;
        }
        display.digits[0] = hour0;
        display.digits[1] = hours % 10;
        display.digits[2] = minutes / 10;
        display.digits[3] = minutes % 10;
        display.digits[4] = seconds / 10;
        display.digits[5] = seconds % 10;
        display.refresh();
    }
}

void displayDate(bool forceUpdate) {
    uint32_t now = millis() / 1000;
    if (now != lastTimeUpdate || forceUpdate) {
        lastTimeUpdate = now;

        uint32_t timestamp = timeClient.getEpochTime();
        int d = day(timestamp);
        int m = month(timestamp);
        int y = year(timestamp) % 100;

        display.digits[0] = m / 10;
        display.digits[1] = m % 10;
        display.digits[2] = d / 10;
        display.digits[3] = d % 10;
        display.digits[4] = y / 10;
        display.digits[5] = y % 10;
        display.refresh();
    }
}

void displayNumber(uint32_t value) {
    for (int8_t i = 5; i >= 0; i--) {
        display.digits[i] = value % 10;
        value /= 10;
    }
    display.refresh();
}

void displayTest(bool forceUpdate) {
  uint32_t now = millis() / 1000;
    if (now != lastTimeUpdate || forceUpdate) {
        lastTimeUpdate = now;
    }

//        display.digits[0] = m / 10;
//        display.digits[1] = m % 10;
//        display.digits[2] = d / 10;
//        display.digits[3] = d % 10;
//        display.digits[4] = y / 10;
//        display.digits[5] = y % 10;
//        display.refresh();
}


void loop() {
    ArduinoOTA.handle();
    // handleWebRequets();
    pollWebServer();
    buttonManager.poll();
    timeClient.update();

    // SerialBT.printf("Touch Sensor: %d\r\n", touchRead(T5));
    switch (displayMode) {
        case 0: displayTime(displayModeChanged); break;
        case 1: displayDate(displayModeChanged); break;
    }

    displayModeChanged = false;
}
