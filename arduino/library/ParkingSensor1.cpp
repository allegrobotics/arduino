//-*- mode: c -*-
/* 
 * NAME
 *     ParkingSensor1.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "ParkingSensor1.h"
#include "Blinker.h"

#define READ_PIN                   2 /* Only pins 2 and 3 are capable of this function on the Nano */

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
volatile uint32_t timeOfFall = 0; // Time of the previous HIGH to LOW state change (usec).
byte numBitsRead = 0;             // Num of bits read so far in this packet.
byte nibbleBeingRead = 0;         // Nibbles are built up one bit at a time.
byte nibbleCount = 0;             // 0..5

char lineToHost[37];              // A line to write host.
volatile byte dataAvailable = 0;  // Set to true when we want loop() to write more data to the host.

extern Blinker blinker;

/*
ParkingSensor1::ParkingSensor1() {
    this->pin = READ_PIN;
    dataLastSentAt = 0L;
}
*/

ParkingSensor1::ParkingSensor1(byte pin) {
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
    if (pwmValue > 900) {           // This is one of the big end-frame or start-frame lows.
        if (numBitsRead % 4 != 0)   // We should have finished a nibble before getting this.
            Serial.println("PE1");  // Report framing error. Normally we wouldn't call Serial.println in the interrupt, but something has gone wrong anyway.
        nibbleCount = 0;
        if (pwmValue > 2000) {      // This is the BIG gap between frames.
            Serial.println("PE2");  // This does CR/LF. LF-only would be more efficient.
        }
        numBitsRead = 0;
        nibbleBeingRead = 0;        // Bits so-far get discarded if there is a framing error.
    } else {
        // This is a 'normal' bit (0 or 1).
        nibbleBeingRead <<= 1;
        // We have to make an arbitrary decision here about which is a '0' and which is a '1'.
        // We made this decision based on the hex looking nice and making slightly more intuitive sense.
        if (pwmValue < 150) // We have read a '1'
            nibbleBeingRead |= 1;
        numBitsRead++;
        if (numBitsRead % 4 == 0) { // We have completely read a nibble.
            //Serial.print(hex[nibbleBeingRead]);
            lineToHost[(1 + (nibbleCount >> 1) * 6) + (nibbleCount & 0x1)] = "0123456789ABCDEF"[nibbleBeingRead]; // Weird index to place hex in right place in line
            nibbleCount++;
            if (nibbleCount >= 12) {
                dataAvailable = 1; // Let the loop() know about this.
                nibbleCount = 0;
            }
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
 * Sets up the interrupt, and packet structure.
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void ParkingSensor1::setup() {
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, risingEdge, RISING);
    //Serial.begin(19200);             // The Nano seems to be reliable at this speed. Must be done in .ino
    strcpy(lineToHost, "PaXX\r\nPbXX\r\PcXX\r\nPdXX\r\nPeXX\r\nPhXX\r\n");
    Serial.println("I ParkingSensor1 ready.");
}

/**
 * Called by the Arduino library continually after setup().
 */
void ParkingSensor1::loop(uint32_t now) {
    if (dataAvailable) {
        // We just assume here that we have been called soon enough to send the most recent data dumped here by the interrupt routine.
        // If we haven't, then we will get race conditions.
        dataAvailable = 0;
        Serial.print(lineToHost);
        dataLastSentAt = now;
    }
    blinker.setBlinkPattern(now - dataLastSentAt < 1000 ? BLINK_21   // All okay
                            : dataLastSentAt == 0       ? BLINK_31   // Never sent data
                            :                             BLINK_32); // Stopped sending
}
