#ifndef ZWIFTBLE_H
#define ZWIFTBLE_H
#include <Arduino.h>
#include "buttonPressEvent.h"

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
#define BUTTON_EVENT_EXPIRE 900 //milliseconds
#define QUEUE_SIZE 20
#define QUEUE_WRITE_TIMEOUT 10
#define QUEUE_READ_TIMEOUT 5

class ZwiftBle
{
public:
    bool getAuthenticated();
    void addNotifiableAngle(ButtonPressEvent buttonEvent);
    ZwiftBle();
    void setupBle();
};
#endif