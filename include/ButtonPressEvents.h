#ifndef BUTTONPRESSEVENTS_H
#define BUTTONPRESSEVENTS_H
#include <Arduino.h>
#include "Settings.h"
#include "ButtonPressEvent.h"
struct ButtonPressEvents
{
public:
    ButtonPressEvent buffer[MAX_BUTTON_EVENTS];
    
    int getBufferSize();
};
#endif