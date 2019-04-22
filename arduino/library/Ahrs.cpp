//-*- mode: c -*-
/* 
 * FILE
 *     Ahrs.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 */

#include <Arduino.h>

#include "Ahrs.h"

#include <stdio.h>

#define IMU_SAMPLE_RATE_MS 50

//void Ahrs::printFloat(float f) {
//    char buffer[9];
//    dtostrf(f, 8, 2, buffer);
//    Serial.print(buffer);
//}

//void Ahrs::printStatus() {
//    
//    Serial.print("D ");
//    /*
//    Serial.print(" A(");   printFloat(a.acceleration.x);
//    Serial.print(",");     printFloat(a.acceleration.y);
//    Serial.print(",");     printFloat(a.acceleration.z); Serial.print(")");
//    
//    Serial.print("\tM(");  printFloat(m.magnetic.x);
//    Serial.print(",");     printFloat(m.magnetic.y);
//    Serial.print(",");     printFloat(m.magnetic.z); Serial.print(")");
//    
//    Serial.print("\tG(");  printFloat(g.gyro.x);
//    Serial.print(",");     printFloat(g.gyro.y);
//    Serial.print(",");     printFloat(g.gyro.z); Serial.print(")");
//    */
//    Serial.print("\tO(");  Serial.print((int) rpy[0]);
//    Serial.print(",");     Serial.print((int) rpy[1]);
//    Serial.print(",");     Serial.print((int) rpy[2]); Serial.print(")");
//
//    Serial.println();
//}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 * PREREQUISITE: imu::setup must be called before this.
 */
void Ahrs::setup() {
    Serial.println("OI Ahrs ready.");
    filter.begin(1000 / IMU_SAMPLE_RATE_MS);
}

/**
 * Called by the Arduino library continually after setup().
 */
void Ahrs::loop(uint32_t now) {
    if(now < nextImuReadAt)
        return; // Nothing to do
    imu->readSensor();  /* Ask IMU to read in current data */ 
    filter.update(imu->gyro[0],imu->gyro[1], imu->gyro[2], imu->acceleration[0], imu->acceleration[1], imu->acceleration[2], imu->magnetic[0], imu->magnetic[1], imu->magnetic[1]); // XYZ==NWU.
    rpy[0] = (int) filter.getRoll();               // deg +ve -> left wing up.
    rpy[1] = (int) -filter.getPitch();             // deg +ve -> nose up 
    rpy[2] = ((int) -filter.getYaw() + 540) % 360; // deg CW of (magnetic) North
    dRpy[0] = (int) imu->gyro[0];                  // d-roll/dt  deg/s
    dRpy[1] = (int) imu->gyro[1];                  // d-pitch/dt deg/s
    dRpy[2] = (int) -imu->gyro[2];                 // d-yaw/dt   deg/s CW
    nextImuReadAt = now + IMU_SAMPLE_RATE_MS;
    if (reportIntervalMs > 0 && now >= nextReportAt) {
        report();
        nextReportAt = now + reportIntervalMs;
    }
}

/**
 * Write state to Serial.
 */
void Ahrs::report() {
    Serial.print("OR");
    Serial.print(rpy[0]); Serial.print(" ");
    Serial.print(rpy[1]); Serial.print(" "); 
    Serial.print(rpy[2]); Serial.print(" ");
    Serial.print((int) dRpy[0]); Serial.print(" "); 
    Serial.print((int) dRpy[1]); Serial.print(" "); 
    Serial.println((int) dRpy[2]);
}

/**
 * Command line received from host.
 * @param commandLine the line received from the host. Note that the line may not be for this object.
 */
void Ahrs::command(char *commandLine) {
    if (commandLine[0] != 'O')
        return; // Not for us.
    if (commandLine[1] == 'R') // Report interval.
        setReportInterval(atoi(commandLine + 2));
}
