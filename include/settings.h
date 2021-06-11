#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>

// Define DEBUG_TO_SERIAL, to write debug information to the serial port
#define DEBUG_TO_SERIAL
//#define SERIAL_ALL_EVENTS
// Define SIMULATE_STEERING to turn on simulation
#define SIMULATE_STEERING

#ifdef DEBUG_TO_SERIAL
#define serialPrintln(X) Serial.println(X)
#define serialPrint(X) Serial.print(X)
#define serialBegin(X) Serial.begin(X)
#else
#define serialPrintln(X)
#define serialPrint(X)
#define serialBegin(X)
#endif

// Pin allocation
#define LEFT_BUTTON 32
#define RIGHT_BUTTON 33
#define LED_CONNECTION 17
#define LED_LEFT 19
#define LED_RIGHT 18

#define LED_ON LOW
#define LED_OFF HIGH

#define BUTTON_PRESS_ANGLE 10.0
#endif