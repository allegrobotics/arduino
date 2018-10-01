//-*- mode: c -*-
/*
 * Main program for a parking sensor type 2.
 * @see
 * ParkingSensor2.h
 * ParkingSensor2.cpp
 * http://allegrobotics.com/parkingSensor.html
 * COPYRIGHT
 *     Scott BARNES 2018
 *     IP Freely on non-commercial applications.
 */

#include "Blinker.h"
#include "ParkingSensor2.h"

Blinker blinker(13);
ParkingSensor2 parkingSensor2(2);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    parkingSensor2.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor2.loop(now);
    blinker.loop(now);
}
