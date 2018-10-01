//-*- mode: c -*-
/*
 * Main program for a parking sensor type 1.
 * @see
 * ParkingSensor1.h
 * ParkingSensor1.cpp
 * http://allegrobotics.com/parkingSensor.html
 * COPYRIGHT
 *     Scott BARNES 2017/2018
 *     IP Freely on non-commercial applications.
 */

#include "Blinker.h"
#include "ParkingSensor1.h"

Blinker blinker(13);
ParkingSensor1 parkingSensor1(2);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    parkingSensor1.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor1.loop(now);
    blinker.loop(now);
}
