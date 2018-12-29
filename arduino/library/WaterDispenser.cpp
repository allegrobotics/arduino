//-*- mode: c -*-
/* 
 * NAME
 *     WaterDispenser.cpp
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 */

// CAVEAT: Assumes that Serial will be set up (ie Serial.start(NNN) will be called) before setup() is run.
// Serial.start(9600); is good, but Serial.start(19200); might be better, depending on what else is being run.

// Which board we are compiling for
// We will definitely need this if we are using the 'proper' attachInterrupt() calls, but we are cheating anyway.
//#define ARDUINO_NANO

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
 * Called every spark.
 */
void WaterDispenser::pulseReceivedFromFlowMeter() {
    pulseCount++;
}

/**
 * Sets up the interrupt.
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
    //Serial.println("DI switch pump");
    digitalWrite(pumpPin, mode ? LOW : HIGH); // Reversed from intuition - change if required.
    pumpStartedAt = mode ? now : 0;
    blinker.setBlinkPattern(mode ? BLINK_22 : BLINK_21);
}

byte inputState = 0; // 0=>idle 1=>just read new line 2=>just read 'P' after new line

/**
 * Called by the Arduino library continually after setup().
 */
void WaterDispenser::loop(uint32_t now) {
    if (now > nextReportAt) {
        if (pumpStartedAt != 0 && now - pumpStartedAt > 2000 && pulseCount - lastReportedPulseCount < 10) { // (almost) no flow for 2s? Turn off pump and notify host.
            switchPump(0 , now);
            Serial.println("DP0");
            Serial.println("DI no flow stopping pump");
            blinker.setBlinkPattern(BLINK_31);
        }
        lastReportedPulseCount = pulseCount;
        Serial.print(digitalRead(floatPin) == LOW ? "DJ0" : "DJ1");
        Serial.print(digitalRead(hookPin) == LOW ? "0\r\nDK" : "1\r\nDK");
        Serial.println(lastReportedPulseCount);
        Serial.println(pumpStartedAt == 0 ? "DP0" : "DP1");
        nextReportAt = now + 500;
    }
    while (Serial.available()) {
        char c = Serial.read(); // Will not block
        if (c == '\n') {
            inputState = 1;     // New line read, now look for 'D'
            //Serial.println("DI read eol");
        } else if (inputState == 1 && c == 'D') {
            inputState = 2;     // Next char should be a '0' or '1', to turn on or off pump.
            //Serial.println("DI read D");
        } else if (inputState == 2 && c == '0') {
            switchPump(0, now);
            inputState = 0;
        } else if (inputState == 2 && c == '1') {
            switchPump(1, now);
            inputState = 0;
        } else {
            inputState = 0;
        }
    }
}
