//-*- mode: c -*-
/* 
 * NAME
 *     Lsm9ds0Imu
 * PURPOSE
 *     An LSM9DS0 inertial measurement unit on an Arduino.
 * PROTOCOL FROM HOST
 *     See parent class.
 * PROTOCOL TO HOST
 *     See parent class.
 * AUTHOR
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * COORDINATE SYSTEM
 *     XYZ=NWU (right handed, same as Madgwick)
 */

#ifndef Lsm9ds0Imu_h
#define Lsm9ds0Imu_h

#include <Arduino.h>
#include "King.h"
#include "Imu.h"
#include "Adafruit_LSM9DS0.h"
#include "Adafruit_Sensor.h"

class Lsm9ds0Imu : public Imu {
private:
    Adafruit_LSM9DS0 lsm9ds0 = Adafruit_LSM9DS0();
public:
    Lsm9ds0Imu();
    virtual void setup();
    virtual void readSensor();
};

#endif /* Lsm9ds0Imu_h */
