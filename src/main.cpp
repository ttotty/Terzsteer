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

Button *button = NULL;
LedIndicator *ledIndicator = NULL;
ZwiftBle *zwift = NULL;
unsigned long lastSteeringEventTime = 0;
const unsigned long reset_steering_frequency = 1000;
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
    ButtonPressEvent event = button->getButtonPress();

    if (ledIndicator != NULL)
    {
        ledIndicator->updateState(zwift->getAuthenticated(), event);
    }

    unsigned long now = millis();
    bool straightEventRequired = (now - lastSteeringEventTime > reset_steering_frequency);
    if (straightEventRequired || event.buttonPressed())
    {
        zwift->addNotifiableAngle(event);
        lastSteeringEventTime = now;
    }
}