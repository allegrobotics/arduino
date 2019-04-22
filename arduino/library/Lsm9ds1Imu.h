//-*- mode: c -*-
/* 
 * NAME
 *     Lsm9ds1Imu
 * PURPOSE
 *     An LSM9DS1
 * OUTPUT TO HOST
 *     None
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * COORDINATE SYSTEM
 *     XYZ=NWU (right handed, same as Madgwick)
 * BUGS
 *     THIS DOES NOT WORK WITH THE Ahrs - SOMETHING TO DO WITH THE MAGNETIC READINGS / NWU. (Something to ponder later on a winter evening).
 */

#ifndef Lsm9ds1Imu_h
#define Lsm9ds1Imu_h

#include <Arduino.h>
#include "Imu.h"
#include "Adafruit_LSM9DS1.h"
#include "Adafruit_Sensor.h"

class Lsm9ds1Imu : public Imu {
private:
    Adafruit_LSM9DS1 lsm9ds1 = Adafruit_LSM9DS1();
public:
    Lsm9ds1Imu();
    virtual void setup();
    virtual void readSensor();
    virtual void report();
};

#endif /* Lsm9ds1Imu_h */
