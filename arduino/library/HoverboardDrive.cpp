//-*- mode: c -*-
/**
 * FILE
 *     HoverboardDrive.cpp
 * PURPOSE
 *     A proxy to talk to dual Hoverboard motor drivers - ZS-X11As or ZS-X11Bs.
 * AUTHOR
 *     Scott Barnes
 * PHILOSOPHY
 *     The Hall effect sensors can be 'tapped' by the Arduino to give D (as in PID) feedback to the Helm. Later.
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * HARDWARE INTERFACE
 *     Assumes each side has a PWM pin (for speed) and a direction pin.
 *     This could be used on many kind of motor drivers.
 *     But the hall sensor interface might not be there.
 */

#include "HoverboardDrive.h"

//#define LEFT_MOTOR_PWM_PIN         3
//#define LEFT_MOTOR_DIRECTION_PIN   4
//#define RIGHT_MOTOR_PWM_PIN        5
//#define RIGHT_MOTOR_DIRECTION_PIN  6

//#define LEFT_MOTOR_HA_PIN          7 // Hall effect left motor pin A
//#define LEFT_MOTOR_HB_PIN          8 // Hall effect left motor pin B
//#define LEFT_MOTOR_HC_PIN          9 // Hall effect left motor pin C
//#define RIGHT_MOTOR_HA_PIN        10 // Hall effect right motor pin A
//#define RIGHT_MOTOR_HB_PIN        11 // Hall effect right motor pin B
//#define RIGHT_MOTOR_HC_PIN        12 // Hall effect right motor pin C

/**
 * @param leftMotorSpeedPin PWM pin which controls motor speed (actually motor power, but it's called a speed pin).
 * @param leftMotorDirectionPin pin which controls motor direction.
 * @param rightMotorSpeedPin  PWM pin which controls motor speed (actually motor power, but it's called a speed pin).
 * @param rightMotorDirectionPin pin which controls motor direction.
 * @param hallLeftMotorAPin not yet used.
 * @param hallLeftMotorBPin not yet used.
 * @param hallLeftMotorCPin not yet used.
 * @param hallRightMotorAPin not yet used.
 * @param hallRightMotorBPin not yet used.
 * @param hallRightMotorCPin not yet used.
 */
HoverboardDrive::HoverboardDrive(byte reverseLeftMotor, byte reverseRightMotor, byte leftMotorSpeedPin, byte leftMotorDirectionPin, byte rightMotorSpeedPin, byte rightMotorDirectionPin, byte hallLeftMotorAPin, byte hallLeftMotorBPin, byte hallLeftMotorCPin, byte hallRightMotorAPin, byte hallRightMotorBPin, byte hallRightMotorCPin) : DifferentialDrive(reverseLeftMotor, reverseRightMotor) {
    this->leftMotorSpeedPin = leftMotorSpeedPin;
    this->leftMotorDirectionPin = leftMotorDirectionPin;
    this->rightMotorSpeedPin = rightMotorSpeedPin;
    this->rightMotorDirectionPin = rightMotorDirectionPin;
    this->hallLeftMotorAPin = hallLeftMotorAPin;
    this->hallLeftMotorBPin = hallLeftMotorBPin;
    this->hallLeftMotorCPin = hallLeftMotorCPin;
    this->hallRightMotorAPin = hallRightMotorAPin;
    this->hallRightMotorBPin = hallRightMotorBPin;
    this->hallRightMotorCPin = hallRightMotorCPin;
    pinMode(leftMotorSpeedPin, OUTPUT);
    pinMode(leftMotorDirectionPin, OUTPUT);
    pinMode(rightMotorSpeedPin, OUTPUT);
    pinMode(rightMotorDirectionPin, OUTPUT);
    pinMode(hallLeftMotorAPin, INPUT);
    pinMode(hallLeftMotorBPin, INPUT);
    pinMode(hallLeftMotorCPin, INPUT);
    pinMode(hallRightMotorAPin, INPUT);
    pinMode(hallRightMotorBPin, INPUT);
    pinMode(hallRightMotorCPin, INPUT);
    //delay(1000); Nope. Don't put delay()s in constructors. Arduino freezes.
}

/**
 * Call this from Arduino setup()
 */
void HoverboardDrive::setup() {
    // Nothing to do here .. well .. except.
    resetMotors();
}

void HoverboardDrive::resetMotors() {
    // There seems to be an issue initialising the BLDC motor drivers.
    // To overcome this, pull them low for 1s before starting.
    // Hopefully this will do some kind of reset on the drivers .. or something.
    digitalWrite(leftMotorSpeedPin, LOW);
    analogWrite(rightMotorSpeedPin, 0);
    digitalWrite(leftMotorDirectionPin, LOW);
    analogWrite(rightMotorDirectionPin, 0);
    delay(1000); // This is actually to let the resetting the BLDC reset take effect. Not really clear whether it's needed, but WTH.
}

/**
 * Call this from Arduino loop()
 */
void HoverboardDrive::loop(uint64_t now) {
    // Nothing to do here except (possibly) some debug reporting
    if (now < nextReportAt || reportIntervalMs <= 0) // No reporting
        return;
    report();
    nextReportAt = now + reportIntervalMs;
}

/**
 * Note we are setting power, not speed.
 * We don't actually control speed, just power.
 * the F(speed) -> power calculation is in the Helm.
 * @param leftMotorPower      -100 .. +100
 * @param rightMotorPower     -100 .. +100
 */
void HoverboardDrive::setMotorPowers(int leftMotorPower, int rightMotorPower) {
    if (leftMotorPower != currentLeftMotorPower) {
        digitalWrite(leftMotorDirectionPin, reverseLeftMotor ? leftMotorPower < 0 : leftMotorPower >= 0);
        analogWrite(leftMotorSpeedPin, leftMotorPower > 0 ? (255 * leftMotorPower / 100) : (255 * leftMotorPower / -100));
        currentLeftMotorPower = leftMotorPower;
    }
    if (rightMotorPower != currentRightMotorPower) {
        digitalWrite(rightMotorDirectionPin, reverseRightMotor ? rightMotorPower < 0 : rightMotorPower >= 0);
        analogWrite(rightMotorSpeedPin, rightMotorPower > 0 ? (255 * rightMotorPower / 100) : (255 * rightMotorPower / -100));
        currentRightMotorPower = rightMotorPower;
    }
}

/**
 * SRnnn (debugging) report interval in ms
 * SP stop both motors.
 * SPlr set power of l (left) r (right) values 0..9, 5 is stationary.
 * 
 * This is really just for debugging.
 * It is unlikely to be used in production, as the host would send commands to the Helm, not to this object.
 */
void HoverboardDrive::command(char *commandLine) {
    if (commandLine[0] != 'S')
        return; // not for us
    if (commandLine[1] == 'Z') {
        resetMotors();
    } else if (commandLine[1] == 'R') {
        reportIntervalMs = atoi(commandLine + 2);
        Serial.print("SD HoverboardDrive.reportIntervalMs now "); Serial.println(reportIntervalMs);
    } else if (commandLine[1] == 'P') {                                      // SP[0-9][0-9] set speeds left and right
        int left = 0;
        int right = 0;
        if (commandLine[2] != '\0' && commandLine[3] != '\0') {
            left  = min(100, max(-100, (commandLine[2] - '5') * 20));
            right = min(100, max(-100, (commandLine[3] - '5') * 20));
        }
        setMotorPowers(left, right);
        report();
    }
}

void HoverboardDrive::report() {
    // Report on current speeds.
    Serial.print("SP"); Serial.print(currentLeftMotorPower); Serial.print(" "); Serial.println(currentRightMotorPower);
}
