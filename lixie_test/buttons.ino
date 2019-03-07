#include "driver/touch_pad.h"

// #define BUTTON_COUNT 2
#define BUTTON_SMOOTHING_FAST 0.05f     // weighting for new values when calculating an expoential moving average
#define BUTTON_SMOOTHING_FAST_INV 1f - BUTTON_SMOOTHING_FAST

#define BUTTON_SMOOTHING_SLOW 0.005f    // weighting for new values when calculating an expoential moving average
#define BUTTON_SMOOTHING_SLOW_INV 1f - BUTTON_SMOOTHING_SLOW

#define BUTTON_THRESHHOLD 0.3f   // fraction of the base value to be considered a button press

#define BUTTON_SHORT_HOLD_TIME  100 // regular press length in ms (non-zero to prevent transients from triggering button)
#define BUTTON_LONG_HOLD_TIME  600 // long press length in ms
#define BUTTON_RESET_HOLD_TIME  5000 // hard reset press length in ms

enum button_event_t {
    BUTTON_PRESS_SHORT = 0,
    BUTTON_PRESS_LONG,
    BUTTON_PRESS_RESET,
};

struct TouchButton {
    // Digital IO pin connected to the touchpad.
    // This will be driven with a pull-up resistor so the switch should
    // pull the pin to ground momentarily.  On a high -> low
    // transition the button press logic will execute.
    byte pin;

    // exponential moving average of measured value
    float filtered;

    // long-term moving average of baseline value
    // (only updated when button is not pressed)
    // this should accomodate slow changes in capacitance due to environmental factors
    float baseline;

    // timestamp when the button press started (0 means button is inactive)
    uint32_t downTime = 0;

    void updateFiltered(float newVal) {
        filtered *= BUTTON_SMOOTHING_FAST_INV;
        filtered += BUTTON_SMOOTHING_FAST * newVal;
    }

    void updateBaseline(float newVal) {
        baseline *= BUTTON_SMOOTHING_SLOW_INV;
        baseline += BUTTON_SMOOTHING_SLOW * newVal;
    }
}

typedef void (*button_handler_t) (byte, button_event_t);

template<size_t BUTTON_COUNT>
class TouchButtonManager {
    private:
        TouchButton buttons[];
        button_handler_t buttonHandler;

    public:
        TouchButtonManager(byte buttonPins[BUTTON_COUNT]) {
            for (byte i = 0; i < BUTTON_COUNT; i++) {
                TouchButton &button = this->buttons[i];
                button.pin = buttonPins[i];
                button.filtered = button.baseline = (float)touchRead(button->pin);
            }
        }

        void registerButtonHandler(button_handler_t onPress) {
            this->buttonHandler = onPress;
        }

        void checkButton(byte index) {
            TouchButton &button = this->buttons[i];
            uint16_t newVal = touchRead(button.pin);

            // 0 values come from a timing-related (?) measurement glitch
            // should only be occasional so we can just ignore it
            if (newVal == 0)
                return;

            button.updateFiltered((float)newVal);


            if (button.filtered < button.baseline[index] * BUTTON_THRESHHOLD) {
                if (button.downTime == 0) {
                    // button has just been pressed
                    button.downTime = now;
                } else {
                    uint16_t downTime = now - button.downTime;
                    if (downTime > BUTTON_RESET_HOLD_TIME) {
                        // wat
                    }


                // } else if (now - buttonDownTime > BUTTON_LONG_PRESS) {
                //     // button is being held down

                // }
                // // else button is being held down

            } else {
                // button is inactive


                button.updateBaseline(button.filtered);

                if (button.downTime != 0) {
                    // button was down long enough to count
                    if (now - button.downTime > BUTTON_SHORT_HOLD_TIME) {
                        button_event_t eventType = (button_event_t)(button.downTime > BUTTON_LONG_HOLD_TIME);
                        auto onPress = buttonHandlers[index];
                        if (buttonHandler)
                            buttonHandler(index, eventType);
                        else
                            Serial.println("Error: no button handler registered!");
                    }

                    button.downTime = 0;
                }
            }

            currentButton++;
            currentButton %= BUTTON_COUNT;
    }
}
