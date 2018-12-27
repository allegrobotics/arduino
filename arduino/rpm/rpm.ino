//-*- mode: c -*-
/* 
 * WARNING/WIP
 *     AS AT 2018-11-03 THIS HAS NOT BEEN TESTED.
 * NAME
 *     rpm.ino
 * PURPOSE
 *     Testbed or run bed for Rpm.
 * SEE
 *     http://allegrobotics.com/rpm.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "Rpm.h"

Blinker blinker(LED_BUILTIN);
Rpm rpm(3);          // If we intend to run this in conjunction with the ParkingSensor, then use pin 3 on the Nano.

void setup() {
    delay(3000);
    Serial.begin(19200);
    blinker.setup();
    rpm.setup();
}

void loop() {
    uint32_t now = millis();
    rpm.loop(now);
    blinker.loop(now);
}
