# Terzsteer

Zwift steering using an ESP32

Using some ideas and code from:
 https://github.com/gertvantslot/zwift-steering-esp32
 and some from here too:
 https://github.com/stefaandesmet2003/ESP32Sterzo
  
Hook up two TTP223 touch pads on ESP32 board and 3 LEDs

 * Each key-press gives +-10° on the steering angle
 * Button presses are checked as fast as the main loop runs
 ** But if the button simulation is running it only creates button presses down to 175 milliseconds 
 * Every 100 milliseconds if there was a button press the BLE zwift client is notified
 ** If there was no button press then for 1 second then 0 angle is sent to the BLE zwift client to centre the steering
 ** Multiple quick button presses occuring between the 100 millisecond notification frequency are added together and sent in one go to the BLE zwift client. Cumulative angles outside the range -40° to 40° are adjusted
