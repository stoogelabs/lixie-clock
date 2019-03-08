#include "Arduino.h"
#include "FastLED.h"

#define LIXIE_LED_PIN_1         15  // Digital IO pin connected to the NeoPixels.
#define LIXIE_LED_PIN_2         5
#define LIXIE_LED_PIN_3         14
#define LIXIE_STRIP_COUNT       3
#define LIXIE_PANEL_COUNT       10  // 10 possible values
#define LIXIE_LEDS_PER_DIGIT    2 * LIXIE_PANEL_COUNT // 2 pixels per value
#define LIXIE_DIGITS_PER_STRIP  2   // each strip contains 2 digits
#define LIXIE_DIGIT_COUNT LIXIE_STRIP_COUNT * LIXIE_DIGITS_PER_STRIP
#define LIXIE_LEDS_PER_STRIP LIXIE_LEDS_PER_DIGIT * LIXIE_DIGITS_PER_STRIP
#define LIXIE_LED_COUNT LIXIE_LEDS_PER_STRIP * LIXIE_STRIP_COUNT

class LixieDisplay {
    private:
        CRGB pixels[LIXIE_LED_COUNT] = {};

        byte currentDigits[LIXIE_DIGIT_COUNT] = {};

        // updates the specified digit with the correct value and color
        void refreshDigit(byte index);

        // each digit value is illuminated by 2 pixels, with offset1 = offset0 + LIXIE_PANEL_COUNT
        // calculates the pixel offset of the first pixel for the specified value
        // and digit index (assumes the strip starts at 0)
        static byte getLedOffset(byte index, byte value);

    public:
        LixieDisplay();

        byte digits[LIXIE_DIGIT_COUNT] = {};

        CRGB colors[LIXIE_DIGIT_COUNT] = {};

        byte brightness = 255;

        void refresh();

        void setColor(CRGB *color);
};
