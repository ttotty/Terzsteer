#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "ButtonSimulation.h"

class Button
{
private:
    ButtonSimulation *buttonSimulation = NULL;

public:
    Button();
    ButtonPressEvent getButtonPress();
};
#endif