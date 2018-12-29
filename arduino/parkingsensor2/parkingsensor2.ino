//-*- mode: c -*-
/*
 * Example of use of ParkingSensor2 class.
 * @see
 * ParkingSensor2.h
 * ParkingSensor2.cpp
 * http://allegrobotics.com/parkingSensor.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "ParkingSensor2.h"

#define PARKING_SENSOR_PIN           2 /* 2 for pin D2, 3 for pin D3 */
#define PARKING_SENSOR_PIN_INTERRUPT 0 /* digitalPinToInterrupt(PARKING_SENSOR_PIN_INTERRUPT) */

Blinker        blinker(LED_BUILTIN);
ParkingSensor2 parkingSensor2(PARKING_SENSOR_PIN, PARKING_SENSOR_PIN_INTERRUPT);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    parkingSensor2.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor2.loop(now);
    blinker.loop(now);
}
