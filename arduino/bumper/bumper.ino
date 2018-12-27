//-*- mode: c -*-
/*
 * Example of use of the Bumper class.
 * @see
 * Bumper.h
 * Bumper.cpp
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
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
    bumper.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    blinker.loop(now);
    bumper.loop(now);
}
