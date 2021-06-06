# Terzsteer

Zwift steering using an ESP32
 * Based on ideas and code from:
 * https://github.com/gertvantslot/zwift-steering-esp32
 * and
 * https://github.com/stefaandesmet2003/ESP32Sterzo
  
Hook up two TTP223 touch pads on ESP32 board and 3 LEDs
 * Each key-press gives +-10° on the steering angle
 * angle is sent every second
 * LEDs indicate connection/authentication state and steering direction
