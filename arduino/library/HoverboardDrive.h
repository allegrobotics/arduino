//-*- mode: c -*-
/* 
 * NAME
 *     HoverboardDrive
 * PURPOSE
 *     Control two ZS-X11A BLDC motor drives for hoverboard motors.
 * OUTPUT TO HOST
 *     None
 * PHILOSOPHY
 *     Power not speed. Speed is a Helm concept.
 *     Later, however we will measure distance travelled via the six Hall sensors.
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 */

#ifndef HoverboardDrive_h
#define HoverboardDrive_h

#include <Arduino.h>
#include "DifferentialDrive.h"

class HoverboardDrive : public DifferentialDrive {
 private:
    byte leftMotorSpeedPin;
    byte leftMotorDirectionPin;
    byte rightMotorSpeedPin;
    byte rightMotorDirectionPin;
    byte hallLeftMotorAPin;
    byte hallLeftMotorBPin;
    byte hallLeftMotorCPin;
    byte hallRightMotorAPin;
    byte hallRightMotorBPin;
    byte hallRightMotorCPin;
    uint32_t nextReportAt = 0L;
public:
    HoverboardDrive(byte reverseLeftMotor, byte reverseRightMotor, byte leftMotorSpeedPin, byte leftMotorDirectionPin, byte rightMotorSpeedPin, byte rightMotorDirectionPin, byte hallLeftMotorAPin, byte hallLeftMotorBPin, byte hallLeftMotorCPin, byte hallRightMotorAPin, byte hallRightMotorBPin, byte hallRightMotorCPin);
    virtual void setup();
    virtual void loop(uint64_t now);
    virtual void setMotorPowers(int leftMotorPower, int rightMotorPower); // percent [-100 .. +100] -v is reverse.
    virtual void command(char *commandLine);
    virtual void report();
    virtual void resetMotors();
};

#endif /* HoverboardDrive_h */
