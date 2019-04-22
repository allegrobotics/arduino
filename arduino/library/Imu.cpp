//-*- mode: c -*-
/* 
 * FILE
 *     Imu.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 */

#include <Arduino.h>
#include "Imu.h"
#include <stdio.h>

// Note the loop doesn't read the sensor - it just looks after reporting.
void Imu::loop(uint32_t now) {
    if (reportIntervalMs > 0 && now >= nextReportAt) {
        report();
        nextReportAt = now + reportIntervalMs;
    }
}

void Imu::report() {
    char b[12];
    Serial.print("IR");
    dtostrf(gyro[0], 8, 3, b); Serial.print(b); dtostrf(gyro[1], 8, 3, b); Serial.print(b); dtostrf(gyro[2], 8, 3, b); Serial.print(b);
    dtostrf(acceleration[0], 8, 3, b); Serial.print(b); dtostrf(acceleration[1], 8, 3, b); Serial.print(b); dtostrf(acceleration[2], 8, 3, b); Serial.print(b);
    dtostrf(magnetic[0], 8, 3, b); Serial.print(b); dtostrf(magnetic[1], 8, 3, b); Serial.print(b); dtostrf(magnetic[2], 8, 3, b); Serial.print(b);
    Serial.println();
}

/**
 * @param commandLine the line received from the host. Note that the line may not be for this object.
 */
void Imu::command(char *commandLine) {
    if (commandLine[0] != 'U') // Look for lines like 'U....' for IMU.
        return; // Not for us.
    if (commandLine[1] == 'R') // Report interval.
        setReportInterval(atoi(commandLine + 2));
}
