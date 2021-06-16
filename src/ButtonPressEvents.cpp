#include <Arduino.h>
#include "ButtonPressEvents.h"
#include "Settings.h"

int ButtonPressEvents::getBufferSize()
{
    return MAX_BUTTON_EVENTS;
}