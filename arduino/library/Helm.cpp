//-*- mode: c -*-
/**
 * FILE
 *     Helm.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Helm.h"
#include "Ahrs.h"
#include "DifferentialDrive.h"

/**
 * @param ahrs to work out orientation.
 * @param drive to drive the motors.
 * @param maxPower never apply more than this percent power to motors.
 * @param speedAtFullPowerMmPS estimated speed we would go if 100% power applied to motors. Assume linear for fractions.
 */
Helm::Helm(Ahrs *ahrs, DifferentialDrive *drive, int maxPower, int speedAtFullPowerMmPS) {
    this->ahrs = ahrs;
    this->drive = drive;
    this->maxPower = maxPower;
    this->speedAtFullPowerMmPS = speedAtFullPowerMmPS;
}

void Helm::loop(uint32_t now) {
    if (updateIntervalMs <= 0) // Helm is effectively turned off.
        return;
    if (stopped) {
        drive->setMotorPowers(0, 0);
        return;
    }
    if (now < nextUpdateAt)
        return;
    // Work out course correction.
    // Apply PID to correctionSpeed.
    
    int yaw = ahrs->getRpy()[2];
    int dYawDt = ahrs->getDRpy()[2];
    int courseCorrection = (((goalCourse - yaw) + 900) % 360) - 180; // How many degrees CW we have to turn to be correct [-180 .. 180]

    float K = (turningCircleMm * 3.1415 * 1000.0 / 360.0) / turnTimeMs;
    float p = pK * courseCorrection * K;
    float d = -dK * dYawDt * K;
    float i = 0.0; // (int) (iK * counter); // for now .. maybe do something with this later.
    // Units of p,i,d are actually mm/s.
    
    int basePower  = 100L * goalSpeedMmPS / speedAtFullPowerMmPS;
    basePower = min(max(basePower, -maxPower), maxPower);

    int turnPower  = 100L * (p + i + d) / speedAtFullPowerMmPS;
    turnPower = min(max(turnPower, -maxPower), maxPower);

    int leftPower = min(max(basePower + turnPower, -maxPower), maxPower);
    int rightPower = min(max(basePower - turnPower, -maxPower), maxPower);

    Serial.print("HD Helm Y "); Serial.print(yaw); Serial.print(" dY/dt "); Serial.print(dYawDt); Serial.print(" cCor "); Serial.print(courseCorrection);
    Serial.print(" BP "); Serial.print(basePower);
    Serial.print(" K "); Serial.print(K);
    Serial.print(" p "); Serial.print(p); Serial.print(" d "); Serial.print(d);
    Serial.print(" lp "); Serial.print(leftPower); Serial.print(" rp "); Serial.println(rightPower);
    drive->setMotorPowers(leftPower, rightPower);
    nextUpdateAt = now + updateIntervalMs;
}

/**
 * @param course deg CW of N
 * @param speed MmPS
 * @param howSoonMs how long to do a 180 course correction.
 */
void Helm::setCourseAndSpeed(int course, int speed, int turnTimeMs) {
    stopped = 0;
    this->goalCourse = (720 + course) % 360;
    this->goalSpeedMmPS = speed;
    this->turnTimeMs = turnTimeMs;
    Serial.print("HD setCourseAndSpeed "); Serial.print(course); Serial.print(" "); Serial.print(speed); Serial.print(" "); Serial.println(turnTimeMs);
}

/**
 * El-cheapo (toy?) version of atoi which doesn't insist about a null terminator.
 */
int atoy(char *s) {
    int y = 0;
    while (*s >= '0' && *s <= '9')
        y = y * 10 + *(s++) - '0';
    return y;
}

/**
 * @param commandLine the line received from the host. Note that the line may not be for this object.
 */
void Helm::command(char *commandLine) {
    if (commandLine[0] != 'H') return;         // We are only interested in "H...", so this is not for us.
    if (commandLine[1] == '0') {               // "H0" stop
        fullStop();
    } else if (commandLine[1] == 'C') {        // HCccc sss
        char *p = commandLine + 2;
        int course = 0;
        while (*p >= '0' && *p <= '9')
            course = course * 10 + *(p++) - '0';
        if (*p != ' ')
            return; // Bad line format. Ignore.
        p++;
        int speed = 0;
        while (*p >= '0' && *p <= '9')
            speed = speed * 10 + *(p++) - '0';
        int turnTimeMs = 0;
        if (*p == ' ') {
            p++;
            while (*p >= '0' && *p <= '9')
                turnTimeMs = turnTimeMs * 10 + *(p++) - '0';
        }
        if (turnTimeMs == 0)
            turnTimeMs = 1000;
        setCourseAndSpeed(course, speed, turnTimeMs);
    } else if (commandLine[1] == 'S') {
        if (commandLine[2] == 'P') {           // "HSPnnn.nn" - set pK (of PID)
            pK = atof(commandLine + 3);
        } else if (commandLine[2] == 'I') {    // "HSInnn.nn" - set iK (of PID)
            iK = atof(commandLine + 3);
        } else if (commandLine[2] == 'D') {    // "HSDnnn.nn" - set dK (of PID)
            dK = atof(commandLine + 3);
        } else if (commandLine[2] == 'M') {    // "HSMnnn" - set max power (percent)
            maxPower = atoi(commandLine + 3);
        } else if (commandLine[2] == 'T') {    // "HSTnnn" - set turningCircleMm
            turningCircleMm = atoi(commandLine + 3);
        } else if (commandLine[2] == 'C') {    // "HSCnnn" - set update interval (HSC0 turns off the Helm)
            updateIntervalMs = atoi(commandLine + 3);
        }
    }
}

void Helm::setStopped(byte stopped) {
    this->stopped = stopped;
    // That won't stop immediately, but the next time we do a speed check (at nextUpdateAt). No hurry.
}

void Helm::fullStop() {
    setStopped(true);
    nextUpdateAt = 0L; // Force quick motor drop
}

void Helm::emergencyStop() {
    drive->setMotorPowers(0, 0);
    setStopped(true);
}
