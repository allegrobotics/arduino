//-*- mode: c -*-
/* 
 * NAME
 *     rpm.ino
 * PURPOSE
 *     Testbed or run bed for Rpm.
 * SEE
 *     http://allegrobotics.com/rpm.html
 * COPYRIGHT
 *     Scott BARNES 2018/2019. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "Rpm.h"

#define RPM_PIN           2 /* 2 for pin D2, 3 for pin D3 */
#define RPM_PIN_INTERRUPT 0 /* digitalPinToInterrupt(RPM_PIN) */

Blinker blinker(LED_BUILTIN);
Rpm rpm(RPM_PIN, RPM_PIN_INTERRUPT);          // If we intend to run this in conjunction with the ParkingSensor, then use pin D3 on the Nano.

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
