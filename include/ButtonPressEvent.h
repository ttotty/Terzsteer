#ifndef BUTTONPRESSEVENT_H
#define BUTTONPRESSEVENT_H
#include <Arduino.h>

struct ButtonPressEvent
{
public:
    bool left = false;
    bool right = false;
    unsigned long created = millis();
    float overrideAngle = 0.0;
    bool useOverrideAngle = false;

    bool buttonPressed();
    float getAngleDelta();
    void toString(char *outStr);
};
#endif