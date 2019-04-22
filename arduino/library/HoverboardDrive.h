//-*- mode: c -*-
/* 
 * NAME
 *     HoverboardDrive
 * PURPOSE
 *     Control two ZS-X11A BLDC motor drives for hoverboard motors.
 * PROTOCOL FROM HOST
 *     Normally none, if this is controlled by a Helm, but
 *     "SZ" reset motors (hold pins down for 1s)
 *     "SRnnn" set reporting interval to every nnn ms (reporting interval of 0 turns off reporting).
 *     "SPnm" set power to left and right motors to n m respectively (n and m are 5 stop, 1-4 backwards, 6-9 forwards, eg SP46 spins slowing ACW)
 * PROTOCOL TO HOST
 *     Nothing by default.
 *     "SPnnn mmm" periodically if reporting power. nnn and mmm are left and right motor powers respectively (variable width fields)
 * PHILOSOPHY
 *     Power not speed. Speed is a Helm concept.
 *     Later, however we will measure distance travelled via the six Hall sensors.
 * AUTHOR
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 */

#ifndef HoverboardDrive_h
#define HoverboardDrive_h

#include <Arduino.h>
#include "King.h"
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
    virtual void loop(uint32_t now);
    virtual void command(char *commandLine);
    virtual void setMotorPowers(int leftMotorPower, int rightMotorPower); // percent [-100 .. +100] -v is reverse.
    virtual void report();
    virtual void resetMotors();
};

#endif /* HoverboardDrive_h */
