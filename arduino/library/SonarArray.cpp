//-*- mode: c -*-
/* 
 * NAME
 *     SonarArray.cpp
 * PRECIS
 *     Five HCSR04s/HCSR05s for obstacle detection .. carefully placed to avoid the control pins used by the motor driver.
 *     Intention is that they run clockwise, 45 degrees apart with sonor[0] pointing left, and sonar[4] pointing right for a MazeRunner.
 * AUTHOR
 *     Copyright Live Software 2018-11-30 All Rights Reserved.
 *     IP Freely on non-commercial applications.
 */

#include "SonarArray.h"
#include <NewPing.h>

// Hardwired setup. Change as required.
// This messy configuration should be parameterized, but that would be messy.
// This could be done in an parameter array, but rough and ready is okay.
// Wiring looks strange -  issues with placement, cable lengths etc, and we have to avoid A0, A1, 0, 12, 3, 8, 13 and 11.
// Note that we are sharing the TRIGGER and ECHO pins - NewPing is smart enough to handle that.
#define TRIGGER_PIN_0	    2
#define ECHO_PIN_0	    2
#define TRIGGER_PIN_1	    7
#define ECHO_PIN_1	    7
#define TRIGGER_PIN_2	   10
#define ECHO_PIN_2	   10
#define TRIGGER_PIN_3	    6
#define ECHO_PIN_3	    6
#define TRIGGER_PIN_4	    4
#define ECHO_PIN_4	    4
#define MAX_DISTANCE_CM   450

NewPing *sonar[NUM_SONARS];

SonarArray::SonarArray() {
    sonar[0] = new NewPing(TRIGGER_PIN_0, ECHO_PIN_0, MAX_DISTANCE_CM);
    sonar[1] = new NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE_CM);
    sonar[2] = new NewPing(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE_CM);
    sonar[3] = new NewPing(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE_CM);
    sonar[4] = new NewPing(TRIGGER_PIN_4, ECHO_PIN_4, MAX_DISTANCE_CM);
    for (int i = 0; i < NUM_SONARS; i++)
        sonarDistanceCm[i] = 0;
}

void SonarArray::setup() {
}

int currentSonar = 0; // We cycle around the sonars.

uint32_t nextPingAt = 0L;

/**
 * @return distance in cm, or 999 if no object found.
 */
int SonarArray::pingCm(byte n) {
    int cm = sonar[n]->ping_cm();
    return cm == 0 ? 999 : cm;
}

/**
 * Do only one ping per loop().
 */
void SonarArray::loop(uint32_t now) {
    if (now < nextPingAt)
        return; // Too close to previous ping.
    sonarDistanceCm[currentSonar] = pingCm(currentSonar);

    //Serial.print(currentSonar); Serial.print(" "); Serial.println(sonarDistanceCm[currentSonar]);
    currentSonar = (currentSonar + 1) % NUM_SONARS;
    nextPingAt = now + 50; // 50ms between pings.
}
