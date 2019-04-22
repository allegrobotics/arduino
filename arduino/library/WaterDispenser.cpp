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
    Serial.println("I WaterDispenser ready.");
}

/**
 * Turn relay/pump on or off.
 * @param mode 0=>OFF, otherwise ON
 * @param now approximately millis()
 */
void WaterDispenser::switchPump(byte mode, uint32_t now) {
    Serial.print("DI switch pump to mode "); Serial.println(mode);
    digitalWrite(pumpPin, mode ? LOW : HIGH); // Reversed from intuition - change if required.
    pumpStartedAt = mode ? now : 0;
    blinker.setBlinkPattern(mode ? BLINK_PATTERN_22 : BLINK_PATTERN_21);
    Serial.println(pumpStartedAt == 0 ? "DP0" : "DP1");
}

byte inputState = 0; // 0=> at start of line; 1=>just read D at start of line; 2=>must read to end of line

/**
 * Called by the Arduino library continually after setup().
 */
void WaterDispenser::loop(uint32_t now) {
    if (now > nextReportAt) {
        if (pumpStartedAt != 0 && now - pumpStartedAt > 2000 && pulseCount - lastReportedPulseCount < 10) { // (almost) no flow for 2s? Turn off pump and notify host.
            switchPump(0 , now);
            Serial.println("DP0");
            Serial.println("DI no flow stopping pump");
            blinker.setBlinkPattern(BLINK_PATTERN_31);
        }
        lastReportedPulseCount = pulseCount;
        Serial.print(digitalRead(floatPin) == LOW ? "DJ0" : "DJ1");
        Serial.print(digitalRead(hookPin) == LOW ? "0\r\nDK" : "1\r\nDK");
        Serial.println(lastReportedPulseCount);
        Serial.println(pumpStartedAt == 0 ? "DP0" : "DP1");
        nextReportAt = now + 500;
    }
    int c = 0;
    // BUG: We assume here that we are the only module reading from Serial. If not, write an on-Arduino exploder.
    if ((c = Serial.read()) != -1) {
        //Serial.print("DI Just read a "); Serial.println(c);
        if (c == '\n' || c == '\r') {
            inputState = 0;     // At start of line
            //Serial.println("DI read EoL");
        } else if (inputState == 0 && c == 'D') { // Read 'D' at start of line.
            inputState = 1;     // Next char should be a '0' or '1', to turn on or off pump.
            //Serial.println("DI read D");
        } else if (inputState == 1 && c == '0') {
            switchPump(0, now);
            inputState = 2; // must read to end-of-line
        } else if (inputState == 1 && c == '1') {
            switchPump(1, now);
            inputState = 2; // must read to end-of-line
        } else {
            inputState = 2; // must read to end-of-line
        }
    }
}
