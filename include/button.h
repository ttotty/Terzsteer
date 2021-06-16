#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "ButtonSimulation.h"
#include "Settings.h"

class Button
{
private:
    ButtonSimulation *buttonSimulation = NULL;

public:
    Button();
    ButtonPressEvents getButtonEvents();
};
#endif