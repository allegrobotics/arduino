//-*- mode: c -*-
/* 
 * NAME
 *     redbot.ino
 * PRECIS
 *     Controls the Sparkfun Robot kit (with the RPi controller replaced with Arduino Nano motor controller).
 * AUTHOR
 *     Copyright Live Software 2018-11-30 All Rights Reserved.
 *     IP Freely on non-commercial applications.
 */

#include <Arduino.h>
#include "CheapieSwitchDrive.h"
#include "SonarArray.h"

CheapieSwitchDrive  switchDrive;
SonarArray          sonarArray;

void setup() {
    delay(2000); // Let things settle.
    Serial.begin(19200);
    Serial.println("I RedBot starting");
    switchDrive.setup();
    sonarArray.setup();
    switchDrive.setMotorSpeed(0, 0);
    Serial.println("I RobBot ready");
}

void loop() {
    uint32_t now = millis();
    switchDrive.loop(now);
    sonarArray.loop(now);

    //Serial.print(d[0]); Serial.print("\t"); Serial.print(s1); Serial.print("\t"); Serial.print(s2); Serial.print("\t"); Serial.print(s3); Serial.print("\t"); Serial.print(s4); Serial.print("\t");  // Debugging

    // Go forward, Move ahead, Try to detect it, It's not to late ..

    int *d = sonarArray.sonarDistanceCm;
    if (d[0] >= 10 && d[0] <= 16 && d[1] > 30 && d[2] > 30) { // Wall hug
        Serial.print("HW");
        switchDrive.setMotorSpeed(150 - 7 * (d[0] - 13), 150 + 7 * (d[0] - 13));
    } else if (d[0] < 10 || d[1] < 30 || d[2] < 30) { // Swerve right
        Serial.print("SR");
        switchDrive.setMotorSpeed(200, 50);
    } else { // Curve right to try to pick up wall again.
        Serial.print("CR");
        switchDrive.setMotorSpeed(100, 200);
    }
    //Serial.print("\t"); Serial.print(switchDrive.leftMotorSpeed); Serial.print("\t"); Serial.println(switchDrive.rightMotorSpeed); // Debugging
}
