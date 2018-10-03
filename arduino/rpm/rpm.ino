//-*- mode: c -*-
/* 
 * NAME
 *     rpm.ino
 * PURPOSE
 * Testbed or run bed for rpm.
 */

#include <Arduino.h>
#include "Blinker.h"
#include "Rpm.h"

Blinker blinker(13); // LED_PIN is 13 on the Nano.
Rpm rpm(3);          // If we intend to run this in conjunction with the ParkingSensor, then use pin 3 on the Nano.

void setup() {
    blinker.setup();
    rpm.setup();
}

void loop() {
    uint32_t now = millis();
    rpm.loop(now);
    blinker.loop(now);
}
