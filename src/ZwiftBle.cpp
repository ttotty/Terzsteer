#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "Settings.h"
#include "ZwiftBle.h"
#include "buttonPressEvent.h"

int FF = 0xFF;
byte authChallenge[4] = {0x03, 0x10, 0xff, 0xff};
byte authSuccess[3] = {0x03, 0x11, 0xff};

static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static bool authenticated = false;

static BLEServer *pServer = NULL;
static BLECharacteristic *pAngle = NULL;
static BLECharacteristic *pRx = NULL;
static BLECharacteristic *pTx = NULL;
static BLEAdvertising *pAdvertising;
static QueueHandle_t steeringQueue;

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

static void adjustAngleInBounds(float &angle)
{
    if (angle < -MAX_ANGLE)
        angle = -MAX_ANGLE;
    if (angle > MAX_ANGLE)
        angle = MAX_ANGLE;
}

static void clearQueue()
{
    ButtonPressEvent nextEvent;
    while (xQueueReceive(steeringQueue, &nextEvent, QUEUE_READ_TIMEOUT) == pdPASS)
        ;

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
        serialPrintln("No data...");
    }
    else
    {
        serialPrintln("Handshaking....");
        if (rxValue[0] == 0x03 && rxValue[1] == 0x10)
        {
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE / portTICK_PERIOD_MS);

            //send 0x0310FFFF (the last two octets can be anything)
            pTx->setValue(authChallenge, 4);
            pTx->indicate();
            //Zwift will now send 4 bytes as a response, which start with 0x3111
            //We don't really care what it is as long as we get a response
            lastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE / portTICK_PERIOD_MS);
            rxValue = pRx->getValue();
            if (rxValue.length() == 4)
            {
                //connected, so send 0x0311ff
                lastWakeTime = xTaskGetTickCount();
                vTaskDelayUntil(&lastWakeTime, DELAY_HANDSHAKE2 / portTICK_PERIOD_MS);
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
        TickType_t delay = UNAUTHENTICATED_FREQUENCY / portTICK_PERIOD_MS;
        if (deviceConnected)
        {
            if (authenticated || authenticate())
            {
                authenticated = true;
                delay = AUTHENTICATED_FREQUENCY / portTICK_PERIOD_MS;
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
            bool hasEvents = false;

            unsigned long now = millis();
            ButtonPressEvent *pActiveEvents[QUEUE_SIZE];
            int activeCount = 0;
            ButtonPressEvent nextEvent;
            while (xQueueReceive(steeringQueue, &nextEvent, QUEUE_READ_TIMEOUT) == pdPASS)
            {
                //ignore old events
                if (!nextEvent.isExpired(now))
                {
                    float angle = nextEvent.getAngleDelta();
                    totalAngle += angle;
                    hasEvents = true;

                    //don't re add the 0 events even if they are current
                    if (angle != 0)
                    {
                        pActiveEvents[activeCount++] = &nextEvent;
                    }
                }
            }

            if (hasEvents)
            {
                adjustAngleInBounds(totalAngle);
                serialPrint(totalAngle);
                serialPrintln("??");
                pAngle->setValue(totalAngle);
                pAngle->notify();

                //add the active events back in so they are still used until they expire
                //they are ordered so do the most recent first
                for (int i = activeCount - 1; i >= 0; i--)
                {
                    ButtonPressEvent oldEvent = *pActiveEvents[i];
                    //serialPrintln(oldEvent.created);
                    //events are ordered so stop if the older events can't fit in the queue
                    //as they can be discarded
                    if (errQUEUE_FULL == xQueueSendToFront(steeringQueue, &oldEvent, 0))
                        break;
                }
            }
        }
        else
        {
            //clear any events so they are not sent on connect
            clearQueue();
        }
        vTaskDelayUntil(&lastWakeTime, NOTIFY_FREQUENCY / portTICK_PERIOD_MS);
    }
}

static void connectTask(void *parameters)
{
    for (;;)
    {
        // disconnecting
        if (!deviceConnected && oldDeviceConnected)
        {
            // give the bluetooth stack the chance to get things ready
            delay(DELAY_HANDSHAKE2 / portTICK_PERIOD_MS);
            pServer->startAdvertising();
            serialPrintln("Nothing connected, start advertising");
            oldDeviceConnected = deviceConnected;

            //force re-authentication in case zwift restarts
            authenticated = false;
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

bool ZwiftBle::getAuthenticated()
{
    return authenticated;
}

bool ZwiftBle::addNotifiableAngle(ButtonPressEvent buttonEvent)
{
    //discard expired button events
    if (buttonEvent.isExpired(millis()))
        return false;

    bool added = false;
    if(buttonEvent.isStraight())
    {
        //go straight so clear out the queue
        clearQueue();
    }
    
    if (xQueueSendToBack(steeringQueue, &buttonEvent, QUEUE_WRITE_TIMEOUT) == errQUEUE_FULL)
    {
        //too many events so remove the oldest so we can try again
        ButtonPressEvent nextEvent;
        xQueueReceive(steeringQueue, &nextEvent, QUEUE_READ_TIMEOUT);
        //even if that did not work we have another go in case he queue has space now
        added = (pdPASS == xQueueSendToBack(steeringQueue, &buttonEvent, QUEUE_WRITE_TIMEOUT));
    }
    else
    {
        added = true;
    }
    return added;
}

ZwiftBle::ZwiftBle()
{
    serialPrintln("Creating event queue...");
    steeringQueue = xQueueCreate(QUEUE_SIZE, sizeof(ButtonPressEvent));
}

void ZwiftBle::setupBle()
{
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
    float zeroAngle = 0.0;
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
