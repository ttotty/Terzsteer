#include <Arduino.h>
#include "settings.h"

#define USE_LEDS

#define BEFORE_AUTH_FREQUENCY 1000 / portTICK_PERIOD_MS
#define AFTER_AUTH_FREQUENCY 5000 / portTICK_PERIOD_MS
#define TURN_FREQUENCY 50 / portTICK_PERIOD_MS

TaskHandle_t beforeAuthenticationHandle = NULL;
TaskHandle_t afterAuthenticationHandle = NULL;
TaskHandle_t leftTurnHandle = NULL;
TaskHandle_t rightTurnHandle = NULL;
class LedIndicator
{
    static void beforeAuthenticationTask(void *parameters)
    {
        TickType_t lastWakeTime;

        vTaskSuspend(afterAuthenticationHandle);
        for (;;)
        {
            digitalWrite(LED_CONNECTION, LED_ON);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, BEFORE_AUTH_FREQUENCY);
            digitalWrite(LED_CONNECTION, LED_OFF);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, BEFORE_AUTH_FREQUENCY);
        }
    }
    static void afterAuthenticationTask(void *parameters)
    {
        TickType_t lastWakeTime;

        vTaskSuspend(beforeAuthenticationHandle);
        for (;;)
        {
            digitalWrite(LED_CONNECTION, LED_ON);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, AFTER_AUTH_FREQUENCY);
            digitalWrite(LED_CONNECTION, LED_OFF);
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, AFTER_AUTH_FREQUENCY);
        }
    }

    static void leftTurnTask(void *parameters)
    {
        TickType_t lastWakeTime;

        for (;;)
        {
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(LED_LEFT, LED_ON);
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY);
                digitalWrite(LED_LEFT, LED_OFF);
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY);
            }
            vTaskSuspend(NULL);
        }
    }
    static void rightTurnTask(void *parameters)
    {
        TickType_t lastWakeTime;

        for (;;)
        {
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(LED_RIGHT, LED_ON);
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY);
                digitalWrite(LED_RIGHT, LED_OFF);
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, TURN_FREQUENCY);
            }
            vTaskSuspend(NULL);
        }
    }

public:
    void starting(bool show)
    {
        uint8_t on = show ? LED_ON : LED_OFF;
        digitalWrite(LED_CONNECTION, on);
        digitalWrite(LED_LEFT, on);
        digitalWrite(LED_RIGHT, on);
    }

    void updateState(bool authenticated,
                     bool leftPressed,
                     bool rightPressed)
    {
        if (authenticated)
            vTaskResume(afterAuthenticationHandle);
        else
            vTaskResume(beforeAuthenticationHandle);

        if (leftPressed)
            vTaskResume(leftTurnHandle);
        if (rightPressed)
            vTaskResume(rightTurnHandle);
    }

    LedIndicator()
    {
        pinMode(LED_CONNECTION, OUTPUT);
        pinMode(LED_LEFT, OUTPUT);
        pinMode(LED_RIGHT, OUTPUT);

        xTaskCreate(
            beforeAuthenticationTask,
            "before auth task",
            2000, //stack size
            NULL,
            1, //priority
            &beforeAuthenticationHandle);
        vTaskSuspend(beforeAuthenticationHandle);

        xTaskCreate(
            afterAuthenticationTask,
            "after auth task",
            2000, //stack size
            NULL,
            1, //priority
            &afterAuthenticationHandle);
        vTaskSuspend(afterAuthenticationHandle);

        xTaskCreate(
            leftTurnTask,
            "left turn task",
            2000, //stack size
            NULL,
            1, //priority
            &leftTurnHandle);
        vTaskSuspend(leftTurnHandle);

        xTaskCreate(
            rightTurnTask,
            "right turn task",
            2000, //stack size
            NULL,
            1, //priority
            &rightTurnHandle);
        vTaskSuspend(rightTurnHandle);
    }
};