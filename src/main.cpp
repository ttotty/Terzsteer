/*
 * Using some ideas and code from:
 * https://github.com/gertvantslot/zwift-steering-esp32
 * and some from here too:
 * https://github.com/stefaandesmet2003/ESP32Sterzo
 */

#include <Arduino.h>
#include "Settings.h"
#include "ButtonPressEvent.h"
#include "Button.h"
#include "LedIndicator.h"
#include "ZwiftBle.h"
#include "ButtonPressEvents.h"

Button *button = NULL;
LedIndicator *ledIndicator = NULL;
ZwiftBle *zwift = NULL;
unsigned long lastSteeringEventTime = 0;
void setup()
{
    serialBegin(115200);
    serialPrintln("Starting...");

#ifdef USE_LEDS
    serialPrintln("Setup LEDs...");
    ledIndicator = new LedIndicator();
    ledIndicator->starting(true);
#endif
    serialPrintln("Setup buttons...");
    button = new Button();

    serialPrintln("Setup Zwift BLE...");
    zwift = new ZwiftBle();
    zwift->setupBle();

    ledIndicator->starting(false);
}

void loop()
{
    ButtonPressEvents buffer = button->getButtonEvents();
    bool isButtonPressed = false;
    int index = 0;
    int bufferSize = buffer.getBufferSize();
    while (index < bufferSize)
    {
        ButtonPressEvent event = buffer.buffer[index];
        index++;
        //events are in order so no need to process after we hit one that is not a press event
        if (!event.buttonPressed())
        {
            break;
        }
        isButtonPressed |= zwift->addNotifiableAngle(event);
    }

    if (isButtonPressed)
    {
        lastSteeringEventTime = millis();
        //only indicate the most recent
        if (ledIndicator != NULL)
            ledIndicator->updateState(zwift->getAuthenticated(), buffer.buffer[index - 1]);
    }

    //add 'still here & going straight' steering event if required
    if (isButtonPressed == false)
    {
        unsigned long now = millis();
        bool straightEventRequired = (now - lastSteeringEventTime > STILL_STEERING_FREQUENCY);
        if (straightEventRequired)
        {
            //serialPrintln("straight");
            ButtonPressEvent stillHere;
            stillHere.press(true, true);
            zwift->addNotifiableAngle(stillHere);
            lastSteeringEventTime = now;
        }
    }
}