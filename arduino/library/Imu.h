//-*- mode: c -*-
/* 
 * NAME
 *     Imu
 * PURPOSE
 *     An LSM9DS0 (or, in principle an LSM9DS1), or even BNO055.
 * PROTOCOL FROM HOST
 *     By default, does not report, but
 *     "IRnnn" will make the IMU report at nnn ms intervals.
 * PROTOCOL TO HOST
 *     If reporting, outputs "IRgx gy gz ax ay az mx my mz"
 * AUTHOR
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * COORDINATE SYSTEM
 *     XYZ=NWU (right handed, same as Madgwick)
 * NOTES
 *     It's not really clear whether the IMU should hang off a low-level unit like an Arduino or a higher level device like the Raspberry Pi.
 *     In the case where there is a high-speed differential drive, the steering corrections are going to have to come thick and fast, and 
 *     are probably better of on a 'real time' platform.
 *     Unfortunately this means puting 0-5V into the SCL/SDA lines of the Imu (instead of the recomended 0-3.3V), but the LSM modules seem
 *     to cope.
 */

#ifndef Imu_h
#define Imu_h

// Chose only one of these ..
#define DS0
// NOTE THERE ARE STILL ISSUES WITH DS1 (ie using the LSM9DS1).
// Something to do with the magnetometer readings.
// An interweb search suggests there are calibration issues with the LSM9DS1 magnetometer, but nothing that serious.
// HINT: LSM9DS1? Just say NO! (Use the LSM9DS0).
//#define DS1

#include <Arduino.h>
#include "King.h"

class Imu : public King {
    int reportIntervalMs = 0;
    uint32_t nextReportAt = 0L;
public:
    Imu() {};                        // Does not assume Serial is initialized.
    void setReportInterval(int reportIntervalMs) { this->reportIntervalMs = reportIntervalMs; }; // 0 => no reporting
    virtual void setup();            // Assumes Serial is initialized.
    virtual void loop(uint32_t now);
    virtual void command(char *commandLine);
    virtual void report();           // Write out the current readings to Serial. A: m/s^2; mag: gauss; gyro: dps; rpy: deg;
    virtual void readSensor();       // Must populate gyro, acceleration and magnetic in XYZ=NWU
    float gyro[3];                   // NWU
    float acceleration[3];           // NWU
    float magnetic[3];               // NWU
};

#endif /* Imu_h */
