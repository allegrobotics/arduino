//-*- mode: c -*-
/* 
 * FILE
 *     ParkingSensor2.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 */

#include <Arduino.h>

#include "ParkingSensor2.h"
#include "Blinker.h"

#define STATE_TIME_THRESHOLD_US 400  /* 800us per bit, 266us per state, less than 400 is 1, more than 400 is 0 */

byte ParkingSensor2::pin;
byte ParkingSensor2::pinInterrupt;
char *ParkingSensor2::hex = "0123456789ABCDEF";

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
volatile uint32_t timeOfChange = 0; // Time of the previous HIGH to LOW state change.
int bitCounter = 0;                 // Num of bits read so far in this packet.
uint16_t packetBeingRead = 0;       // 16 bit packets, built up one bit at a time.

char parkingSensorHostPacket[7];    // "PsXX\r\n\0" s is sensor XX is distance in cm hex

extern Blinker blinker;

volatile uint16_t parkingSensor2Cubby;      // This is actually the 16-bit packet read from the sensor controller.
volatile byte parkingSensor2DataAvailable;  // 0 => no data to send, 1 => send data to host.

/**
 * @param pinInterrupt must be digitalPinToInterrupt(pin)
 */
ParkingSensor2::ParkingSensor2(byte pin, byte pinInterrupt) {
    this->pin = pin;
    this->pinInterrupt = pinInterrupt;
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
    attachInterrupt(pinInterrupt, risingEdge, RISING);
    if (pwmValue > 800) {          // This is one of the big end-frame or start-frame lows.
        if (bitCounter % 16 != 0)  // We should have finished a nibble before getting this.
            Serial.println("PX");  // Report framing error. Reader should ignore (or report) any line arriving with an X in it. Normally we wouldn't call Serial.println in the interrupt, but something has gone wrong anyway.
        //Serial.print("\n");        // This does LF, but no CR for efficiency. Minicom might require 'U' setting. Change to \r\n if desired.
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
            parkingSensor2Cubby = packetBeingRead;
            packetBeingRead = 0;
            parkingSensor2DataAvailable = (byte) 1;
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
    attachInterrupt(pinInterrupt, fallingEdge, FALLING);
}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void ParkingSensor2::setup() {
    //Serial.begin(19200);             // The Nano seems to be reliable at this speed. Must be done by .ino
    pinMode(pin, INPUT_PULLUP);
    // The 'correct' form of the attachInterrupt call:
    //attachInterrupt(digitalPinToInterrupt(PWM_READ_PIN), risingEdge, RISING);
    // .. but that fails for some reason (old .h libraries?), so for the Nano we just cheat and use this:
    attachInterrupt(pinInterrupt, risingEdge, RISING);
    parkingSensorHostPacket[0] = 'P';  // P header byte for packet.
    parkingSensorHostPacket[4] = '\r';
    parkingSensorHostPacket[5] = '\n';
    parkingSensorHostPacket[6] = '\0'; // Put null terminator in place for later.
    Serial.println("I ParkingSensor2 ready.");
}

/**
 * Called by the Arduino library continually after setup().
 */
void ParkingSensor2::loop(uint32_t now) {
    // Check if there is data to send to the host.
    if (parkingSensor2DataAvailable) {
        // The interrupt routine has dumped something in parkingSensor2Cubby for us.
        // We will write that to the host.
        register int packetRead = parkingSensor2Cubby; // Grab a copy ASAP
        parkingSensor2DataAvailable = 0;               // Free it up. We aren't guaranteed against overwrite by any stretch, but this will reduce the chances. Sort of.
        int whichSensor = (packetRead >> 8) & 0xf3;    // Removes fiddly bits which change depending on whether an object is detected or not.
        // Packet being read: 1B or 17 => E; 12 or 16 => H; 0B or 07 => A; 08 or 04 => B; 09 or 05 => C; 0A or 06 => D
        // A 00000111 00001011 & 0xF3 = 0x03
        // B 00000100 00001000 & 0xF3 = 0x00
        // C 00000101 00001001 & 0xF3 = 0x01
        // D 00000110 00001010 & 0xF3 = 0x02
        // E 00010111 00011011 & 0xF3 = 0x13
        // H 00010010 00010110 & 0xF3 = 0x12
        // Hmm .. it seems that 0x10 == MASK_BACK, 0x02 == MASK_CORNER, 0x01 == MASK_LEFT .. but are B and C swapped above?
        parkingSensorHostPacket[1] = whichSensor == 0x3 ? 'a'
            : whichSensor == 0x00 ? 'b'
            : whichSensor == 0x01 ? 'c'
            : whichSensor == 0x02 ? 'd'
            : whichSensor == 0x13 ? 'e'
            : whichSensor == 0x10 ? 'f' // Untested and unused (F&G swapped?)
            : whichSensor == 0x11 ? 'g' // Untested and unused (F&G swapped?)
            : whichSensor == 0x12 ? 'h'
            : 'X';
        parkingSensorHostPacket[2] = hex[(packetRead >> 4) & 0x0F]; // Distance hi nibble
        parkingSensorHostPacket[3] = hex[(packetRead >> 0) & 0x0F]; // Distance lo nibble
        Serial.print(parkingSensorHostPacket); // Will print both CR and LF, but it's easier that way.
        // Timing note: We don't know how long it will take to print these 4 (5 including \r?) chars (particularly at the slow speed of 19200baud,
        // This has been tested with a genuine Arduino Nano, and the cheap SumTingWong clones. Both appear to work fine.
        dataLastSentAt = now;
    }
    blinker.setBlinkPattern(now - dataLastSentAt < 1000 ? BLINK_21   // All okay
                            : dataLastSentAt == 0       ? BLINK_31   // Never sent data
                            :                             BLINK_32); // Stopped sending
}
