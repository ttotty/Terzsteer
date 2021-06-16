#ifndef BUTTONPRESSEVENT_H
#define BUTTONPRESSEVENT_H
#include <Arduino.h>
#include "Settings.h"

struct ButtonPressEvent
{
private:
    unsigned long created = 0;
    bool left = false;
    bool right = false;
public:
    bool buttonPressed();
    float getAngleDelta();
    bool getLeft();
    bool getRight();
    void toString(char *outStr);
    bool isExpired(unsigned long millis);
    void press(bool leftPress, bool rightPress);
    bool isStraight();
};

#endif