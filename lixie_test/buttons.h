#include "Arduino.h"
#include "driver/touch_pad.h"

// #define BUTTON_COUNT    2

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

template<size_t BUTTON_COUNT>
class TouchButtonManager {
    private:
        TouchButton buttons[BUTTON_COUNT];
        button_handler_t buttonHandler;
        uint32_t lastButtonCheck = 0;
        byte currentButton = 0;

    public:
        TouchButtonManager(const byte (&buttonPins)[BUTTON_COUNT]);

        void registerButtonHandler(button_handler_t buttonHandler);

        void poll();

    private:
        bool detectReset(uint32_t &now);

        void callButtonHandler(byte index, button_event_t event);

        void checkButton(byte index, uint32_t &now);
};

//  ########################################################################
//  ##                           Implementation                           ##
//  ########################################################################

void TouchButton::updateFiltered(float newVal) {
    filtered *= BUTTON_SMOOTHING_FAST_INV;
    filtered += BUTTON_SMOOTHING_FAST * newVal;
}

void TouchButton::updateBaseline(float newVal) {
    baseline *= BUTTON_SMOOTHING_SLOW_INV;
    baseline += BUTTON_SMOOTHING_SLOW * newVal;
}

bool TouchButton::isActive() {
    return filtered < baseline * BUTTON_THRESHHOLD;
}

void TouchButton::read() {
    float newVal = (float)touchRead(pin);
    if (filtered == 0) {
        // this is the first measurement
        filtered = newVal;
        baseline = newVal;
    } else {
        // 0 values come from a timing-related (?) measurement glitch
        // should only be occasional so we can just ignore it
        if (newVal != 0) {
            updateFiltered(newVal);

            if (newVal < baseline * BUTTON_THRESHHOLD)
                updateBaseline(newVal);
        }
    }
}

template <size_t BUTTON_COUNT>
TouchButtonManager<BUTTON_COUNT>::TouchButtonManager(const byte (&buttonPins)[BUTTON_COUNT]) {
    // initialize buttons and get initial measurements
    for (byte i = 0; i < BUTTON_COUNT; i++) {
        TouchButton &button = this->buttons[i];
        button.pin = buttonPins[i];

        delay(BUTTON_READ_DELAY);
        button.read();
    }
}

template <size_t BUTTON_COUNT>
void TouchButtonManager<BUTTON_COUNT>::registerButtonHandler(button_handler_t buttonHandler) {
    this->buttonHandler = buttonHandler;
}

template <size_t BUTTON_COUNT>
void TouchButtonManager<BUTTON_COUNT>::poll() {
    uint32_t now = millis();
    uint32_t &lastCheck = this->lastButtonCheck;
    byte &currentButton = this->currentButton;
    if (now - lastCheck < BUTTON_READ_DELAY || now < lastCheck) {
        checkButton(currentButton, now);
        lastCheck = now;
        currentButton = (currentButton + 1) % BUTTON_COUNT;
    }
}


template <size_t BUTTON_COUNT>
bool TouchButtonManager<BUTTON_COUNT>::detectReset(uint32_t &now) {
    for (byte i = 0; i < BUTTON_COUNT; i++) {
        if (now - this->buttons[i].downTime < BUTTON_RESET_HOLD_TIME)
            return false;
    }
    return true;
}

template <size_t BUTTON_COUNT>
void TouchButtonManager<BUTTON_COUNT>::callButtonHandler(byte index, button_event_t event) {
    if (this->buttonHandler)
        this->buttonHandler(index, event);
    else
        Serial.println("Error: no button handler registered!");
}

template <size_t BUTTON_COUNT>
void TouchButtonManager<BUTTON_COUNT>::checkButton(byte index, uint32_t &now) {
    TouchButton &button = this->buttons[index];
    button.read();

    if (button.isActive()) {
        if (button.downTime == 0) {
            // button has just been pressed down
            button.downTime = now;
        } else {
            uint32_t holdTime = now - button.downTime;

            if (holdTime > BUTTON_RESET_HOLD_TIME) {
                // button has been down long enough for a reset,
                // but only if all other buttons are also in this state
                if (this->detectReset(now))
                    this->callButtonHandler(index, BUTTON_PRESS_RESET);
            } else if (holdTime > BUTTON_LONG_HOLD_TIME) {
                // button has been held down long enough for a normal press,
                // but only if no other buttons are active
                bool othersActive = false;
                for (byte i = 0; i < BUTTON_COUNT; i++) {
                    othersActive |= i != index && this->buttons[i].isActive();
                }
                if (!othersActive)
                    this->callButtonHandler(index, BUTTON_PRESS_LONG);
            }
            // else, the button hasn't been held long enough to matter
        }
    } else {
        if (button.downTime != 0) {
            uint32_t holdTime = now - button.downTime;

            // we only detect short holds here.
            // long holds are detected while the button is still active
            if (holdTime > BUTTON_SHORT_HOLD_TIME)
                this->callButtonHandler(index, BUTTON_PRESS_SHORT);

            button.downTime = 0;
        }
    }
}
