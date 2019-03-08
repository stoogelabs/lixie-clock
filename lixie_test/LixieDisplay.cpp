#include "LixieDisplay.h"

LixieDisplay::LixieDisplay() {
    Serial.println("Initializing FastLED neopixel strips...");
    FastLED.addLeds<NEOPIXEL, LIXIE_LED_PIN_1>(this->pixels, 0 * LIXIE_LEDS_PER_STRIP, LIXIE_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, LIXIE_LED_PIN_2>(this->pixels, 1 * LIXIE_LEDS_PER_STRIP, LIXIE_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, LIXIE_LED_PIN_3>(this->pixels, 2 * LIXIE_LEDS_PER_STRIP, LIXIE_LEDS_PER_STRIP);
}

byte LixieDisplay::getLedOffset(byte index, byte value) {
    if (value > 9) {
        Serial.printf("Display value out of bounds: %d\r\n", value);
        value = 9;
    }

    byte digitOffset = index * LIXIE_LEDS_PER_DIGIT;

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

    byte isOdd = value % 2;
    byte halfValue = value / 2;
    // even numbers count up from 5, odd numbers count down from 4
    return (isOdd * (4 - halfValue)) + ((1 - isOdd) * (5 + halfValue)) + digitOffset;
}

void LixieDisplay::refreshDigit(byte index) {
    byte &currentValue = this->currentDigits[index];
    byte &newValue = this->digits[index];

    // clear the previous digit if necessary
    if (currentValue != newValue) {
        byte oldOffset0 = LixieDisplay::getLedOffset(index, currentValue);
        byte oldOffset1 = oldOffset0 + LIXIE_PANEL_COUNT;
        this->pixels[oldOffset0] = 0;
        this->pixels[oldOffset1] = 0;
        currentValue = newValue;
    }

    byte offset0 = LixieDisplay::getLedOffset(index, newValue);
    byte offset1 = offset0 + LIXIE_PANEL_COUNT;

    CRGB color = this->colors[index];
    color.nscale8(this->brightness);

    this->pixels[offset0] = color;
    this->pixels[offset1] = color;
}


void LixieDisplay::refresh() {
    for (byte i = 0; i < LIXIE_DIGIT_COUNT; i++) {
        this->refreshDigit(i);
    }
    FastLED.show();
}

void LixieDisplay::setColor(CRGB *color) {
    for (byte i = 0; i < LIXIE_DIGIT_COUNT; i++) {
        this->colors[i] = *color;
    }
}
