#ifndef LEDINDICATOR_H
#define LEDINDICATOR_H
#include <Arduino.h>
#include "ButtonPressEvent.h"

#define USE_LEDS

#define BEFORE_AUTH_FREQUENCY 1000
#define AFTER_AUTH_FREQUENCY 5000
#define TURN_FREQUENCY 100
#define AUTH_BLINK_TIME 100

class LedIndicator
{

public:
    void starting(bool show);
    void updateState(bool authenticated, ButtonPressEvent buttonPress);
    LedIndicator();
};
#endif