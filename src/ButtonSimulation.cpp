#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "ButtonSimulation.h"
#include "Settings.h"

#define SIMULATION_QUEUE_SIZE 10

static QueueHandle_t simulationQueue;
static void simulationTask(void *parameters)
{
    volatile int simulationIndex = 0;
    volatile int eventCount = sizeof(simulations);
    volatile int delay;
    for (;;)
    {
        delay = DEFAULT_DELAY;
        simulationIndex = simulationIndex % eventCount;
        Direction event = simulations[simulationIndex++];
        ButtonPressEvent simulationEvent;
        simulationEvent.left = (event == left);
        simulationEvent.right = (event == right);

        xQueueSendToBack(simulationQueue, &simulationEvent, 5);

        if (event == straight1Sec)
            delay = DELAY_1_SECOND;
        else if (event == straight5Sec)
            delay = DELAY_5_SECONDS;
        else if (event == straight10Sec)
            delay = DELAY_10_SECONDS;
        vTaskDelay(delay);
    }
}

void ButtonSimulation::receiveCurrentEvent(ButtonPressEvent &event)
{
    ButtonPressEvent nextEvent;
    xQueueReceive(simulationQueue, &nextEvent, 5);
    //default zeroed event can be used if there is none.
    event.left = nextEvent.left;
    event.right = nextEvent.right;
}

ButtonSimulation::ButtonSimulation()
{
    simulationQueue = xQueueCreate(SIMULATION_QUEUE_SIZE, sizeof(ButtonPressEvent));
    xTaskCreate(simulationTask, "simulation task", 2000, NULL, 0, NULL);
}
