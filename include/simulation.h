#include <Arduino.h>

// Define SIMULATE_STEERING to turn on simulation
#define SIMULATE_STEERING
#define SIMULATE_EVENT_FREQUENCY 400
enum direction { left, right, straight };
const direction simulations[20] = {left, left, straight, straight, straight, straight, right, 
                                   straight, straight, straight, straight, straight, right, right, 
                                   right, right, straight, straight,straight,straight};
