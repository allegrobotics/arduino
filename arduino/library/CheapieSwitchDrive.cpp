//-*- mode: c -*-
/* 
 * NAME
 *     CheapieSwitchDrive.cpp
 * PRECIS
 *     PWM controller for the cheapie (with the RPi controller replaced with Arduino nano motor controller).
 * AUTHOR
 *     Copyright Live Software 2018-11-30 All Rights Reserved.
 *     IP Freely on non-commercial applications.
 * ALGORITHM
 *     Uses the PWM outputs on the digital pins to control the speed.
 */

#include "CheapieSwitchDrive.h"

CheapieSwitchDrive::CheapieSwitchDrive() {}

void CheapieSwitchDrive::setMotorSpeed(int leftSpeed, int rightSpeed) {
    this->leftMotorSpeed = leftSpeed > 255 ? 255 : leftSpeed < -255 ? -255 : leftSpeed;
    if (leftMotorSpeed == 0) {
        digitalWrite(LEFT_BRAKE_PIN, HIGH);
        digitalWrite(LEFT_SPEED_PIN, LOW);
    } else if (leftMotorSpeed > 0) {
        digitalWrite(LEFT_BRAKE_PIN, LOW);
        digitalWrite(LEFT_DIRECTION_PIN, leftMotorSpeed > 0 ? LOW : HIGH);
        analogWrite(LEFT_SPEED_PIN, leftMotorSpeed > 0 ? leftMotorSpeed : -leftMotorSpeed);
    }
    this->rightMotorSpeed = rightSpeed > 255 ? 255 : rightSpeed < -255 ? -255 : rightSpeed;
    if (rightMotorSpeed == 0) {
        digitalWrite(RIGHT_BRAKE_PIN, HIGH);
        digitalWrite(RIGHT_SPEED_PIN, LOW);
    } else if (rightMotorSpeed > 0) {
        digitalWrite(RIGHT_BRAKE_PIN, LOW);
        digitalWrite(RIGHT_DIRECTION_PIN, rightMotorSpeed > 0 ? LOW : HIGH);
        analogWrite(RIGHT_SPEED_PIN, rightMotorSpeed > 0 ? rightMotorSpeed : -rightMotorSpeed);
    }
}

void CheapieSwitchDrive::setup() {
    // Motor pins
    pinMode(LEFT_MOTOR_CURRENT_SENSE_PIN, INPUT);
    pinMode(RIGHT_MOTOR_CURRENT_SENSE_PIN, INPUT);
    pinMode(LEFT_BRAKE_PIN, OUTPUT);
    digitalWrite(LEFT_BRAKE_PIN, HIGH);
    pinMode(LEFT_DIRECTION_PIN, OUTPUT);
    pinMode(LEFT_SPEED_PIN, OUTPUT);
    digitalWrite(LEFT_SPEED_PIN, LOW);
    pinMode(RIGHT_BRAKE_PIN, OUTPUT);
    digitalWrite(RIGHT_BRAKE_PIN, HIGH);
    pinMode(RIGHT_DIRECTION_PIN, OUTPUT);
    pinMode(RIGHT_SPEED_PIN, OUTPUT);
    digitalWrite(RIGHT_SPEED_PIN, LOW);
}

void CheapieSwitchDrive::loop(uint32_t now) {
}
