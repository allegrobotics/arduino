//-*- mode: c -*-
/**
 * FILE
 *     Blinker.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */
#include "Blinker.h"

#define NUM_BITS_IN_BLINK_MASK 32

#define LED_PIN 13

Blinker::Blinker(byte pin) {
    this->pin = pin;
    pinMode(pin, OUTPUT);
}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void Blinker::setup() {
    pattern = 0x00000501; // Startup pattern
    //pattern = 0x00001505; // Startup pattern
    blinkPosition = 0;
    Serial.println("I Blinker ready.");
}

void Blinker::setBlinkPattern(uint32_t blinkPattern) {
    pattern = blinkPattern;
}

void Blinker::loop(uint32_t now) {
    if (now >= nextBlinkAt) {
        blinkPosition = (blinkPosition + 1) % (sizeof(pattern) * 8);
        digitalWrite(pin, (pattern >> blinkPosition) & 0x01);
        nextBlinkAt = now + 90;
    }
}
