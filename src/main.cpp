/*
 * Based on ideas and code from:
 * https://github.com/gertvantslot/zwift-steering-esp32
 * and
 * https://github.com/stefaandesmet2003/ESP32Sterzo
 * 
 * Hook up two TTP223 touch pads on ESP32 board and 3 LEDs
 * Each key-press gives +-10° on the steering angle
 * The steering angle is notified to the BLE client (Zwift) every second, and then reset.
 */

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "settings.h"
#include "simulation.h"

#define NAME "Terzsteer"
#define STEERING_DEVICE_UUID "347b0001-7635-408b-8918-8ff3949ce592"
#define STEERING_ANGLE_CHAR_UUID "347b0030-7635-408b-8918-8ff3949ce592" //notify
#define STEERING_RX_CHAR_UUID "347b0031-7635-408b-8918-8ff3949ce592"    //write
#define STEERING_TX_CHAR_UUID "347b0032-7635-408b-8918-8ff3949ce592"    //indicate

#define DELAY_HANDSHAKE 125
#define DELAY_HANDSHAKE2 250
#define DELAY_BLE_COOLDOWN 100
#define DELAY_BLINK 250
#define NOTIFY_NODATA_FREQUENCY 20

#define CHANNEL_CONNECTION 15
#define CHANNEL_LEFT 14
#define CHANNEL_RIGHT 13

// Timers
unsigned long timer_ble_cooldown = 0;
unsigned long timer_blink = 0;
unsigned long timer_bleNotifyMillis;
unsigned long timer_simulateLeftButton;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool authenticated = false;
float currentAngle = 0.0;

#ifdef DEBUG_TO_SERIAL
#define serialPrintln(X) Serial.println(X)
#define serialPrint(X) Serial.print(X)
#define serialBegin(X) Serial.begin(X)
#else
#define serialPrintln(X)
#define serialPrint(X)
#define serialBegin(X)
#endif

int FF = 0xFF;
byte authChallenge[4] = {0x03, 0x10, 0xff, 0xff};
byte authSuccess[3] = {0x03, 0x11, 0xff};

BLEServer *pServer = NULL;
BLECharacteristic *pAngle = NULL;
BLECharacteristic *pRx = NULL;
BLECharacteristic *pTx = NULL;
BLEAdvertising *pAdvertising;

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

void adjustAngleInBounds(float &angle)
{
    if (angle < -40.0)
        angle = -40.0;
    if (angle > 40.0)
        angle = 40.0;
}

float getAngleDelta()
{
    float delta = 0.0;
    bool buttonLeft = false;
    bool buttonRight = false;
#ifdef SIMULATE_STEERING
    static int simulationIndex = 0;

    if (millis() - timer_simulateLeftButton > SIMULATE_EVENT_FREQUENCY)
    {
        simulationIndex = simulationIndex % sizeof(simulations);
        buttonLeft = (simulations[simulationIndex] == left);
        buttonRight = (simulations[simulationIndex] == right);
        char s[50];
        snprintf_P(s, sizeof(s), PSTR("Simulate buttons: left=%u, right=%u"), buttonLeft, buttonRight);
        serialPrintln(s);
        timer_simulateLeftButton = millis();
        simulationIndex++;
    }
#else
    buttonLeft = digitalRead(LEFT_BUTTON);
    buttonRight = digitalRead(RIGHT_BUTTON);
#endif

    if (buttonLeft)
        delta = -10.0;
    if (buttonRight)
        delta = 10.0;
    return delta;
}

void handleIndicatorLights(float angle)
{
    //allways on when waiting
    if (!deviceConnected)
    {
        ledcWrite(CHANNEL_CONNECTION, 100);
    }
    else if (!authenticated)
    {
        ledcWrite(CHANNEL_CONNECTION, 255);
    }

    if (millis() - timer_blink < DELAY_BLINK)
    {
        return;
    }
    timer_blink = millis();

    if (authenticated)
    {
        ledcWrite(CHANNEL_CONNECTION, 100);
    }

    ledcWrite(CHANNEL_LEFT, map((int)angle, -41, -1, 0, 255));
    ledcWrite(CHANNEL_RIGHT, map((int)angle, 1, 40, 0, 255));
}

bool trySendAngle(float angle)
{
    //Must be connected to Zwift.
    bool isSent = false;
    // notify steering angle every second
    if (millis() - timer_bleNotifyMillis > 1000)
    {
        serialPrint("Angle: ");
        serialPrintln(angle);

        pAngle->setValue(angle);
        // pAngle->setValue((uint8_t *)&angle,4);
        pAngle->notify();
        timer_bleNotifyMillis = millis();
        isSent = true;
    }
    return isSent;
}

void authenticate()
{
    static int notifyNoDataCount = 0;

    //start the connection process
    pTx->setValue(FF);
    pTx->indicate();
    
    //handshaking
    std::string rxValue = pRx->getValue();
    if (rxValue.length() == 0)
    {
        if (notifyNoDataCount <= 0)
        {
            //no data so keep trying
            serialPrintln(".");
            notifyNoDataCount = NOTIFY_NODATA_FREQUENCY;
        }
        notifyNoDataCount--;
        delay(DELAY_HANDSHAKE);
    }
    else
    {
        serialPrintln("Handshaking....");
        if (rxValue[0] == 0x03 && rxValue[1] == 0x10)
        {
            delay(DELAY_HANDSHAKE);
            //send 0x0310FFFF (the last two octets can be anything)
            pTx->setValue(authChallenge, 4);
            pTx->indicate();
            //Zwift will now send 4 bytes as a response, which start with 0x3111
            //We don't really care what it is as long as we get a response
            delay(DELAY_HANDSHAKE);
            rxValue = pRx->getValue();
            if (rxValue.length() == 4)
            {
                //connected, so send 0x0311ff
                delay(DELAY_HANDSHAKE2);
                pTx->setValue(authSuccess, 3);
                pTx->indicate();
                authenticated = true;
                serialPrintln("Success!");
            }
        }
    }
}

void setup()
{
    serialBegin(115200);

    serialPrintln("Indicator lights on in startup...");
    pinMode(LED_CONNECTION, OUTPUT);
    pinMode(LED_LEFT, OUTPUT);
    pinMode(LED_RIGHT, OUTPUT);
    digitalWrite(LED_CONNECTION, HIGH);
    digitalWrite(LED_LEFT, HIGH);
    digitalWrite(LED_RIGHT, HIGH);

    serialPrintln("Setup buttons...");
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);

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

    serialPrintln("Staring BLE service...");
    pService->start();

    // Zwift only shows the steering button when the service is advertised
    serialPrintln("Define the advertising...");
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(true);
    pAdvertising->addServiceUUID(STEERING_DEVICE_UUID);
    pAdvertising->setMinPreferred(0x06); // set value to 0x00 to not advertise this parameter
    serialPrintln("Starting advertiser...");
    BLEDevice::startAdvertising();
    serialPrintln("Waiting a client connection to notify...");

    // LED channels
    ledcSetup(CHANNEL_CONNECTION, 5000, 8);
    ledcAttachPin(LED_CONNECTION, CHANNEL_CONNECTION);
    ledcSetup(CHANNEL_LEFT, 5000, 8);
    ledcAttachPin(LED_LEFT, CHANNEL_LEFT);
    ledcSetup(CHANNEL_RIGHT, 5000, 8);
    ledcAttachPin(LED_RIGHT, CHANNEL_RIGHT);
}

void loop()
{
    currentAngle += getAngleDelta();
    adjustAngleInBounds(currentAngle);
    handleIndicatorLights(currentAngle);

    // small interval so BLE stack doesn't get overloaded
    if (millis() - timer_ble_cooldown > DELAY_BLE_COOLDOWN)
    {
        timer_ble_cooldown = millis();
        if (deviceConnected)
        {
            if (authenticated)
            {
                if (trySendAngle(currentAngle)) currentAngle = 0.0;
            }
            else
            {
                authenticate();
            }
        }
    }

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
}