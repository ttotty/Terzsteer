#include <Arduino.h>
#include "settings.h"

// Define SIMULATE_STEERING to turn on simulation
#define SIMULATE_STEERING
//#define SERIAL_ALL_EVENTS
#define SIMULATE_EVENT_FREQUENCY 200
#define DEFAULT_DELAY SIMULATE_EVENT_FREQUENCY / portTICK_PERIOD_MS;
#define DELAY_1_SECOND 1000 / portTICK_PERIOD_MS
#define DELAY_5_SECONDS DELAY_1_SECOND * 5
#define DELAY_10_SECONDS DELAY_1_SECOND * 10

enum direction
{
    left,
    right,
    straight,
    straight1Sec,
    straight5Sec,
    straight10Sec
};

struct ButtonPressEvent
{
    bool left = false;
    bool right = false;

    float getAngleDelta()
    {
        float delta = 0.0;
        if (left)
            delta = -BUTTON_PRESS_ANGLE;
        if (right)
            delta = BUTTON_PRESS_ANGLE;
        return delta;
    }

    void toString(char *outStr)
    {
        sprintf(outStr, "Button press: left=%u, right=%u, delta=%f", left, right, getAngleDelta());
    }
};
#ifdef SIMULATE_STEERING
const direction simulations[20] = {left, straight5Sec, 
                                   right, straight1Sec,
                                   straight, straight, straight,
                                   right, straight5Sec,
                                   right, right, left, straight1Sec,
                                   straight10Sec,
                                   right, right, right, right, right, straight5Sec};

ButtonPressEvent simulationEvent;
class ButtonSimulation
{
    static void task(void *parameters)
    {
        volatile int simulationIndex = 0;
        volatile int eventCount = sizeof(simulations);
        volatile int delay;
        for (;;)
        {
            delay = DEFAULT_DELAY;
            simulationIndex = simulationIndex % eventCount;
            direction event = simulations[simulationIndex++];
                simulationEvent.left = (event == left);
                simulationEvent.right = (event == right);

            if (event == straight1Sec)
                delay = DELAY_1_SECOND;
            else if (event == straight5Sec)
                delay = DELAY_5_SECONDS;
            else if (event == straight10Sec)
                delay = DELAY_10_SECONDS;
            vTaskDelay(delay);
        }
    }

public:
    void getCurrentEvent(ButtonPressEvent& event)
    {        
            event.left = simulationEvent.left;
            event.right = simulationEvent.right;
            simulationEvent.left = false;
            simulationEvent.right = false;
    }

    ButtonSimulation()
    {
        xTaskCreate(
            task,
            "simulation task",
            5000, //stack size
            NULL,
            0, //priority
            NULL);
    }
};
#endif

class Button
{
    ButtonSimulation *buttonSimulation = NULL;

public:
    Button()
    {
        pinMode(LEFT_BUTTON, INPUT);
        pinMode(RIGHT_BUTTON, INPUT);

#ifdef SIMULATE_STEERING
        buttonSimulation = new ButtonSimulation();
#endif
    }

    ButtonPressEvent getButtonPress()
    {
        ButtonPressEvent event;
#ifdef SIMULATE_STEERING
        buttonSimulation->getCurrentEvent(event);
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
};