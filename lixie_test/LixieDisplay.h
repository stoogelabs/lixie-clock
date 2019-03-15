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

#define LIXIE_BRIGHTNESS_MODE_COUNT 3

#define LIXIE_BLANK_DIGIT       255

enum brightness_mode_t {
    LIXIE_BRIGHT = 0,
    LIXIE_DIM = 1,
    LIXIE_OFF = 2,
};

static const byte brightnessValues[] = {
    255,
    80,
    0,
};

static const byte panelBrightnessFactor[] = {
    200,
    200,
    200,
    200,
    200,
    200,
    200,
    200,
    255,
    255,
};

class LixieDisplay {
    private:
        CRGB pixels[LIXIE_LED_COUNT] = {};

        byte currentDigits[LIXIE_DIGIT_COUNT] = {};

        brightness_mode_t brightnessMode = LIXIE_BRIGHT;

        void clearDigit(byte index);

        // updates the specified digit with the correct value and color
        void refreshDigit(byte index);

        // each digit value is illuminated by 2 pixels, with offset1 = offset0 + LIXIE_PANEL_COUNT
        // calculates the pixel offset of the first pixel for the specified value
        // and digit index (assumes the strip starts at 0)
        static byte getLedOffset(byte index, byte value);

        // compensates for panels in the back appearing dimmer
        static byte getPanelBrightness(byte value);

    public:
        LixieDisplay();

        byte digits[LIXIE_DIGIT_COUNT] = {};

        CRGB colors[LIXIE_DIGIT_COUNT] = {};

        byte brightness = 255;

        void refresh();

        void setColor(CRGB *color);

        brightness_mode_t getBrightnessMode();

        void setBrightnessMode(brightness_mode_t mode);

        void nextBrightnessMode();
};
