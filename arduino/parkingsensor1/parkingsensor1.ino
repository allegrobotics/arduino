//-*- mode: c -*-
/*
 * Example of use of ParkingSensor1 class.
 * @see
 * ParkingSensor1.h
 * ParkingSensor1.cpp
 * http://allegrobotics.com/parkingSensor.html
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "ParkingSensor1.h"

#define PARKING_SENSOR_PIN           2 /* 2 for pin D2, 3 for pin D3 */
#define PARKING_SENSOR_PIN_INTERRUPT 0 /* digitalPinToInterrupt(PARKING_SENSOR_PIN) */

Blinker blinker(LED_BUILTIN);
ParkingSensor1 parkingSensor1(PARKING_SENSOR_PIN, PARKING_SENSOR_PIN_INTERRUPT);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    parkingSensor1.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor1.loop(now);
    blinker.loop(now);
}
