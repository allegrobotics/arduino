//-*- mode: c -*-
/* 
 * NAME
 *     StepperMotor
 * PURPOSE
 *     A little module to control a stepper motor controller such as the BIG EASY DRIVER. Or even the (not so big) EASY DRIVER.
 * AUTHOR
 *     Scott BARNES 2016/2017/2018. IP freely on non-commercial applications.
 * PROTOCOL FROM HOST
 *     None
 * PROTOCOL TO HOST
 *     None
 * EASYDRIVER PINS
 * Easy driver has A1 A2 B1 B2 PFD RST EN MS2 GND M+ GND +5V SLP MS1 GND STEP DIR
 * MOTORS: A1 A2 B1 B2
 * UNUSED: PFD
 * POWER: GND M+
 * LOGIC: RST EN MS2 SLP MS1 STEP DIR (+GND to Arduino)
 * So, logic pins are
 *    STEP == commandPulsePin
 *    DIR  == directionPin
 *    EN   == enablePin
 *    SLP  == sleepPin
 *    MS1  == ms1Pin
 *    MS2  == ms2Pin
 *    RST  == resetPin
 *    (GND == groundPin)
 * 
 * And some other stuff about step rates which I have forgotten.
 * PHILOSOPHY
 *     We use the 'loop()' mechanism instead of a customer interrupt-timer.
 *     Unfortunately this means that the smallest resolution is 1ms, which creates issues with the microstepping.
 *     This might change later.
 */

#ifndef StepperMotor_h
#define StepperMotor_h

#include <Arduino.h>
#include "King.h"

class StepperMotor : public King {
public:
    int position;    // Position (ie angle (possibly in 200ths of a rotation == 1.8 degrees)).
    int speed;       // milliseconds between pulses.

    StepperMotor(int stepPin, int directionPin = -1, int enablePin = -1, int sleepPin = -1, int ms1Pin = -1, int ms2Pin = -1, int resetPin = -1);

    /**
     * @param speed milliseconds between pulses (note if microstepping, then there will be multiple pulses per step).
     */
    void setSpeed(int speed);

    /**
     * Does initialization.
     */
    void setup();

    /**
     * Called often.
     */
    void loop(uint32_t now);

    /**
     * 
     */
    void command(char *commandLine) {};

    /**
     * Set the known position, of the turret.
     * This could be called when the position detector is triggered.
     */
    void resetPosition(int position);

    void setEnable(int enable);

    void setSleep(int sleep);

    void setMicrostepping(int steps);

private:
    int stepPin;         // Step pin on the EasyDriver
    int directionPin;    // Direction pin on the EasyDriver
    int enablePin;       // Enable pin on the EasyDriver
    int sleepPin;        // Sleep pin on the EasyDriver
    int ms1Pin;          // MS1 pin on the EasyDriver
    int ms2Pin;          // MS2 pin on the EasyDriver
    int resetPin;        // Reset pin on the EasyDriver
    uint32_t nextStepAt; // millis() must return at least this before we do the next step
    uint8_t stepIsHigh;  // Hold this high for one loop() just to trigger the EasyDriver
    void dump();         // Debugging
};

#endif /* StepperMotor_h */
