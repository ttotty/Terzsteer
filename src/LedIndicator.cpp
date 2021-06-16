#include <Arduino.h>
#include "Settings.h"
#include "LedIndicator.h"
#include "ButtonPressEvent.h"

static TaskHandle_t beforeAuthenticationHandle = NULL;
static TaskHandle_t afterAuthenticationHandle = NULL;
static TaskHandle_t leftTurnHandle = NULL;
static TaskHandle_t rightTurnHandle = NULL;

static void beforeAuthenticationTask(void *parameters)
{
    TickType_t lastWakeTime;
    uint8_t pin = (uint32_t)parameters;

    for (;;)
    {
        vTaskSuspend(afterAuthenticationHandle);

        digitalWrite(pin, LED_ON);
        lastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&lastWakeTime, AUTH_BLINK_TIME);
        digitalWrite(pin, LED_OFF);
        lastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&lastWakeTime, BEFORE_AUTH_FREQUENCY / portTICK_PERIOD_MS);
    }
}
static void afterAuthenticationTask(void *parameters)
{
    TickType_t lastWakeTime;
    uint8_t pin = (uint32_t)parameters;

    for (;;)
    {
        vTaskSuspend(beforeAuthenticationHandle);

        digitalWrite(pin, LED_ON);
        lastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&lastWakeTime, AUTH_BLINK_TIME);
        digitalWrite(pin, LED_OFF);
        lastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&lastWakeTime, AFTER_AUTH_FREQUENCY / portTICK_PERIOD_MS);
    }
}

static void turnTask(void *parameters)
{
    TickType_t lastWakeTime;
    uint8_t pin = (uint32_t)parameters;

    for (;;)
    {
        for (int i = 0; i < 2; i++)
        {
            digitalWrite(pin, LED_ON);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY / portTICK_PERIOD_MS);
            digitalWrite(pin, LED_OFF);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY / portTICK_PERIOD_MS);
        }
        vTaskSuspend(NULL);
    }
}

void LedIndicator::starting(bool show)
{
    uint8_t on = show ? LED_ON : LED_OFF;
    digitalWrite(LED_CONNECTION, on);
    digitalWrite(LED_LEFT, on);
    digitalWrite(LED_RIGHT, on);
}

void LedIndicator::updateState(bool authenticated, ButtonPressEvent buttonPress)
{
    if (authenticated)
        vTaskResume(afterAuthenticationHandle);
    else
        vTaskResume(beforeAuthenticationHandle);

    if (buttonPress.getLeft())
        vTaskResume(leftTurnHandle);
    if (buttonPress.getRight())
        vTaskResume(rightTurnHandle);
}

LedIndicator::LedIndicator()
{
    pinMode(LED_CONNECTION, OUTPUT);
    pinMode(LED_LEFT, OUTPUT);
    pinMode(LED_RIGHT, OUTPUT);

    xTaskCreate(beforeAuthenticationTask,
                "before auth task", 2000, (void *)LED_CONNECTION, 1, &beforeAuthenticationHandle);
    vTaskSuspend(beforeAuthenticationHandle);

    xTaskCreate(afterAuthenticationTask,
                "after auth task", 2000, (void *)LED_CONNECTION, 1, &afterAuthenticationHandle);
    vTaskSuspend(afterAuthenticationHandle);

    xTaskCreate(turnTask, "left turn task", 2000, (void *)LED_LEFT, 1, &leftTurnHandle);
    vTaskSuspend(leftTurnHandle);

    xTaskCreate(turnTask, "right turn task", 2000, (void *)LED_RIGHT, 1, &rightTurnHandle);
    vTaskSuspend(rightTurnHandle);
}
