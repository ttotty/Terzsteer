#include <Arduino.h>
#include "ButtonPressEvent.h"
#include "ButtonSimulation.h"
#include "ButtonPressEvents.h"
#include "Settings.h"

static QueueHandle_t simulationQueue;
static void simulationTask(void *parameters)
{
    volatile int simulationIndex = 0;
    const int eventCount = sizeof(simulations) / sizeof(simulations[0]);
    volatile int delay;
    for (;;)
    {
        delay = DEFAULT_DELAY;
        simulationIndex = simulationIndex % eventCount;
        Direction event = simulations[simulationIndex++];
        ButtonPressEvent simulationEvent;

        simulationEvent.press(event == left, event == right);
        if(!simulationEvent.isStraight())
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

void ButtonSimulation::getBufferedEvents(ButtonPressEvents &events)
{
    int count = 0;
    ButtonPressEvent nextEvent;
    int maxEvents = events.getBufferSize();
    while (count < maxEvents && xQueueReceive(simulationQueue, &nextEvent, 0) == pdPASS)
    {
        events.buffer[count++] = nextEvent;
    }
}

ButtonSimulation::ButtonSimulation()
{
    simulationQueue = xQueueCreate(MAX_BUTTON_EVENTS, sizeof(ButtonPressEvent));
    xTaskCreate(simulationTask, "simulation task", 2000, NULL, 0, NULL);
}
