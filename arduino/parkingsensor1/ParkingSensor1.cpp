//-*- mode: c -*-
/* 
 * NAME
 *     ParkingSensor1.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     Scott BARNES 2017
 *     IP Freely on non-commercial applications.
 */

// Which board we are compiling for
// We will definitely need this if we are using the 'proper' attachInterrupt() calls, but we are cheating anyway.
//#define ARDUINO_NANO

#include <Arduino.h>

#include "ParkingSensor1.h"
#include "Blinker.h"

#define READ_PIN                   2 /* Only pins 2 and 3 are capable of this function on the Nano */

//char hex[] = "0123456789ABCDEF";

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
volatile uint32_t timeOfFall = 0;        // Time of the previous HIGH to LOW state change (usec).
volatile int numBitsRead = 0;            // Num of bits read so far in this packet.
volatile byte nibbleBeingRead = 0;       // Nibbles are built up one bit at a time.
volatile uint32_t dataLastSentAt;

extern Blinker blinker;

ParkingSensor1::ParkingSensor1() {
    this->pin = READ_PIN;
    dataLastSentAt = 0L;
}

ParkingSensor1::ParkingSensor1(int pin) {
    this->pin = pin;
    dataLastSentAt = 0L;
}

/**
 * Called as interrupt when rising edge detected.
 */
void ParkingSensor1::risingEdge() {
    int pwmValue = micros() - timeOfFall;
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(READ_PIN), fallingEdge, FALLING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, fallingEdge, FALLING);
    if (pwmValue > 900) {      // This is one of the big end-frame or start-frame lows.
        if (numBitsRead % 4 != 0)  // We should have finished a nibble before getting this.
            Serial.println("X"); // Report framing error.
        if (pwmValue > 2000) {  // This is the BIG gap between frames.
            Serial.println();  // This does CR/LF. LF-only would be more efficient.
        }
        numBitsRead = 0;
        nibbleBeingRead = 0;   // Bits so-far get discarded if there is a framing error.
    } else {
        // This is a 'normal' bit (0 or 1).
        nibbleBeingRead <<= 1;
        // We have to make an arbitrary decision here about which is a '0' and which is a '1'.
        // We made this decision based on the hex looking nice and making slightly more intuitive sense.
        if (pwmValue < 150) // We have read a '1'
            nibbleBeingRead |= 1;
        numBitsRead++;
        if (numBitsRead % 4 == 0) {
            //Serial.print(hex[nibbleBeingRead]);
            Serial.print("0123456789ABCDEF"[nibbleBeingRead]);
            dataLastSentAt = millis();
            /*
            dataLastSentAt = millis();
            if (numBitsRead % 8 == 0) Serial.print(' '); // DEBUG  Improve readability during testing. Drop this in production for efficiency.
            */
            nibbleBeingRead = 0;
        }
    }
}

/**
 * Called as interrupt when falling edge detected.
 */
void ParkingSensor1::fallingEdge() {
    timeOfFall = micros();
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, risingEdge, RISING);
}

/**
 * Sets up the interrupt, and starts the Serial.
 */
void ParkingSensor1::setup() {
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, risingEdge, RISING);
    Serial.begin(9600);             // The Nano seems to be reliable at this speed.
    Serial.println("ParkingSensor1::setup() done");
}

/**
 * Called by the Arduino library continually after setup().
 */
void ParkingSensor1::loop(uint32_t now) {
    blinker.setBlinkPattern(now - dataLastSentAt < 1000 ? BLINK_21   // All okay
                            : dataLastSentAt == 0       ? BLINK_31   // Never sent data
                            :                             BLINK_32); // Stopped sending
                            
}
