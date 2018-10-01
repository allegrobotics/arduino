//-*- mode: c -*-
/**
 * FILE
 *     Blinker.cpp
 * AUTHOR
 *     Scott Barnes
 * COPYRIGHT
 *     2018
 */
#include "Blinker.h"

#define NUM_BITS_IN_BLINK_MASK 32

#define LED_PIN 13

Blinker::Blinker(int ledPin) {
    this->ledPin = ledPin;
}

void Blinker::setup() {
    pattern = 0x00000501; // Startup pattern
    //pattern = 0x00001505; // Startup pattern
    blinkPosition = 0;
}

void Blinker::setBlinkPattern(uint32_t blinkPattern) {
    pattern = blinkPattern;
}

void Blinker::loop(uint32_t now) {
    if (now >= nextBlinkAt) {
        blinkPosition = (blinkPosition + 1) % (sizeof(pattern) * 8);
        digitalWrite(ledPin, (pattern >> blinkPosition) & 0x01);
        nextBlinkAt = now + 75;
    }
}
