#include <Arduino.h>
#include "Settings.h"
#include "ButtonPressEvent.h"
#include "Button.h"

Button::Button()
{
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);

#ifdef SIMULATE_STEERING
    buttonSimulation = new ButtonSimulation();
#endif
}

ButtonPressEvent Button::getButtonPress()
{
    ButtonPressEvent event;
#ifdef SIMULATE_STEERING
    buttonSimulation->receiveCurrentEvent(event);
#else
    event.left = digitalRead(LEFT_BUTTON);
    event.right = digitalRead(RIGHT_BUTTON);
#endif

#ifdef SERIAL_ALL_EVENTS
    float deltaAngle = event.getAngleDelta();
    static float previousDelta = 0.0;
    if (previousDelta != deltaAngle)
    {
        char eventText[80];
        event.toString(eventText);
        serialPrintln(eventText);
    }
    previousDelta = deltaAngle;
#endif

    return event;
}
