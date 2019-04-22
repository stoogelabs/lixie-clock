#include "TouchButtonManager.h"
#include "DebugServer.h"

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

            if (newVal > baseline * BUTTON_THRESHHOLD)
                updateBaseline(newVal);
        }
    }
}

const byte buttonPins[] = {BUTTON_PINS};

TouchButtonManager::TouchButtonManager() {
    // initialize buttons and get initial measurements
    for (byte i = 0; i < BUTTON_COUNT; i++) {
        TouchButton &button = this->buttons[i];
        button.pin = buttonPins[i];

        delay(BUTTON_READ_DELAY);
        button.read();
    }
}

void TouchButtonManager::registerButtonHandler(button_handler_t buttonHandler) {
    this->buttonHandler = buttonHandler;
}

void TouchButtonManager::poll() {
    uint32_t now = millis();
    uint32_t &lastCheck = this->lastButtonCheck;
    byte &currentButton = this->currentButton;
    if (now - lastCheck >= BUTTON_READ_DELAY || now < lastCheck) {
        checkButton(currentButton, now);

        if (currentButton == 0) {
            TouchButton &button = this->buttons[currentButton];

            // DebugServer::print("Fuckin button doesn't work right...\r\n");
            DebugServer::printf(
                "Read button %d: filtered = %.2f, baseline = %.2f\r\n",
                currentButton,
                button.filtered,
                button.baseline
            );
        }

        lastCheck = now;
        currentButton = (currentButton + 1) % BUTTON_COUNT;
    }
}


bool TouchButtonManager::detectReset(uint32_t &now) {
    for (byte i = 0; i < BUTTON_COUNT; i++) {
        if (now - this->buttons[i].downTime < BUTTON_RESET_HOLD_TIME)
            return false;
    }
    return true;
}

void TouchButtonManager::callButtonHandler(byte index, button_event_t event) {
    Serial.printf("Button %d: firing event %d\r\n", index, event);

    if (this->buttonHandler)
        this->buttonHandler(index, event);
    else
        Serial.println("Error: no button handler registered!");
}

void TouchButtonManager::checkButton(byte index, uint32_t &now) {
    TouchButton &button = this->buttons[index];
    button.read();

    if (button.isActive()) {
        if (button.downTime == 0) {
            // button has just been pressed down
            button.downTime = now;
        } else if (!button.cooldown) {
            uint32_t holdTime = now - button.downTime;

            if (holdTime >= BUTTON_RESET_HOLD_TIME) {
                // button has been down long enough for a reset,
                // but only if all other buttons are also in this state
                if (this->detectReset(now)) {
                    button.cooldown = true;
                    this->callButtonHandler(index, BUTTON_PRESS_RESET);
                }
            } else if (holdTime >= BUTTON_LONG_HOLD_TIME) {
                // button has been held down long enough for a normal press,
                // but only if no other buttons are active
                bool othersActive = false;
                for (byte i = 0; i < BUTTON_COUNT; i++) {
                    othersActive |= i != index && this->buttons[i].isActive();
                }
                if (!othersActive) {
                    button.cooldown = true;
                    this->callButtonHandler(index, BUTTON_PRESS_LONG);
                }
            }
            // else, the button hasn't been held long enough to matter
        }
    } else {
        if (button.downTime != 0) {
            uint32_t holdTime = now - button.downTime;

            // we only detect short holds here.
            // long holds are detected while the button is still active
            if (holdTime >= BUTTON_SHORT_HOLD_TIME && !button.cooldown)
                this->callButtonHandler(index, BUTTON_PRESS_SHORT);

            button.downTime = 0;
            button.cooldown = false;
        }
    }
}
