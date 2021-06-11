#ifndef LEDINDICATOR_H
#define LEDINDICATOR_H
#include <Arduino.h>

#define USE_LEDS

#define BEFORE_AUTH_FREQUENCY 1000 / portTICK_PERIOD_MS
#define AFTER_AUTH_FREQUENCY 5000 / portTICK_PERIOD_MS
#define TURN_FREQUENCY 100 / portTICK_PERIOD_MS
#define AUTH_BLINK_TIME 100 / portTICK_PERIOD_MS

class LedIndicator
{

public:
    void starting(bool show);

    void updateState(bool authenticated,
                     bool leftPressed,
                     bool rightPressed);

    LedIndicator();
};
#endif