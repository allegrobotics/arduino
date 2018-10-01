//-*- mode: c -*-
/* 
 * NAME
 *     ParkingSensor2.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     Scott BARNES 2017/2018
 *     IP Freely on non-commercial applications.
 */

// Which board we are compiling for
// We will definitately need this if we are using the 'proper' attachInterrupt() calls, but we are cheating anyway.
//#define ARDUINO_NANO

#include <Arduino.h>

#include "ParkingSensor2.h"
#include "Blinker.h"

#define STATE_TIME_THRESHOLD_US 400    /* 800us per bit, 266us per state, less than 400 is 1, more than 400 is 0 */
#define PWM_READ_PIN                   2 /* Only pins 2 and 3 are capable of this function on the Nano */

char hex[] = "0123456789ABCDEF";

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
volatile uint32_t timeOfChange = 0; // Time of the previous HIGH to LOW state change.
volatile int bitCounter = 0;                // Num of bits read so far in this packet.
volatile unsigned int packetBeingRead = 0;   // 16 bit packets, built up one bit at a time.
volatile uint32_t dataLastSentAt = 0L;

extern Blinker blinker;

ParkingSensor2::ParkingSensor2() {
    this->pin = PWM_READ_PIN;
    dataLastSentAt = 0L;
}

ParkingSensor2::ParkingSensor2(int pin) {
    this->pin = pin;
    dataLastSentAt = 0L;
}

/**
 * Called as interrupt when falling edge detected.
 */
void ParkingSensor2::fallingEdge() {
    // Serial.print("F"); // DEBUGGING
    int pwmValue = micros() - timeOfChange; // How long the high signal lasted for.
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(PWM_READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, risingEdge, RISING);
    if (pwmValue > 800) {          // This is one of the big end-frame or start-frame lows.
        if (bitCounter % 16 != 0)  // We should have finished a nibble before getting this.
            Serial.print("X\n");   // Report framing error. Reader should ignore (or report) any line arriving with an X in it.
        Serial.print("\n");        // This does LF, but no CR for efficiency. Minicom might require 'U' setting. Change to \r\n if desired.
        bitCounter = 0;
        packetBeingRead = 0;       // Bits so-far get discarded if there is a framing error.
    } else {                       // This is a 'normal' bit (0 or 1).
        packetBeingRead >>= 1;
        // We have to make an arbitrary decision here about which is a '0' and which is a '1'.
        // We made this decision based on the hex looking nice and making slightly more intuitive sense.
        if (pwmValue < STATE_TIME_THRESHOLD_US) // We have read a '1'
            packetBeingRead |= 0x8000;
        bitCounter++;
        if (bitCounter % 16 == 0) {
            // Write out the packet to the host
            //Serial.print("O"); // DEBUGGING
            int whichSensor = (packetBeingRead >> 8) & 0xf3; // Removes fiddly bits which change depending on whether an object is detected or not.
            // Packet being read: 1B or 17 => E; 12 or 16 => H; 0B or 07 => A; 08 or 04 => B; 09 or 05 => C; 0A or 06 => D
            // A 00000111 00001011 & 0xF3 = 0x03
            // B 00000100 00001000 & 0xF3 = 0x00
            // C 00000101 00001001 & 0xF3 = 0x01
            // D 00000110 00001010 & 0xF3 = 0x02
            // E 00010111 00011011 & 0xF3 = 0x13
            // H 00010010 00010110 & 0xF3 = 0x12
            // Hmm .. it seems that 0x10 == MASK_BACK, 0x02 == MASK_CORNER, 0x01 == MASK_LEFT .. but are B and C swapped above?
            Serial.print(whichSensor == 0x3 ? "A"
                         : whichSensor == 0x00 ? "B"
                         : whichSensor == 0x01 ? "C"
                         : whichSensor == 0x02 ? "D"
                         : whichSensor == 0x13 ? "E"
                         : whichSensor == 0x10 ? "F" // Untested and unused (F&G swapped?)
                         : whichSensor == 0x11 ? "G" // Untested and unused (F&G swapped?)
                         : whichSensor == 0x12 ? "H"
                         : "?");
            Serial.print(hex[(packetBeingRead >> 4) & 0x0F]); // Distance hi nibble
            Serial.print(hex[(packetBeingRead >> 0) & 0x0F]); // Distance lo nibble
            // Timing note: We don't know how long these three Serial.print()s will take (particularly at the slow speed of 9600baud,
            // but we are assuming here that the Serial.print() call will return before we have to react to the next rising edge.
            // If it didn't, an alternatitive approach would be to put the answer into buffer, and print it from 'loop()', but
            // apparerently this is not necessary.
            // This has been tested with a genuine Arduino Nano, and the cheap SumTingWong clones. Both appear to work fine.
            dataLastSentAt = millis();
            packetBeingRead = 0;
        }
    }
}

/**
 * Called as interrupt when rising edge detected.
 */
void ParkingSensor2::risingEdge() {
    //Serial.print("R"); // DEBUGGING
    timeOfChange = micros();
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(PWM_READ_PIN), fallingEdge, FALLING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, fallingEdge, FALLING);
}

/**
 * Sets up the interrupt, and starts the Serial.
 */
void ParkingSensor2::setup() {
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(PWM_READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(0, risingEdge, RISING);
    Serial.begin(9600);             // The Nano seems to be reliable at this speed.
}

/**
 * Called by the Arduino library continually after setup().
 */
void ParkingSensor2::loop(uint32_t now) {
    blinker.setBlinkPattern(now - dataLastSentAt < 1000 ? BLINK_21   // All okay
                            : dataLastSentAt == 0       ? BLINK_31   // Never sent data
                            :                             BLINK_32); // Stopped sending
                            
}
