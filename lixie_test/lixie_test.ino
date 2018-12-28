// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include <NeoPixelBus.h>
//#include <EEPROM.h>

//EEPROMClass  MODE("eeprom0", 0x1000);

#define BUTTON_PIN   T5    // Digital IO pin connected to the button.  This will be
#define TRIGGER_PIN  13                             // driven with a pull-up resistor so the switch should
                                                    // pull the pin to ground momentarily.  On a high -> low
                                                    // transition the button press logic will execute.

#define PIXEL_PIN    15   // Digital IO pin connected to the NeoPixels.
#define LLC   11

#define PIXEL_COUNT 20

#define BUTTON_COOLDOWN  100 // minimum time in miliseconds between button presses (to reduce noise)
#define BUTTON_LONG_PRESS  600 // miliseconds between button down and up to count as a long press

#define TWO_PI 6.28318530718

#define DISPLAY_MODE_ADDR  0

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
NeoPixelBus<NeoGrbFeature, NeoWs2813Method> strip(PIXEL_COUNT, PIXEL_PIN);

uint8_t displayMode = 0;
bool buttonState = HIGH;   // LOW = pressed
bool buttonHeld = false;
unsigned long lastButtonCheck = 0;
unsigned long buttonDownTime = 0;
float brightness = 0.0;

#include <WiFi.h>

const char* ssid     = "4wire";
const char* password = "laserlaser";

WiFiServer server(80);

int value = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(PIXEL_PIN, OUTPUT);
    pinMode(LLC, OUTPUT);
    digitalWrite(LLC, LOW);

    strip.Begin();
    
    strip.Show(); // Initialize all pixels to 'off'
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

    bool newState;
    if (touchRead(T5) > 20) newState = HIGH;
    if (touchRead(T5) <= 20) newState = LOW;
    if(newState == buttonState || lastButtonCheck > now - BUTTON_COOLDOWN)
        return false;

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
            if (displayMode > 11)
                displayMode = 0;

            //EEPROM.write(DISPLAY_MODE_ADDR, displayMode);
            return true;
        }

    }
    return false;
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

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
  Serial.println(touchRead(T5));  // get value using T0
  RgbColor white(255 * brightness, 255 * brightness, 255 * brightness);
  RgbColor red(255 * brightness, 0 * brightness, 0 * brightness);
  RgbColor green(30 * brightness, 255 * brightness, 0 * brightness);
  RgbColor blue(0, 30 * brightness, 255 * brightness);

  
  
    switch(displayMode) {
      
        case 0: colorWipe0(red, 50);  // green
        digitalWrite(LLC, LOW);
                        break;
        case 1: colorWipe1(green, 50);  // blue
        digitalWrite(LLC, LOW);
                        break;;
        case 2: colorWipe9(blue, 50);  // green
        digitalWrite(LLC, LOW);
                        break;
        case 3: colorWipe2(white, 50);  // blue
        digitalWrite(LLC, LOW);
                        break;;
        case 4: colorWipe8(blue, 50);  // green
        digitalWrite(LLC, LOW);
                        break;
        case 5: colorWipe3(green, 50);  // blue
        digitalWrite(LLC, LOW);
                        break;;
        case 6: colorWipe7(red, 50);  // green
        digitalWrite(LLC, LOW);
                        break;
        case 7: colorWipe4(white, 50);  // blue
        digitalWrite(LLC, LOW);
                        break;;
        case 8: colorWipe6(green, 50);  // green
        digitalWrite(LLC, LOW);
                        break;
        case 9: colorWipe5(blue, 50);  // blue
        digitalWrite(LLC, LOW);
                        break;;
                        
 
        default:
            displayMode = 0;
           // EEPROM.write(DISPLAY_MODE_ADDR, 0);
    }
}

// Fill the dots one after the other with a color
void colorWipe0(RgbColor c, uint8_t wait) {
  if(checkModeChange())
    return;
  
  for(int j=0; j<20; j++) {
    strip.SetPixelColor(j, 0);
  }
  strip.SetPixelColor(0, c);
  strip.SetPixelColor(10, c);
  
  strip.Show();
}

// Fill the dots one after the other with a color
void colorWipe1(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(9, c);
        strip.SetPixelColor(19, c);
        
        strip.Show();
        
    
}
// Fill the dots one after the other with a color
void colorWipe2(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(8, c);
        strip.SetPixelColor(18, c);
        
        strip.Show();
        
        
    
}
// Fill the dots one after the other with a color
void colorWipe3(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(7, c);
        strip.SetPixelColor(17, c);
        
        strip.Show();
        
    
}

// Fill the dots one after the other with a color
void colorWipe4(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(6, c);
        strip.SetPixelColor(16, c);
        
        strip.Show();
        
    
}
// Fill the dots one after the other with a color
void colorWipe5(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(5, c);
        strip.SetPixelColor(15, c);
        
        strip.Show();
        
    
}

// Fill the dots one after the other with a color
void colorWipe6(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(4, c);
        strip.SetPixelColor(14, c);
        
        strip.Show();
        
    
}

// Fill the dots one after the other with a color
void colorWipe7(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(3, c);
        strip.SetPixelColor(13, c);
        
        strip.Show();
        
    
}
// Fill the dots one after the other with a color
void colorWipe8(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(2, c);
        strip.SetPixelColor(12, c);
        
        strip.Show();
        
    
}

// Fill the dots one after the other with a color
void colorWipe9(RgbColor c, uint8_t wait) {
    
        if(checkModeChange())
            return;
            for(int j=0; j<20; j++) {
            strip.SetPixelColor(j, 0);
    }
        strip.SetPixelColor(1, c);
        strip.SetPixelColor(11, c);
        
        strip.Show();
        
    
}

//void rainbow(uint8_t wait) {
//    uint16_t i, j;
//
//    for(j=0; j<256; j++) {
//        if(checkModeChange())
//            return;
//
//            strip.SetPixelColor(0, 0);
//            strip.SetPixelColor(10, 0);
//
//            strip.SetPixelColor(9, Wheel((j) & 255));
//            strip.SetPixelColor(19, Wheel((j) & 255));
//        
//        strip.Show();
//        delay(wait * 8);
//    }
//}

//void rainboww(uint8_t wait) {
//    uint16_t i, j;
//
//    for(j=0; j<256; j++) {
//        if(checkModeChange())
//            return;
//
//            strip.SetPixelColor(9, 0);
//            strip.SetPixelColor(19, 0);
//            strip.SetPixelColor(0, Wheel((j) & 255));
//            strip.SetPixelColor(10, Wheel((j) & 255));
//        
//        strip.Show();
//        delay(wait * 8);
//    }
//}

// Slightly different, this makes the rainbow equally distributed throughout
//void rainbowCycle(uint8_t wait) {
//    uint16_t i, j;
//
//    for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
//        if(checkModeChange())
//            return;
//
//        for(i=0; i< PIXEL_COUNT; i++) {
//            strip.SetPixelColor(i, Wheel(((i * 256 / PIXEL_COUNT) + j) & 255));
//        }
//        
//        strip.Show();
//        delay(wait);
//    }
//}
//
//void shimmerRainbow(uint8_t wait) {
//    float t = 0;
//    float deltaT = 0.0002 * (float)wait;
//    int hue = 0;
//    int pixelCount = PIXEL_COUNT;
//
//    int colorScale = wait * 0.1;
//    float spacialScale = TWO_PI / (float)pixelCount;
//
//    while(!checkModeChange()) {
//        hue = (hue + colorScale) % 255;
//        t += deltaT;
//        if(t > TWO_PI)
//            t = 0;
//
//        for (int i = 0; i < pixelCount; i++) {
//            float x = (float)i * spacialScale;
//
//            float intensity =   sin(t + x) * 0.2 +
//                                sin((-t + x) * 2.0) * 0.3 +
//                                sin((t + x) * 4.0) * 0.5;
//
//            uint32_t c = hsb2rgb((int)(hue + 10 * x) % 255, 255, round((128 + 128 * intensity) * brightness));
//
//            strip.SetPixelColor(i, c);
//        }
//        
//        strip.Show();
//
//        delay(wait);
//    }
//
//}
//
//void shimmerWhiteAndColor(uint8_t hue, uint8_t wait) {
//    float t = 0;
//    float deltaT = 0.0005 * wait;
//    int pixelCount = PIXEL_COUNT;
//
//    float spacialScale = TWO_PI / (float)pixelCount;
//
//    while(!checkModeChange()) {
//        t += deltaT;
//        if(t > TWO_PI)
//            t -= TWO_PI;
//
//        for (int i = 0; i < pixelCount; i++) {
//            float x = (float)i * spacialScale;
//
//            float intensity =  sin(t + x) * 0.2 +
//                                sin((-t + x) * 2.0) * 0.3 +
//                                sin((t + x) * 4.0) * 0.5;
//
//            // int saturation = 128 + floor(128.0 * sin((t + x) * 1));
//            int saturation = round(128 + 128 * intensity);
//
//            uint32_t c = hsb2rgb(hue, saturation, round((128 + 128 * intensity) * brightness));
//
//
//            strip.SetPixelColor(i, c);
//        }
//        
//        strip.Show();
//
//        delay(wait);
//    }
//}
//
//void pulseTwoColor(uint8_t hue1, uint8_t hue2, uint8_t wait) {
//    float t = 0;
//    float deltaT = 0.0003 * wait;
//    int pixelCount = PIXEL_COUNT;
//    int hueDelta = round((hue2 - hue1) / (float)(pixelCount - 1));
//    float dx = 1.0 / pixelCount;
//
//    while(!checkModeChange()) {
//        t += deltaT;
//        if(t > 1)
//            t -= 1;
//
//        int hue = hue1;
//        float x = 0;
//
//        for (int i = 0; i < pixelCount; i++) {
//            float intensity = (1 - cos(TWO_PI * (x - t))) * 0.4 + 0.2;
//
//            uint32_t c = hsb2rgb(int(hue), 255, round(255 * intensity * brightness));
//            strip.SetPixelColor(i, c);
//
//            hue += hueDelta;
//            x += dx;
//        }
//        strip.Show();
//
//        delay(wait);
//    }
//}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
//uint32_t Wheel(byte WheelPos) {
//    WheelPos = 255 - WheelPos;
//    RgbColor colornew;
//    if(WheelPos < 85) {
//        return colornew((255 - WheelPos * 3) * brightness, 0, WheelPos * 3 * brightness);
//    }
//    if(WheelPos < 170) {
//        WheelPos -= 85;
//        return colornew(0, WheelPos * 3 * brightness, (255 - WheelPos * 3) * brightness);
//    }
//    WheelPos -= 170;
//    return colornew(WheelPos * 3 * brightness, (255 - WheelPos * 3) * brightness, 0);
//}
//
//uint32_t hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright) {
//    uint16_t r, g, b;
//    uint8_t index_mod;
//    uint8_t inverse_sat = (sat ^ 255);
//
//    index *= 3;
//
//    index = index % 768;
//    index_mod = index % 256;
//
//    if (index < 256) {
//        r = index_mod ^ 255;
//        g = index_mod;
//        b = 0;
//    } else if (index < 512) {
//        r = 0;
//        g = index_mod ^ 255;
//        b = index_mod;
//    } else if (index < 768) {
//        r = index_mod;
//        g = 0;
//        b = index_mod ^ 255;
//    } else {
//        r = 0;
//        g = 0;
//        b = 0;
//    }
//
//    r = ((r * sat) / 255) + inverse_sat;
//    g = ((g * sat) / 255) + inverse_sat;
//    b = ((b * sat) / 255) + inverse_sat;
//
//    r = (r * bright) / 255;
//    g = (g * bright) / 255;
//    b = (b * bright) / 255;
//RgbColor colornew(r, g, b);
//    return colornew;
//}




