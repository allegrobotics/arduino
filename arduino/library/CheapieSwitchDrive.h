//-*- mode: c -*-
/* 
 * NAME
 *     CheapieSwitchDrive.h
 * PRECIS
 *     Controls the Sparkfun Robot kit (with the RPi controller replaced with Arduino nano motor controller).
 * THE VEHICLE
 * Left and right motors, driver by cheapie Chinese ripoff motor driver.
 * AUTHOR
 *     Copyright Live Software 2018-11-30 All Rights Reserved.
 *     IP Freely on non-commercial applications.
 * PINS
 * On cheapie Chinese ripoff motor driver the pins are pretty illogical:

 * LEFT_MOTOR_CURRENT_SENSE_PIN  A0
 * RIGHT_MOTOR_CURRENT_SENSE_PIN A1
 * LEFT_BRAKE_PIN                 9 // HIGH -> stop, LOW -> go
 * LEFT_DIRECTION_PIN            12
 * LEFT_SPEED_PIN                 3
 * RIGHT_BRAKE_PIN                8
 * RIGHT_DIRECTION_PIN           13
 * RIGHT_SPEED_PIN               11
 *
 */

#include <Arduino.h>

#define RIGHT_MOTOR_CURRENT_SENSE_PIN A0
#define LEFT_MOTOR_CURRENT_SENSE_PIN  A1
#define RIGHT_BRAKE_PIN                9 // HIGH -> stop, LOW -> go
#define RIGHT_DIRECTION_PIN           12 // forwards / backwards depending of wiring
#define RIGHT_SPEED_PIN                3 // HIGH -> go, LOW -> stop
#define LEFT_BRAKE_PIN                 8 // HIGH -> stop, LOW -> go
#define LEFT_DIRECTION_PIN            13 // forwards / backwards depending of wiring
#define LEFT_SPEED_PIN                11 // HIGH -> go, LOW -> stop

class CheapieSwitchDrive {
    
public:
    CheapieSwitchDrive();
    void setup();
    void loop(uint32_t now);
    void setMotorSpeed(int leftSpeed, int rightSpeed); // leftSpeed,rightSpeed :== [-255, 255]
    int leftMotorSpeed  = 0;
    int rightMotorSpeed = 0;
private:
};
