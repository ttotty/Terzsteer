#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "Settings.h"

bool ButtonPressEvent::buttonPressed()
{
    return ((left == true || right == true) && created > 0);
}

float ButtonPressEvent::getAngleDelta()
{
    float delta = 0.0;
    if (left)
        delta += -BUTTON_PRESS_ANGLE;
    if (right)
        delta += BUTTON_PRESS_ANGLE;
    return delta;
}

bool ButtonPressEvent::isStraight()
{
    return getAngleDelta() == 0.0;
}

void ButtonPressEvent::toString(char *outStr)
{
    sprintf(outStr, "Button press: left=%u, right=%u, delta=%f, created=%lu", left, right, getAngleDelta(), created);
}

bool ButtonPressEvent::isExpired(unsigned long millis)
{
    return millis - created >= BUTTON_EVENT_EXPIRE;
}

bool ButtonPressEvent::getLeft()
{
    return left;
}

bool ButtonPressEvent::getRight()
{
    return right;
}

void ButtonPressEvent::press(bool leftPress, bool rightPress)
{
    left = leftPress;
    right = rightPress;
    created = millis();
}
