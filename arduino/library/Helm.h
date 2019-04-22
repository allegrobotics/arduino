//-*- mode: c -*-
/* 
 * NAME
 *     Helm
 * PURPOSE
 *     High(er) level motor driver - pass a speed and direction to drive mechanism.
 * OUTPUT TO HOST
 * 
 * PHILOSOPHY
 *     Speed, not power. Helm has responsibility to do the mapping for the particular drive.
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 * PROTOCOL FROM HOST
 *     "H0"           - (zero) Stop immediately.
 *     "HCccc sss"    - Set course to atoi(ccc) [-180 .. 180] and speed to atoi(sss) [-100 .. 100].
 *     "HSPnnn.nn"    - Set pK to atof(nnn.nn)
 *     "HSInnn.nn"    - Set iK to atof(nnn.nn)
 *     "HSDnnn.nn"    - Set dK to atof(nnn.nn)
 *     "HSMnnn"       - Set max power to atoi(nnn)
 *     "HSTnnn"       - Set update interval (ie how often we update the Drive). Zero is never.
 * PROTOCOL TO HOST
 *     "HD arbitrary debugging message which could be logged"
 */

#ifndef Helm_h
#define Helm_h

#include <Arduino.h>
#include "Ahrs.h"
#include "HoverboardDrive.h"

class Helm {
private:
    Ahrs *ahrs;
    DifferentialDrive *drive;
    boolean stopped = true;
    int maxPower = 50;               // Never direct the Drive to power outside [-maxPower .. +maxPower]
    int speedAtFullPowerMmPS = 1000; // Estimated speed at 100% power.
    float pK = 1.0;
    float iK = 0.0;                  // Add some of this later.
    float dK = 0.5;                  // Put a value for this later, when the Hall feedback is available.
    int updateIntervalMs = 50;
    uint64_t nextUpdateAt = 0L;
    int goalCourse = 0;              // [0 .. 359] deg CW of N. This is the course we have been INSTRUCTED to follow.
    int goalSpeedMmPS = 0;           // [-100 .. 100] -ve is backwards. This is the speed we have been INSTRUCTED to go.
    int turningCircleMm = 520;       // In the case of Differential drive, this is the wheel base. Ackerman drive .. um .. maybe something else.
    int turnTimeMs = 1000;
public:
    Helm(Ahrs *ahrs, DifferentialDrive *drive, int maxPower, int speedAtFullPowerMmPS);
    virtual void loop(uint32_t now);
    virtual void setCourseAndSpeed(int course, int speedMmPS, int turnTimeMs); // speedMmPS must not be -ve
    virtual void setStopped(byte stopped);
    virtual void emergencyStop();
    virtual void fullStop();
    virtual void command(char *commandLine);
};

#endif /* Helm_h */
