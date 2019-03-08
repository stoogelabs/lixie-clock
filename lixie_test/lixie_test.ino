// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include "FastLED.h"
#include "WiFi.h"
#include "ESPmDNS.h"
#include "NTPClient.h"
#include "TimeLib.h"
#include "WiFiUdp.h"
#include "ArduinoOTA.h"
#include "TouchButtonManager.h"
// #include "BluetoothSerial.h"
//#include <EEPROM.h>

const char* ssid     = "4wire";
const char* password = "laserlaser";
WiFiServer server(80);

//EEPROMClass  MODE("eeprom0", 0x1000);

#define NTP_UPDATE_INTERVAL     86400

#define PIXEL_PIN_1    15   // Digital IO pin connected to the NeoPixels.
#define PIXEL_PIN_2    5
#define PIXEL_PIN_3    14

#define LLC   11

#define PIXELS_PER_DIGIT 20
#define DIGITS_PER_STRIP 2
#define STRIP_COUNT 3
#define DIGIT_COUNT STRIP_COUNT * DIGITS_PER_STRIP
#define PIXELS_PER_STRIP PIXELS_PER_DIGIT * DIGITS_PER_STRIP

#define TWO_PI 6.28318530718

#define DISPLAY_MODE_ADDR  0

CRGB ledStrips[STRIP_COUNT][PIXELS_PER_STRIP];

CRGB testColors[] = {
    0xffffff,   // white
    0xff0000,   // red
    0x10ff00,   // green
    0x1010ff,   // blue
};

byte currentColor = 0;

// BluetoothSerial SerialBT;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

uint32_t long lastTimeUpdate = 0;

bool twelveHourTime = false;

uint8_t displayMode = 0;
bool displayModeChanged = true;

TouchButtonManager buttonManager;

byte brightnessMode = 0;
const byte brightnessValues[] = {255, 80, 0};

void setupLEDs() {
    Serial.println("Initializing FastLED neopixel strips...");
    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_1>(ledStrips[0], PIXELS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_2>(ledStrips[1], PIXELS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_3>(ledStrips[2], PIXELS_PER_STRIP);
}

void setupWifi() {
    Serial.printf("Connecting to %s...", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    //Serial.printf("Connected! IP address: %s\r\n", WiFi.localIP());
    Serial.println("Connected!");
    Serial.print("IP address: ");
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
    timeClient.setTimeOffset(-8 * 3600);
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

    server.begin();

    // displayMode = EEPROM.read(DISPLAY_MODE_ADDR);
}

// what do the buttons actually do, bob???
void onButtonPress(byte button, button_event_t event) {
    if (event == BUTTON_PRESS_RESET) {

    } else if (button == 0) {
        if (event == BUTTON_PRESS_LONG){
            brightnessMode = (brightnessMode + 1) % 3;
        } else if (brightnessMode <= 2) {
            brightnessMode = 0;
        } else {
            currentColor = (currentColor + 1) % 4;
            displayTime(true);
        }
    } else if (button == 1) {
        // switch to next mode
        displayMode = (displayMode + 1) % 2;
        displayModeChanged = true;
    }
}

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
                displayMode++;               // GET /H turns the LED on
            }
            if (currentLine.endsWith("GET /L")) {
                displayMode--;                // GET /L turns the LED off
            }
        }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
}

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
            hour0 = hours < 10 ? 255 : 1;   // don't display leading 0
        } else {
            hour0 = hours / 10;
        }

        CRGB color = testColors[currentColor];
        color.nscale8(brightnessValues[brightnessMode]);
        setAllDigits(
            hour0,
            hours % 10,
            minutes / 10,
            minutes % 10,
            seconds / 10,
            seconds % 10,
            &color
        );
    }
}

void displayDate(bool forceUpdate) {
    uint32_t now = millis() / 1000;
    if (now != lastTimeUpdate || forceUpdate) {
        lastTimeUpdate = now;

        CRGB color = testColors[currentColor];
        color.nscale8(brightnessValues[brightnessMode]);

        uint32_t timestamp = timeClient.getEpochTime();
        int d = day(timestamp);
        int m = month(timestamp);
        int y = year(timestamp) % 100;

        setAllDigits(
            m / 10,
            m % 10,
            d / 10,
            d % 10,
            y / 10,
            y % 10,
            &color
        );
    }
}

void displayNumber(uint32_t value) {
    CRGB color = testColors[currentColor];
    color.nscale8(brightnessValues[brightnessMode]);

    for (int8_t i = 5; i >= 0; i--) {
        setDigit(i, value % 10, &color);
        value /= 10;
    }
}


void loop() {
    ArduinoOTA.handle();
    handleWebRequets();
    buttonManager.poll();
    timeClient.update();

    // SerialBT.printf("Touch Sensor: %d\r\n", touchRead(T5));
    switch (displayMode) {
        case 0: displayTime(displayModeChanged); break;
        case 1: displayDate(displayModeChanged); break;
    }

    displayModeChanged = false;
}

void clearPixels(byte stripIndex, byte offset, byte count) {
    for (byte i = 0; i < count; i++) {
        ledStrips[stripIndex][i + offset] = CRGB::Black;
    }
}

void setAllDigits(byte val0, byte val1, byte val2, byte val3, byte val4, byte val5, CRGB *color) {
    setDigit(0, val0, color);
    setDigit(1, val1, color);
    setDigit(2, val2, color);
    setDigit(3, val3, color);
    setDigit(4, val4, color);
    setDigit(5, val5, color);
}

void setDigit(byte index, byte value, CRGB *color) {
    byte stripIndex = index / DIGITS_PER_STRIP;

    // digit  offsets
    // 0      5, 15
    // 1      4, 14
    // 2      6, 16
    // 3      3, 13
    // 4      7, 17
    // 5      2, 12
    // 6      8, 18
    // 7      1, 11
    // 8      9, 19
    // 9      0, 10

    byte pixelOffset = (index % DIGITS_PER_STRIP) * PIXELS_PER_DIGIT;
    clearPixels(stripIndex, pixelOffset, PIXELS_PER_DIGIT);

    if (value < 10) {
        byte isOdd = value % 2;
        byte halfValue = value / 2;
        // even numbers count up from 5, odd numbers count down from 4
        byte pixel0 = (isOdd * (4 - halfValue)) + ((1 - isOdd) * (5 + halfValue)) + pixelOffset;
        byte pixel1 = pixel0 + 10;

        ledStrips[stripIndex][pixel0] = *color;
        ledStrips[stripIndex][pixel1] = *color;
    }

    FastLED.show();
}
