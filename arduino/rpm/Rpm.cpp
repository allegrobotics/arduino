//-*- mode: c -*-
/* 
 * NAME
 *     Rpm.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     Scott BARNES 2017/2018
 *     IP Freely on non-commercial applications.
 */

// CAVEAT: Assumes that Serial will be set up (ie Serial.start(NNN) will be called) before setup() is run.
// Serial.start(9600); is good, but Serial.start(19200); might be good too, depending on what else is run.

// Which board we are compiling for
// We will definitely need this if we are using the 'proper' attachInterrupt() calls, but we are cheating anyway.
//#define ARDUINO_NANO

#include <Arduino.h>

#include "Rpm.h"
#include "Blinker.h"

#define REFRESH_INTERVAL_MS 100 // milliseconds between sensor updates
#define SPARKPLUG_READ_PIN    2 // Only pins 2 and 3 are capable of this function on the Nano

// Think of REACTION_TIME as the number of MS which we want to smooth over (like a half-life).
// 1 will be no smoothing at all (the bumpiest ride). 9999999 means totally smooth (but probably useless).
// REACTIONTIME of 200 ms is probably pretty good for picking up sudden drops in speed from hitting a big patch of high grass with a lawn-mower.
#define REACTION_TIME 200

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
volatile uint32_t timeOfChange = 0; // Time of the previous HIGH to LOW state change.
volatile int bitCounter = 0;                // Num of bits read so far in this packet.
volatile unsigned int packetBeingRead = 0;   // 16 bit packets, built up one bit at a time.
volatile uint32_t dataLastSentAt = 0L;

extern Blinker blinker;

unsigned long Rpm::previousMillis;
volatile int Rpm::smoothedRpm;

Rpm::Rpm() {
    this->pin = SPARKPLUG_READ_PIN;
    dataLastSentAt = 0L;
}

Rpm::Rpm(int pin) {
    this->pin = pin;
    dataLastSentAt = 0L;
}

/**
 * Called every spark.
 * This must complete comfortably in around 10ms (ie every revolution at 6000 RPM), so we don't use floating points, trig functions, searches, internet downloads from North Korea ..
 */
void Rpm::pulseReceived() {
    /*
      ALGORITHM:
      each time we are called, we know the time between this (current) pulse, and the previous one (call this deltaT ms), and
      currentRpm = 60000 / deltaT
      The usual formula for expontial smoothing would be
      smoothedRpm = ALPHA * currentRpm + (1 - ALPHA) * smoothedRpm; // Where "0 < ALPHA <= 1" and 1 is totally bumpy.
      expressing "ALPHA = 1 / REACTION_TIME
      smoothedRpm = currentRpm / REACTION_TIME + (1 - 1 / REACTION_TIME) * smoothedRpm; // Where "0 < ALPHA <= 1" and 1 is totally bumpy. infinity is totally smooth, but useless.
      however in this case the currentRpm has been the current value for (60000 / deltaT) ms.
      so the alpha value should be multiplied by this, hence (strictly we sould multiply out by each ms, but we want to do it efficiently, we grossly approximate "X^Y = 1 + X * (Y - 1)" for Y slightly bigger than 1.
      smoothedRpm = (deltaT / REACTION_TIME) * currentRpm + (1 - deltaT / REACTION_TIME) * smoothedRpm; // Assuming "ALPHA * deltaT < 1"
      And if deltaT is greater than reactionTime we just assign "smoothedRpm = currentRpm
      We want to avoid floating point operations, so we use:
      1024 * smoothedRpm = (1024 * deltaT / REACTION_TIME) * currentRpm + (1024 - 1024 * deltaT / REACTION_TIME) * smoothedRpm;
      ie
      smoothedRpm = ((1024 * deltaT / REACTION_TIME) * currentRpm + (1024 - 1024 * deltaT / REACTION_TIME) * smoothedRpm) / 1024;
      or 
      smoothedRpm = ((deltaT << 10 / REACTION_TIME) * currentRpm + (1024 - (deltaT << 10) / REACTION_TIME) * smoothedRpm) >> 10;
      Which should be quite fast to compute even on a cheapie Arduino such as a Nano.
     */
    //    rpmPulses++;
    unsigned long nowMillis = millis();
    int deltaT = nowMillis - previousMillis;
    int currentRpm = 60000 / deltaT;
    if (deltaT >= REACTION_TIME)
        smoothedRpm = currentRpm;
    else {
        long deltaT10 = (long) deltaT << 10;
        smoothedRpm = (int) ((deltaT10 / REACTION_TIME) * currentRpm + ((1024 - deltaT10 / REACTION_TIME) * smoothedRpm)) >> 10;
    }
    previousMillis = nowMillis;
}

/**
 * Called every _refreshIntervalMs_ from loop().
 * Sends the current (smoothed) RPM out via serial as a single byte which is (rpm << 5),
 * which will a range of 0 .. 8160 RPM, and a resolution of 32 RPM.
 * Good enough.
 * This will give a resolution of around 3% at 1000 RPM (a realistic idle speed),
 * but any reaction (eg "are we slowing down?") should be based on a movement at least twice this.
 */
void Rpm::sendRpmViaSerialPort() {
    if (smoothedRpm > 8160)
        Serial.print((byte) 0xFF); // Shouldn't happen, they really don't go this fast.
    else
        Serial.print((byte) (smoothedRpm << 5));
}

/**
 * Sets up the interrupt, and starts the Serial.
 */
void Rpm::setup() {
    pinMode(pin, INPUT); // enable internal pullup for the pin
    attachInterrupt(1, pulseReceived, FALLING);
    previousMillis = millis();
    Serial.begin(9600);
}

/**
 * Called by the Arduino library continually after setup().
 */
void Rpm::loop(uint32_t now) {
    // Loop around and every _refreshIntervalMs_, send the current count via RPM.
    if (now - previousMillis > REFRESH_INTERVAL_MS) {
        sendRpmViaSerialPort();
        previousMillis = now;
    }
}
