#ifndef _TouchButtonManager_h
#define _TouchButtonManager_h

#include "Arduino.h"

#define BUTTON_COUNT    2
#define BUTTON_PINS     T5, T4

#define BUTTON_SMOOTHING_FAST       0.05     // weighting for new values when calculating an expoential moving average
#define BUTTON_SMOOTHING_FAST_INV   1 - BUTTON_SMOOTHING_FAST

#define BUTTON_SMOOTHING_SLOW       0.005    // weighting for new values when calculating an expoential moving average
#define BUTTON_SMOOTHING_SLOW_INV   1 - BUTTON_SMOOTHING_SLOW

#define BUTTON_THRESHHOLD       0.3   // fraction of the base value to be considered a button press

#define BUTTON_SHORT_HOLD_TIME  100 // regular press length in ms (non-zero to prevent transients from triggering button)
#define BUTTON_LONG_HOLD_TIME   600 // long press length in ms
#define BUTTON_RESET_HOLD_TIME  5000 // hard reset press length in ms

#define BUTTON_READ_DELAY       20 // wait20ms between readings to reduce chance of measurement errors

enum button_event_t {
    BUTTON_PRESS_SHORT = 0,
    BUTTON_PRESS_LONG,
    BUTTON_PRESS_RESET,
};

enum button_state_t {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_ACTIVE = 1,
    BUTTON_STATE_SHORT_HOLD = 2,
    BUTTON_STATE_LONG_HOLD = 3,
    BUTTON_STATE_RESET = 4,
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

    // update moving average with a new data point
    void updateFiltered(float newVal);

    // update baseline moving average with a new data point
    void updateBaseline(float newVal);

    // determine if measured value is low enough to count as being pressed
    bool isActive();

    // take a measurement and update filtered and baseline values
    void read();
};

typedef void (*button_handler_t) (byte, button_event_t);

class TouchButtonManager {
    private:
        TouchButton buttons[BUTTON_COUNT];
        button_handler_t buttonHandler;
        uint32_t lastButtonCheck = 0;
        byte currentButton = 0;

    public:
        TouchButtonManager();

        void registerButtonHandler(button_handler_t buttonHandler);

        void poll();

    private:
        bool detectReset(uint32_t &now);

        void callButtonHandler(byte index, button_event_t event);

        void checkButton(byte index, uint32_t &now);
};

#endif
