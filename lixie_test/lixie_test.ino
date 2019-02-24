// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include "FastLED.h"
//#include <EEPROM.h>

//EEPROMClass  MODE("eeprom0", 0x1000);

#define BUTTON_PIN   T5   // Digital IO pin connected to the button.  This will be
#define TRIGGER_PIN  13   // driven with a pull-up resistor so the switch should
                          // pull the pin to ground momentarily.  On a high -> low
                          // transition the button press logic will execute.

#define PIXEL_PIN_1    15   // Digital IO pin connected to the NeoPixels.
#define PIXEL_PIN_2    5
#define PIXEL_PIN_3    14

#define LLC   11

#define PIXELS_PER_DIGIT 20
#define DIGITS_PER_STRIP 2
#define STRIP_COUNT 3
#define DIGIT_COUNT STRIP_COUNT * DIGITS_PER_STRIP
#define PIXELS_PER_STRIP PIXELS_PER_DIGIT * DIGITS_PER_STRIP

#define BUTTON_COOLDOWN  100 // minimum time in miliseconds between button presses (to reduce noise)
#define BUTTON_LONG_PRESS  600 // miliseconds between button down and up to count as a long press

#define TWO_PI 6.28318530718

#define DISPLAY_MODE_ADDR  0

CRGB ledStrips[STRIP_COUNT][PIXELS_PER_STRIP];

CRGB testColors[] = {
    CRGB::White,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
};

uint8_t displayMode = 0;
bool buttonState = HIGH;   // LOW = pressed
bool buttonHeld = false;
unsigned long lastButtonCheck = 0;
unsigned long buttonDownTime = 0;
float brightness = 0.0;

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid     = "4wire";
const char* password = "laserlaser";

WiFiServer server(80);

int value = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(PIXEL_PIN_1, OUTPUT);
    pinMode(PIXEL_PIN_2, OUTPUT);
    pinMode(PIXEL_PIN_3, OUTPUT);
    pinMode(LLC, OUTPUT);
    digitalWrite(LLC, LOW);

    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_1>(ledStrips[0], PIXELS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_2>(ledStrips[1], PIXELS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, PIXEL_PIN_3>(ledStrips[2], PIXELS_PER_STRIP);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA
    .onStart([]() {
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
    .onProgress([](unsigned int progress, unsigned int total) {
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

    server.begin();

    // displayMode = EEPROM.read(DISPLAY_MODE_ADDR);
}

boolean checkModeChange() {
    unsigned long now = millis();

    // check if we've been holding the button for a while
    if(!buttonHeld && buttonState == LOW && now > buttonDownTime + BUTTON_LONG_PRESS) {
        // pretend the button has actually been released
        // buttonState = HIGH;
        buttonHeld = true;

        if(brightness == 0)
            brightness = 1.0;
        else
            brightness -= 0.7;

        if(brightness <= 0)
            brightness = 0;

        return false;
    }

    int touchVal = touchRead(T5);
    bool newState = touchVal > 20;

    if(newState == buttonState || lastButtonCheck > now - BUTTON_COOLDOWN)
        return false;

    Serial.printf("Touch Sensor: %d\r\n", touchVal);

    lastButtonCheck = now;
    buttonState = newState;

    if(newState == LOW) {
        buttonDownTime = millis();
    } else {
        if(buttonHeld) {
            buttonHeld = false;
            return false;
        }
        if(brightness == 0) {
            brightness = 1.0;
            return false;
        }
        if(now - buttonDownTime < BUTTON_LONG_PRESS) {
            displayMode++;

            if (displayMode >= DIGIT_COUNT * 10)
                displayMode = 0;


            // Serial.printf("Current mode: %u\n", displayMode);

            //EEPROM.write(DISPLAY_MODE_ADDR, displayMode);
            return true;
        }

    }
    return false;
}

void loop() {

    ArduinoOTA.handle();

    WiFiClient client = server.available();   // listen for incoming clients

    if (client) {                             // if you get a client,
        Serial.println("New Client.");        // print a message out the serial port
        String currentLine = "";              // make a String to hold incoming data from the client
        while (client.connected()) {          // loop while the client's connected
            if (client.available()) {         // if there's bytes to read from the client,
                char c = client.read();       // read a byte, then
                Serial.write(c);              // print it out the serial monitor
                if (c == '\n') {              // if the byte is a newline character

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

    if (checkModeChange()) {
        byte digitIndex = displayMode / 10;
        byte digitValue = displayMode % 10;

        setDigit(digitIndex, digitValue, testColors[digitValue % 4]);
    }

}

void clearPixels(byte stripIndex, byte offset, byte count) {
    for (byte i = 0; i < count; i++) {
        ledStrips[stripIndex][i + offset] = CRGB::Black;
        // *strip[i + offset] = CRGB::Black;
    }
}

void setDigit(byte index, byte value, CRGB c) {
    byte stripIndex = index / DIGITS_PER_STRIP;
    // Serial.printf("stripIndex: %u\r\n", stripIndex);

    // digit  offsets
    // 0      0, 10
    // 1      9, 19
    // 2      1, 11
    // 3      8, 18
    // 4      2, 12
    // 5      7, 17
    // 6      3, 13
    // 7      6, 16
    // 8      4, 14
    // 9      5, 15

    byte pixelOffset = (index % DIGITS_PER_STRIP) * PIXELS_PER_DIGIT;
    clearPixels(stripIndex, pixelOffset, PIXELS_PER_DIGIT);

    byte isEven = value % 2;
    byte halfValue = value / 2;
    // even numbers count up from 0, odd numbers count down from 9
    byte pixel0 = (isEven * (9 - halfValue)) + ((1 - isEven) * halfValue) + pixelOffset;
    byte pixel1 = pixel0 + 10;

    ledStrips[stripIndex][pixel0] = c;
    ledStrips[stripIndex][pixel1] = c;

    FastLED.show();
}
