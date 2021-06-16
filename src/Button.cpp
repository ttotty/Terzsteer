#include <Arduino.h>
#include "Settings.h"
#include "ButtonPressEvents.h"
#include "Button.h"

Button::Button()
{
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);

#ifdef SIMULATE_STEERING
    buttonSimulation = new ButtonSimulation();
#endif
}

ButtonPressEvents Button::getButtonEvents()
{
    ButtonPressEvents events;
#ifdef SIMULATE_STEERING
    buttonSimulation->getBufferedEvents(events);
#else
    //TODO: get button presses entered through ISR
    
    //event.left = digitalRead(LEFT_BUTTON);
    //event.right = digitalRead(RIGHT_BUTTON);
#endif

    return events;
}
