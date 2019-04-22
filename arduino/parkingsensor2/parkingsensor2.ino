//-*- mode: c -*-
/*
 * NAME
 *     parkingsensor2.ino
 * PURPOSE
 *     Example of use of ParkingSensor2 class.
 * SEE ALSO
 *     http://allegrobotics.com/parkingSensor.html
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
    delay(2000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    while (!Serial) delay(1);
    parkingSensor2.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor2.loop(now);
    blinker.loop(now);
}
