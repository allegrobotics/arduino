//-*- mode: c -*-
/* 
 * NAME
 *     Ahrs
 * PURPOSE
 *     Given a LSM9DS0 (or, in principle an LSM9DS1), calculates its orientation
 * OUTPUT TO HOST
 *     None
 * DEPENDENCIES
 *     Adafruit_LSMDS0 (Adafruit_LSMDS1)
 *     Adafruit_Sensor
 *     MadgwickAHRS
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * BUGS
 *     Doesn't work with LSM9DS1.
 *     This seems to be an issue with the magnetometer, but further investigation required.
 * COORDINATE SYSTEM
 *     Standard aircraft roll/pitch/yaw is NWD (left handed; +roll=>left wing up; +pitch => nose up; +yaw = nose to right), but Madgwick is right handed
 *     XYZ=NWU (right handed, same as Madgwick)
 *     External interface to Ahrs is NWD, Ahrs is handled.
 */

#ifndef Ahrs_h
#define Ahrs_h

// NOTE THERE ARE STILL ISSUES WITH DS1 (ie using the LSM9DS1).
// Something to do with the magnetometer readings.
// A interweb search suggests there are calibration issues with the LSM9DS1 magnetometer, but nothing that serious.
// HINT: LSM9DS1? Just say NO! (Use the LSM9DS0).

#include <Arduino.h>
#include "Imu.h"
#include "MadgwickAHRS.h"

class Ahrs {
private:
    void printFloat(float);
    Imu *imu;
    Madgwick filter;
    byte imuDump = 1;
    uint32_t nextImuReadAt = 0L;
    uint32_t nextReportAt = 0L;
    int reportIntervalMs = 333; // Default is report thrice per second.
    int rpy[3]; // roll, pitch, yaw. Degrees.
    int dRpy[3]; // d-roll/dt, d-pitch/dt, d-yaw/dt. deg/s
public:
    Ahrs(Imu *imu) { this->imu = imu; }
    // Must be called from Arduino startup.
    virtual void setup();
    // Must be called from Arduino loop each time around.
    virtual void loop(uint32_t now);
    // Reports on current state, as instructed.
    virtual void report();
    // Instructs Ahrs to report this often (0 is never).
    void setReportInterval(uint32_t reportIntervalMs) { this->reportIntervalMs = reportIntervalMs; };
    // Returns roll pitch yaw (NWD)
    int *getRpy() { return rpy; };
    // Returns roll pitch yaw rates (NWD)
    int *getDRpy() { return dRpy; };
    // A command line has been received from the host - pass it to the Ahrs.
    virtual void command(char *commandLine);
};

#endif /* Ahrs_h */
