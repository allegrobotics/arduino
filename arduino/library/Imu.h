//-*- mode: c -*-
/* 
 * NAME
 *     Imu
 * PURPOSE
 *     An LSM9DS0 (or, in principle an LSM9DS1), or even BNO055.
 * OUTPUT TO HOST
 *     None
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * COORDINATE SYSTEM
 *     XYZ=NWU (right handed, same as Madgwick)
 */

#ifndef Imu_h
#define Imu_h

// Chose only one of these ..
#define DS0
// NOTE THERE ARE STILL ISSUES WITH DS1 (ie using the LSM9DS1).
// Something to do with the magnetometer readings.
// A interweb search suggests there are calibration issues with the LSM9DS1 magnetometer, but nothing that serious.
// HINT: LSM9DS1? Just say NO! (Use the LSM9DS0).
//#define DS1

#include <Arduino.h>

class Imu {
    int reportIntervalMs = 0;
    uint32_t nextReportAt = 0L;
public:
    Imu() {};                        // Does not assume Serial is initialized.
    void setReportInterval(int reportIntervalMs) { this->reportIntervalMs = reportIntervalMs; }; // 0 => no reporting
    virtual void setup();            // Assumes Serial is initialized.
    virtual void report();           // Write out the current readings to Serial. A: m/s^2; mag: gauss; gyro: dps; rpy: deg;
    virtual void readSensor();       // Must populate gyro, acceleration and magnetic in XYZ=NWU
    virtual void loop(uint32_t now);
    virtual void command(char *commandLine);
    float gyro[3];                   // NWU
    float acceleration[3];           // NWU
    float magnetic[3];               // NWU
};

#endif /* Imu_h */
