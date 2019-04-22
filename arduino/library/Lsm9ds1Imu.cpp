//-*- mode: c -*-
/* 
 * FILE
 *     Lsm9ds1Imu.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 */

#include <Arduino.h>

#include "Lsm9ds1Imu.h"

#include <stdio.h>
#include <Wire.h>
#include <SPI.h>


#define LSM9DSx_SCK  A5
#define LSM9DSx_MISO 12
#define LSM9DSx_MOSI A4
#define LSM9DSx_XGCS  6
#define LSM9DSx_MCS   5
// You can also use software SPI
//Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(LSM9DS1_SCK, LSM9DS1_MISO, LSM9DS1_MOSI, LSM9DS1_XGCS, LSM9DS1_MCS);
// Or hardware SPI! In this case, only CS pins are passed in
//Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(LSM9DS1_XGCS, LSM9DS1_MCS);


#define IMU_SAMPLE_RATE_MS 50

Lsm9ds1Imu::Lsm9ds1Imu() {
    // Hmm .. Serial may not yet be up. Wait for setup() to be called to do stuff.
}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void Lsm9ds1Imu::setup() {
    Serial.println("I Lsm9ds1Imu ready.");
    // Try to initialise and warn if we couldn't detect the chip
    if (!lsm9ds1.begin()) {
        Serial.println("E ERROR ... unable to initialize the LSM9DSx. Check your wiring!");
        while (1);
    }
    Serial.println("D Found LSM9DS1");
    // 1.) Set the accelerometer range
    lsm9ds1.setupAccel(lsm9ds1.LSM9DS1_ACCELRANGE_2G);
    //lsm9ds1.setupAccel(lsm9ds1.LSM9DS1_ACCELRANGE_4G);
    //lsm9ds1.setupAccel(lsm9ds1.LSM9DS1_ACCELRANGE_8G);
    //lsm9ds1.setupAccel(lsm9ds1.LSM9DS1_ACCELRANGE_16G);
    
    // 2.) Set the magnetometer sensitivity
    lsm9ds1.setupMag(lsm9ds1.LSM9DS1_MAGGAIN_4GAUSS);
    //lsm9ds1.setupMag(lsm9ds1.LSM9DS1_MAGGAIN_8GAUSS);
    //lsm9ds1.setupMag(lsm9ds1.LSM9DS1_MAGGAIN_12GAUSS);
    //lsm9ds1.setupMag(lsm9ds1.LSM9DS1_MAGGAIN_16GAUSS);
    
    // 3.) Setup the gyroscope
    lsm9ds1.setupGyro(lsm9ds1.LSM9DS1_GYROSCALE_245DPS);
    //lsm9ds1.setupGyro(lsm9ds1.LSM9DS1_GYROSCALE_500DPS);
    //lsm9ds1.setupGyro(lsm9ds1.LSM9DS1_GYROSCALE_2000DPS);
}


void Lsm9ds1Imu::readSensor() {
    lsm9ds1.read();
    sensors_event_t a, m, g, temp;    
    lsm9ds1.getEvent(&a, &m, &g, &temp);
    // This section is seriously messed up.
    // It need to be thought through from scratch, and then compared to whatever TF the LSM9DS1 magnetometer is actually up to.
    gyro[0] = g.gyro.x;
    gyro[1] = -g.gyro.y;
    gyro[2] = g.gyro.z;
    acceleration[0] = a.acceleration.x;
    acceleration[1] = -a.acceleration.y;
    acceleration[2] = a.acceleration.z;
    magnetic[0] = -m.magnetic.x;
    magnetic[1] = m.magnetic.y;
    magnetic[2] = m.magnetic.z;
}

