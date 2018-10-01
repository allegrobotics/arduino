//-*- mode: c -*-
/**
 * FILE
 *     StepperMotor.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     2018
 */
#include "StepperMotor.h"

StepperMotor::StepperMotor(int stepPin, int directionPin, int enablePin, int sleepPin, int ms1Pin, int ms2Pin, int resetPin) {
    this->stepPin = stepPin;
    this->directionPin = directionPin;
    this->enablePin = enablePin;
    this->sleepPin = sleepPin;
    this->ms1Pin = ms1Pin;
    this->ms2Pin = ms2Pin;
    this->resetPin = resetPin;
    if (stepPin >= 0)
        pinMode(stepPin, OUTPUT);
    if (directionPin >= 0)
        pinMode(directionPin, OUTPUT);
    if (enablePin >= 0)
        pinMode(enablePin, OUTPUT);
    if (sleepPin >= 0)
        pinMode(sleepPin, OUTPUT);
    if (ms1Pin >= 0)
        pinMode(ms1Pin, OUTPUT);
    if (ms2Pin >= 0)
        pinMode(ms2Pin, OUTPUT);
    if (resetPin >= 0)
        pinMode(resetPin, OUTPUT);
    
    position = 0;
    speed = 0;
    stepIsHigh = 0;
}

void StepperMotor::dump() {
    Serial.print("D STP "); Serial.print(stepPin);
    Serial.print(" DIR "); Serial.print(directionPin);
    Serial.print(" ENB "); Serial.print(enablePin);
    Serial.print(" SLP "); Serial.print(sleepPin);
    Serial.print(" MS1 "); Serial.print(ms1Pin);
    Serial.print(" MS2 "); Serial.print(ms2Pin);
    Serial.print(" RST "); Serial.print(resetPin);
    Serial.print(" ");
}

void StepperMotor::setEnable(int enable) {
    if (enablePin >= 0)
        digitalWrite(enablePin, enable ? LOW : HIGH);
}

void StepperMotor::setSleep(int sleep) {
    if (sleepPin >= 0)
        digitalWrite(sleepPin, sleep ? LOW : HIGH);
}

void StepperMotor::setMs1(int ms1) {
    if (ms1Pin >= 0)
        digitalWrite(ms1Pin, ms1 ? HIGH : LOW);
}

void StepperMotor::setMs2(int ms2) {
    if (ms2Pin >= 0)
        digitalWrite(ms2Pin, ms2 ? HIGH : LOW);
}

void StepperMotor::setup() {
    if (enablePin >= 0)
        digitalWrite(enablePin, LOW);
    if (sleepPin >= 0)
        digitalWrite(sleepPin, HIGH);
    if (ms1Pin >= 0)
        digitalWrite(ms1Pin, HIGH);
    if (ms2Pin >= 0)
        digitalWrite(ms2Pin, HIGH);
}

/**
 * @param speed is the number of ms between pulses to the stepper motor. -ve is backwards. 0 is stopped.
 */
void StepperMotor::setSpeed(int speed) {
    this->speed = speed;
    if (directionPin >= 0)
        digitalWrite(directionPin, speed > 0 ? HIGH : LOW);
    nextStepAt = millis() + (speed > 0 ? speed : -speed);
    //dump();
}

void StepperMotor::resetPosition(int position) {
    this->position = position;
}

void StepperMotor::loop(uint32_t now) {
    if (stepIsHigh) {
        digitalWrite(stepPin, LOW);
        stepIsHigh = 0;
    } else if (speed != 0) {
        if (now >= nextStepAt) {
            digitalWrite(stepPin, HIGH);
            nextStepAt += (speed > 0 ? speed : -speed); // Note not "now +  ..." in case we fall behind :)
            position += speed > 0 ? 1 : -1;
            stepIsHigh = 1; // Will keep up the line for a short period of time - until the next loop() call.
        }
    }
}

/**
 * Only works if the MS1 and MS2 are connected (ie pins are >= 0)
 * @param steps how many pulses per step {0 => 1 pulsePerStep, 1 => 2 pulsesPerStep, 2 => 4 pulsesPerStep, 3 => 8 pulsesPerStep}
 */
void StepperMotor::setMicrostepping(int steps) {
    setMs1(steps & 0x01);
    setMs2(steps & 0x02);
}

