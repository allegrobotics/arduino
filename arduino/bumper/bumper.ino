//-*- mode: c -*-
/*
 * NAME
 *     bumper.ino
 * PURPOSE
 *     Example of use of the Bumper class.
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 * SEE ALSO
 *     Bumper.h
 *     Bumper.cpp
 */

#include <Arduino.h>

#include "Blinker.h"
#include "Bumper.h"

Blinker blinker(LED_BUILTIN);
Bumper  bumper(12);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    while (!Serial) delay(1);
    bumper.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    blinker.loop(now);
    bumper.loop(now);
}
