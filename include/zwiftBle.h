 /* some of the connection/authentication comes from
    https://github.com/gertvantslot/zwift-steering-esp32
*/

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "settings.h"

#define NAME "Terzsteer"
#define STEERING_DEVICE_UUID "347b0001-7635-408b-8918-8ff3949ce592"
#define STEERING_ANGLE_CHAR_UUID "347b0030-7635-408b-8918-8ff3949ce592" //notify
#define STEERING_RX_CHAR_UUID "347b0031-7635-408b-8918-8ff3949ce592"    //write
#define STEERING_TX_CHAR_UUID "347b0032-7635-408b-8918-8ff3949ce592"    //indicate

#define MAX_ANGLE 40.0

#define CONNECTION_CHECK_FREQUENCY 500
#define NOTIFY_FREQUENCY 100 / portTICK_PERIOD_MS
#define UNAUTHENTICATED_FREQUENCY 125 / portTICK_PERIOD_MS
#define AUTHENTICATED_FREQUENCY 5000 / portTICK_PERIOD_MS
#define DELAY_HANDSHAKE 125 / portTICK_PERIOD_MS
#define DELAY_HANDSHAKE2 250 / portTICK_PERIOD_MS

#define QUEUE_WRITE_TIMEOUT 5
#define QUEUE_READ_TIMEOUT 10

int FF = 0xFF;
byte authChallenge[4] = {0x03, 0x10, 0xff, 0xff};
byte authSuccess[3] = {0x03, 0x11, 0xff};
float zeroAngle = 0.0;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool authenticated = false;

BLEServer *pServer = NULL;
BLECharacteristic *pAngle = NULL;
BLECharacteristic *pRx = NULL;
BLECharacteristic *pTx = NULL;
BLEAdvertising *pAdvertising;
QueueHandle_t steeringQueue;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

class ZwiftBle
{
    static void adjustAngleInBounds(float &angle)
    {
        if (angle < -MAX_ANGLE)
            angle = -MAX_ANGLE;
        if (angle > MAX_ANGLE)
            angle = MAX_ANGLE;
    }

    static bool authenticate()
    {
        bool hasAuthenticated = false;
        TickType_t lastWakeTime;

        //start the connection process
        pTx->setValue(FF);
        pTx->indicate();

        //handshaking
        std::string rxValue = pRx->getValue();
        if (rxValue.length() == 0)
        {
            serialPrintln("No data yet...");
        }
        else
        {
            serialPrintln("Handshaking....");
            if (rxValue[0] == 0x03 && rxValue[1] == 0x10)
            {
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE);

                //send 0x0310FFFF (the last two octets can be anything)
                pTx->setValue(authChallenge, 4);
                pTx->indicate();
                //Zwift will now send 4 bytes as a response, which start with 0x3111
                //We don't really care what it is as long as we get a response
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE);
                rxValue = pRx->getValue();
                if (rxValue.length() == 4)
                {
                    //connected, so send 0x0311ff
                    lastWakeTime = xTaskGetTickCount();
                    vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE2);
                    pTx->setValue(authSuccess, 3);
                    pTx->indicate();
                    hasAuthenticated = true;
                    serialPrintln("Success!");
                }
            }
        }

        return hasAuthenticated;
    }

    static void authenticateTask(void *parameters)
    {
        for (;;)
        {
            TickType_t delay = UNAUTHENTICATED_FREQUENCY;
            if (deviceConnected)
            {
                if (authenticated || authenticate())
                {
                    authenticated = true;
                    delay = AUTHENTICATED_FREQUENCY;
                }
            }
            vTaskDelay(delay);
        }
    }

    static void notifyTask(void *parameters)
    {
        TickType_t lastWakeTime;
        for (;;)
        {
            lastWakeTime = xTaskGetTickCount();
            if (deviceConnected && authenticated)
            {
                float totalAngle = 0.0;
                float nextAngle;
                bool hasEvents = false;
                while (xQueueReceive(steeringQueue, &nextAngle, QUEUE_READ_TIMEOUT) == pdPASS)
                {
                    totalAngle += nextAngle;
                    hasEvents = true;
                }

                if (hasEvents)
                {
                    adjustAngleInBounds(totalAngle);
                    serialPrint("Angle: ");
                    serialPrintln(totalAngle);
                    pAngle->setValue(totalAngle);
                    pAngle->notify();
                }
            }
            else
            {
                //clear any events so they are not sent on connect
                float nextAngle;
                while (xQueueReceive(steeringQueue, &nextAngle, QUEUE_READ_TIMEOUT) == pdPASS)
                    ;
            }
            vTaskDelayUntil(&lastWakeTime, NOTIFY_FREQUENCY);
        }
    }

    static void connectTask(void *parameters)
    {
        for (;;)
        {
            // disconnecting
            if (!deviceConnected && oldDeviceConnected)
            {
                delay(DELAY_HANDSHAKE2);     // give the bluetooth stack the chance to get things ready
                pServer->startAdvertising(); // restart advertising
                serialPrintln("Nothing connected, start advertising");
                oldDeviceConnected = deviceConnected;
            }
            // connecting
            if (deviceConnected && !oldDeviceConnected)
            {
                oldDeviceConnected = deviceConnected;
                serialPrintln("Connecting...");
            }

            vTaskDelay(CONNECTION_CHECK_FREQUENCY / portTICK_PERIOD_MS);
        }
    }

public:
    void addNotifiableAngle(float angle)
    {
        if (xQueueSendToBack(steeringQueue, &angle, QUEUE_WRITE_TIMEOUT) == errQUEUE_FULL)
        {
            float totalAngle = angle;
            float nextAngle;
            while (xQueueReceive(steeringQueue, &nextAngle, QUEUE_READ_TIMEOUT) == pdPASS)
            {
                totalAngle += nextAngle;
            }
            xQueueSendToBack(steeringQueue, &totalAngle, QUEUE_WRITE_TIMEOUT);
        }
    }

    ZwiftBle()
    {
        serialPrintln("Creating event queue...");
        steeringQueue = xQueueCreate(5, sizeof(float));

        serialPrintln("Creating BLE server...");
        BLEDevice::init(NAME);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks());

        serialPrintln("Define service...");
        BLEService *pService = pServer->createService(STEERING_DEVICE_UUID);

        serialPrintln("Define characteristics");
        pTx = pService->createCharacteristic(STEERING_TX_CHAR_UUID, BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_READ);
        pTx->addDescriptor(new BLE2902());
        pRx = pService->createCharacteristic(STEERING_RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
        pRx->addDescriptor(new BLE2902());
        pAngle = pService->createCharacteristic(STEERING_ANGLE_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
        pAngle->addDescriptor(new BLE2902());
        pAngle->setValue(zeroAngle);

        serialPrintln("Staring BLE service...");
        pService->start();

        // Zwift only shows the steering button when the service is advertised
        serialPrintln("Define the advertising...");
        pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->setScanResponse(true);
        pAdvertising->addServiceUUID(STEERING_DEVICE_UUID);
        pAdvertising->setMinPreferred(0x06); // set value to 0x00 to not advertise this parameter

        serialPrintln("Schedule BLE tasks...");
        xTaskCreate(connectTask, "zwift connection task", 5000, NULL, 10, NULL);
        xTaskCreate(notifyTask, "zwift notify task", 5000, NULL, 30, NULL);
        xTaskCreate(authenticateTask, "zwift authenticate task", 5000, NULL, 20, NULL);

        serialPrintln("Starting advertiser...");
        BLEDevice::startAdvertising();
        serialPrintln("Waiting a client connection to notify...");
    }
};