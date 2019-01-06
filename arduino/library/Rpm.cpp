//-*- mode: c -*-
/* 
 * FILE
 *     Rpm.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 */

// CAVEAT: Assumes that Serial will be set up (ie Serial.start(NNN) will be called) before setup() is run.
// Serial.start(9600); is good, but Serial.start(19200); might be good too, depending on what else is being run.

#include <Arduino.h>

#include "Rpm.h"
#include "Blinker.h"

#define REFRESH_INTERVAL_MS 100 // milliseconds between sensor updates

// Think of REACTION_TIME_MS as the number of MS which we want to smooth over.
// REACTIONTIME of 200 ms is probably pretty good for picking up sudden drops in speed from hitting a big patch of high grass with a lawn-mower.
#define REACTION_TIME_MS 200

/**
 * Volatile variables to be used in interrupt routines.
 * Note that these mean that we can only have one ParkingSensor active because they will scribble over each other.
 */
//volatile uint32_t timeOfChange = 0;        // Time of the previous HIGH to LOW state change.
//volatile int bitCounter = 0;               // Num of bits read so far in this packet.
//volatile uint32_t dataLastSentAt = 0L;

extern Blinker blinker;

volatile uint32_t Rpm::lastSparkAt;
volatile int Rpm::smoothedRpm;

volatile int deltaTMs = 0;

/*
 * @param pinInterrupt must be digitalPinToInterrupt(pin)
 */
Rpm::Rpm(byte pin, byte pinInterrupt) {
    this->pin = pin;
    this->pinInterrupt = pinInterrupt;
    //dataLastSentAt = 0L;
}

volatile int theDelta = 0L;

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
      We are interested in a pseudo-average of the RPM over REACTION_TIME_MS.
      Hence we approximate:
         smoothedRpm = 60000 / REACTION_TIME_MS + ((REACTION_TIME_MS - deltaT) * smoothedRpm) / REACTION_TIME_MS;
      Which should be quite fast to compute even on a cheapie Arduino such as a Nano.
     */
    unsigned long now = millis();
    int delta = now - lastSparkAt;
    if (delta < 6)
        return; // We may get multiple triggers from a single spark. They seem to echo for around 5ms. With a 6m 'debounce', we can't measure RPM faster than 60000/6 == 10000 rpm. Suffer.
    deltaTMs = delta;
    int currentRpm = 60000 / deltaTMs;
    if (deltaTMs >= REACTION_TIME_MS) {
        //smoothedRpm = 60000 / deltaTMs;
        smoothedRpm = 60000 / deltaTMs;
        //smoothedRpm = currentRpm;
    } else {
        smoothedRpm = 60000 / REACTION_TIME_MS + ((REACTION_TIME_MS - (long) deltaTMs) * (long) smoothedRpm) / REACTION_TIME_MS;
        /*
        long deltaTMs10 = (long) deltaTMs << 10;
        smoothedRpm = (int) ((deltaTMs10 / REACTION_TIME) * currentRpm + ((1024 - deltaTMs10 / REACTION_TIME) * smoothedRpm)) >> 10;
        */
    }
    lastSparkAt = now;
}

char *Rpm::hex = "0123456789ABCDEF";

char rpmPacket[7]; // "Rxxx\r\n\0" xxx is hex of RPM up to 8192, and 'FFF' thereafter.

/**
 * Called every _refreshIntervalMs_ from loop().
 * Sends the current (smoothed) RPM out via serial as a single byte (2 hex nibbles) which is (rpm << 5),
 * which will a range of 0 .. 8160 RPM, and a resolution of 32 RPM.
 * Good enough.
 * This will give a resolution of around 3% at 1000 RPM (a realistic idle speed),
 * but any reaction (eg "are we slowing down?") should be based on a movement at least twice this.
 */
void Rpm::sendRpmViaSerialPort(uint32_t now) {
    int rpm = smoothedRpm;
    if (now - lastSparkAt > REFRESH_INTERVAL_MS) {
        // Awkward. We haven't actually seen a spark since last report.
        // Maybe we have stopped, or at least slowed down considerably.
        rpm = 30000 / (now - lastSparkAt); // Assume next spark will be twice as long as we have been waiting. Bear in mind we will report rpm=zero at rpm=32, so we will detect stall at about 1s. Good enough for now.
    }
    if (rpm > 8191) { // Shouldn't happen, they really don't go this fast.
        rpmPacket[1] = 'F';
        rpmPacket[2] = 'F';
        rpmPacket[3] = 'F';
    } else {
        rpmPacket[1] = hex[(rpm >> 8) & 0x0f];
        rpmPacket[2] = hex[(rpm >> 4) & 0x0f];
        rpmPacket[3] = hex[(rpm >> 0) & 0x0f];
    }
    Serial.print(rpmPacket);
    //Serial.print(deltaTMs); Serial.print(" "); Serial.println(rpm); // Debugging only
}

/**
 * Sets up the interrupt, and packet structure.
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void Rpm::setup() {
    pinMode(pin, INPUT_PULLUP); // Read from spark-plug pin
    //attachInterrupt(digitalPinToInterupt(pin), pulseReceived, FALLING);
    attachInterrupt(pinInterrupt, pulseReceived, FALLING);
    nextReportAt = millis();
    //Serial.begin(19200); must be done in .ino
    rpmPacket[0] = 'R';
    rpmPacket[4] ='\r';
    rpmPacket[5] ='\n';
    rpmPacket[6] ='\0';
    Serial.println("I Rpm ready.");
}

/**
 * Called by the Arduino library continually after setup().
 */
void Rpm::loop(uint32_t now) {
    // Loop around and every _refreshIntervalMs_, send the current count via RPM.
    if (now >= nextReportAt) {
        sendRpmViaSerialPort(now);
        nextReportAt = now + REFRESH_INTERVAL_MS;
    }
}
