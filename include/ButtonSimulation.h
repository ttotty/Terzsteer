#ifndef BUTTONSIMULATION_H
#define BUTTONSIMULATION_H
#include <Arduino.h>
#include "Settings.h"
#include "ButtonPressEvent.h"

#define SIMULATE_EVENT_FREQUENCY 175
#define DEFAULT_DELAY SIMULATE_EVENT_FREQUENCY / portTICK_PERIOD_MS
#define DELAY_1_SECOND 1000 / portTICK_PERIOD_MS
#define DELAY_5_SECONDS DELAY_1_SECOND * 5
#define DELAY_10_SECONDS DELAY_1_SECOND * 10

enum Direction
{
    left,
    right,
    straight,
    straight1Sec,
    straight5Sec,
    straight10Sec
};

const Direction simulations[24] = {left, straight5Sec,
                                   right, straight5Sec,
                                   left, left, left, right, left, right, straight5Sec,
                                   right, straight5Sec,
                                   right, right, left, straight10Sec,
                                   straight10Sec,
                                   right, right, right, right, right, straight5Sec};

class ButtonSimulation
{
public:
    static void task(void *parameters);
    void receiveCurrentEvent(ButtonPressEvent &event);
    ButtonSimulation();
};
#endif