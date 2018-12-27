//-*- mode: c -*-
/**
 * FILE
 *     Bumper.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */
#include "Bumper.h"

#define BUMPER_REPORTING_INTERVAL_MS 1000

//uint32_t nextDigitalReadAt = 0L;

Bumper::Bumper(byte pin) {
    this->pin = pin;
}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this is.
 */
void Bumper::setup() {
    pinMode(pin, INPUT_PULLUP);
    reportedValue = digitalRead(pin);
    previousValue = reportedValue;
    Serial.println("I Bumper ready.");
}

void Bumper::report(byte value, uint32_t now) {
    Serial.println(value ? "Z00" : "Z01"); // N/C switch means we will get Z01 normally, Z00 is collision detected.
    reportedValue = value;
    nextReportAt = now + BUMPER_REPORTING_INTERVAL_MS;
}

void Bumper::loop(uint32_t now) {
    // Pseudo logic her to reduce jitter and bounce.
    // There is a weird jitter issue here, which only occurs on the rover and not in test.
    // This code looks weird, but seems to mitigate it.
    byte value = digitalRead(pin);
    if (value != previousValue)
        previousValue = value;
    else if (value != reportedValue)
        report(value, now);
    else if (now >= nextReportAt) {
        report(value, now);
    }
}
