#include <Arduino.h>
#include "settings.h"

#define USE_LEDS

TaskHandle_t beforeAuthenticationHandle = NULL;
TaskHandle_t afterAuthenticationHandle = NULL;
TaskHandle_t leftTurnHandle = NULL;
TaskHandle_t rightTurnHandle = NULL;
class LedIndicator
{
    static void beforeAuthenticationTask(void *parameters)
    {
        vTaskSuspend(afterAuthenticationHandle);
        for (;;)
        {
            digitalWrite(LED_CONNECTION, LED_ON);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            digitalWrite(LED_CONNECTION, LED_OFF);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    static void afterAuthenticationTask(void *parameters)
    {
        vTaskSuspend(beforeAuthenticationHandle);
        for (;;)
        {
            digitalWrite(LED_CONNECTION, LED_ON);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            digitalWrite(LED_CONNECTION, LED_OFF);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    static void leftTurnTask(void *parameters)
    {
        for (;;)
        {
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(LED_LEFT, LED_ON);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                digitalWrite(LED_LEFT, LED_OFF);
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            vTaskSuspend(NULL);
        }
    }
    static void rightTurnTask(void *parameters)
    {
        for (;;)
        {
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(LED_RIGHT, LED_ON);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                digitalWrite(LED_RIGHT, LED_OFF);
                vTaskDelay(50 / portTICK_PERIOD_MS);
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