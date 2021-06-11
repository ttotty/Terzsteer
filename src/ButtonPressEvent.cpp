#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "Settings.h"

bool ButtonPressEvent::buttonPressed()
{
    return (left == true || right == true || useOverrideAngle == true);
}

float ButtonPressEvent::getAngleDelta()
{
    float delta = 0.0;
    if (useOverrideAngle)
    {
        delta = overrideAngle;
    }
    else
    {
        if (left)
            delta = -BUTTON_PRESS_ANGLE;
        if (right)
            delta = BUTTON_PRESS_ANGLE;
    }
    return delta;
}

void ButtonPressEvent::toString(char *outStr)
{
    sprintf(outStr, "Button press: left=%u, right=%u, delta=%f, created=%lu", left, right, getAngleDelta(), created);
}
