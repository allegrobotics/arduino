//-*- mode: c -*-
/* 
 * FILE
 *     ParkingSensor1.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017/2018/2019. IP freely on non-commercial applications.
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 */

#include <Arduino.h>

#include "ParkingSensor1.h"
#include "Blinker.h"

extern Blinker blinker;

/**
 * Volatile variables to be used in interrupt routines.
 * Of course these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
uint32_t ParkingSensor1::timeOfFall = 0;            // Time of the previous HIGH to LOW state change (usec).

char ParkingSensor1::lineToHost[37];                // A line to write to the host.
volatile byte ParkingSensor1::dataIsAvailable = 0;  // Set to true when we want loop() to write more data to the host.
volatile byte ParkingSensor1::errorNumber = 0;      // When non-zero, loop() will report an error instead of reporting distance.
byte ParkingSensor1::pinInterrupt;

char *ParkingSensor1::hex = "0123456789ABCDEF";
byte ParkingSensor1::numBitsRead;
byte ParkingSensor1::byteCount;
byte ParkingSensor1::byteBeingRead;

/**
 * @param pinInterrupt must be digitalPinToInterrupt(pin)
 */
ParkingSensor1::ParkingSensor1(byte pin, byte pinInterrupt) {
    this->pin = pin;
    this->pinInterrupt = pinInterrupt;
    dataLastSentAt = 0L;
}

/**
 * Called as interrupt when rising edge detected.
 */
void ParkingSensor1::risingEdge() {
    int pwmValue = micros() - timeOfFall;
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(pin), fallingEdge, FALLING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano and similar we use:
    attachInterrupt(pinInterrupt, fallingEdge, FALLING);
    if (pwmValue > 900) {           // This is one of the big end-frame or start-frame lows.
        if (numBitsRead % 8 != 0)   // We should have finished a byte before getting this.
            errorNumber = 1;        // '1' is framing error.
        byteCount = 0;
        /*
        if (pwmValue > 2000) {      // This is the BIG gap between frames.
            errorNumber = 2;        // Big gap between frames error .. thing.
        }
        */
        numBitsRead = 0;
        byteBeingRead = 0;        // Bits so-far get discarded if there is a framing error.
    } else {
        // This is a 'normal' bit (0 or 1).
        byteBeingRead <<= 1;
        // We have to make an arbitrary decision here about which is a '0' and which is a '1'.
        // We made this decision based on the hex looking nice and making slightly more intuitive sense.
        if (pwmValue < 150) // We have read a '1'
            byteBeingRead |= 1;
        numBitsRead++;
        if (numBitsRead % 8 == 0) { // We have completely read a byte.
            //Serial.print(byteBeingRead); // debugging
            if (byteBeingRead >= 0xF0) // FA is 'sensor not active', F0 is 'object not detected' - these are 'FF' in our coding scheme.
                byteBeingRead = 0xFF;
            else
                byteBeingRead *= 10; // dm to cm
            lineToHost[2 + byteCount * 6] = hex[(byteBeingRead >> 4) & 0x0f]; // Weird index to place hex in right place in line
            lineToHost[3 + byteCount * 6] = hex[byteBeingRead & 0x0f];  // Weird index to place hex in right place in line
            byteCount++;
            if (byteCount >= 6) {
                dataIsAvailable = 1; // Let the loop() know about this.
                byteCount = 0;
            }
            byteBeingRead = 0;
        }
    }
}

/**
 * Called as interrupt when falling edge detected.
 */
void ParkingSensor1::fallingEdge() {
    timeOfFall = micros();
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(pin), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(pinInterrupt, risingEdge, RISING);
}

/**
 * Sets up the interrupt, and packet structure.
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void ParkingSensor1::setup() {
    pinMode(pin, INPUT_PULLUP);
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(pin), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(pinInterrupt, risingEdge, RISING);
    //Serial.begin(19200);             // The Nano seems to be reliable at this speed. Must be done in .ino
    strcpy(lineToHost, "PaXX\r\nPbXX\r\nPcXX\r\nPdXX\r\nPeXX\r\nPhXX\r\n");
    Serial.print("I ParkingSensor1 ready.\n");
}

/**
 * Called by the Arduino library continually after setup().
 */
void ParkingSensor1::loop(uint32_t now) {
    if (errorNumber != 0) {
        Serial.print("PE\r\n"); // no need to check the error number - there is only one type of error - a framing error.
        errorNumber = 0; // We have reported it - clear the field.
    } else if (dataIsAvailable) {
        // We just assume here that we have been called soon enough to send the most recent data dumped here by the interrupt routine.
        // If we haven't, then we will get race conditions, but bad luck.
        dataIsAvailable = 0;
        Serial.print(lineToHost);
        dataLastSentAt = now;
    }
    blinker.setBlinkPattern(now - dataLastSentAt < 1000 ? BLINK_21   // All okay
                            : dataLastSentAt == 0       ? BLINK_31   // Never sent data
                            :                             BLINK_32); // Stopped sending
}
