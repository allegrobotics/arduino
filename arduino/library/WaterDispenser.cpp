//-*- mode: c -*-
/* 
 * FILE
 *     WaterDispenser.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 */

// CAVEAT: Assumes that Serial will be set up (ie Serial.start(NNN) will be called) before setup() is run.
// Serial.start(9600); is good, but Serial.start(19200); might be better, depending on what else is being run.

#include <Arduino.h>

#include "WaterDispenser.h"
#include "Blinker.h"

extern Blinker blinker;

volatile uint32_t WaterDispenser::pulseCount = 0;

/**
 * @param flowMeterPinInterrupt must be digitalPinToInterrupt(flowMeterPin)
 */
WaterDispenser::WaterDispenser(byte floatPin, byte pumpPin, byte flowMeterPin, byte flowMeterPinInterrupt, byte hookPin) {
    this->floatPin = floatPin;
    this->pumpPin = pumpPin;
    this->flowMeterPin = flowMeterPin;
    this->flowMeterPinInterrupt = flowMeterPinInterrupt;
    this->hookPin = hookPin;
}

/**
 * Called at every pulse-counter pulse by the interrupt.
 */
void WaterDispenser::pulseReceivedFromFlowMeter() {
    pulseCount++;
}

/**
 * Should be called by setup() in the .ino sketch.
 * PREREQUISITE: Serial.begin(...) must be called before this.
 */
void WaterDispenser::setup() {
    //Serial.begin(19200); must be done in .ino
    pinMode(floatPin, INPUT_PULLUP);     // We will read from float switch pin
    pinMode(pumpPin, OUTPUT);            // We will write to the pump relay pin
    digitalWrite(pumpPin, HIGH);         // Reversed from intuition - change if required
    pinMode(flowMeterPin, INPUT_PULLUP); // We will read from float switch pin
    attachInterrupt(flowMeterPinInterrupt, pulseReceivedFromFlowMeter, FALLING);
    Serial.println("WI WaterDispenser ready.");
}

/**
 * Turn relay/pump on or off.
 * @param mode 0=>OFF, otherwise ON
 * @param now approximately millis()
 */
void WaterDispenser::switchPump(byte mode, uint32_t now) {
    Serial.print("WI switch pump to mode "); Serial.println(mode);
    digitalWrite(pumpPin, mode ? LOW : HIGH); // Reversed from intuition - change if required.
    pumpStartedAt = mode ? now : 0;
    blinker.setBlinkPattern(mode ? BLINK_PATTERN_22 : BLINK_PATTERN_21);
    Serial.println(pumpStartedAt == 0 ? "WP0" : "WP1");
}

byte inputState = 0; // 0=> at start of line; 1=>just read D at start of line; 2=>must read to end of line

/**
 * Called by the Arduino library continually after setup().
 */
void WaterDispenser::loop(uint32_t now) {
    if (now > nextReportAt) {
        if (pumpStartedAt != 0 && now - pumpStartedAt > 2000 && pulseCount - lastReportedPulseCount < 10) { // (almost) no flow for 2s? Turn off pump and notify host.
            switchPump(0 , now);
            Serial.println("WP0");
            Serial.println("WI no flow stopping pump");
            blinker.setBlinkPattern(BLINK_PATTERN_31);
        }
        lastReportedPulseCount = pulseCount;
        Serial.print(digitalRead(floatPin) == LOW ? "WJ0" : "WJ1");
        Serial.print(digitalRead(hookPin) == LOW ? "0\r\nWK" : "1\r\nWK");
        Serial.println(lastReportedPulseCount);
        Serial.println(pumpStartedAt == 0 ? "WP0" : "WP1");
        nextReportAt = now + 500;
    }
}

void WaterDispenser::command(char *commandLine) {
    if (commandLine[0] != 'W') {
        return; // nothing to do with us.
    } else if (commandLine[0] = '0') {
        switchPump(0, millis());
    } else if (commandLine[0] = '1') {
        switchPump(1, millis());
    }
}
