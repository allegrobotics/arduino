//-*- mode: c -*-
/*
 * The GizMow uses the ParkingSensor, the Rpm and the collision Bumper.
 * @see
 * http://allegrobotics.com/parkingSensor.html
 * ParkingSensor2.h
 * ParkingSensor2.cpp
 * Rpm.h
 * Rpm.cpp
 * Bumper.h
 * Bumper.cpp
 * Blinker.h
 * Blinker.cpp
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>
#include "Blinker.h"
//#include "ParkingSensor1.h"
#include "ParkingSensor2.h"
#include "Bumper.h"
#include "Rpm.h"

Blinker        blinker(LED_BUILTIN);
//ParkingSensor1 parkingSensor(2);
ParkingSensor2 parkingSensor(2);
Rpm            rpm(3);
Bumper         bumper(12);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    parkingSensor.setup();
    rpm.setup();
    bumper.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor.loop(now);
    bumper.loop(now);
    rpm.loop(now);
    blinker.loop(now);
}
