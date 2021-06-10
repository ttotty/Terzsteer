/*
 * Using some ideas and code from:
 * https://github.com/gertvantslot/zwift-steering-esp32
 * and some from here too:
 * https://github.com/stefaandesmet2003/ESP32Sterzo
 */ 

#include <Arduino.h>
#include "settings.h"
#include "button.h"
#include "ledIndicator.h"
#include "zwiftBle.h"

#define RESET_STEERING_FREQUENCY 1000
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

    ledIndicator->starting(false);
}

void loop()
{
    ButtonPressEvent event = button->getButtonPress();

    if (ledIndicator != NULL)
    {
        ledIndicator->updateState(authenticated, event.left, event.right);
    }

    if (event.buttonPressed())
    {
        float angle = event.getAngleDelta();
        zwift->addNotifiableAngle(angle);
        lastSteeringEventTime = millis();
    }
    else if (millis() - lastSteeringEventTime >= RESET_STEERING_FREQUENCY)
    {
        float zeroAngle = 0.0;
        zwift->addNotifiableAngle(zeroAngle);
        lastSteeringEventTime = millis();
    }
}