//-*- mode: c -*-
/* 
 * NAME
 *     WaterDispenser
 * PRECIS
 *     An Arduino-based water dispenser, which
 *     0. Is controlled by, and reports to, a host computer (eg RPi) via the USB RX/TX interface.
 *     1. Detects tank full (via float switch).
 *     2. Activates the pump (via relay).
 *     3. Measures the quantity of water through the pump (by counting pulses from a flowMeter).
 * SEE
 *     http://allegrobotics.com/aquarius.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 * DETAILS
 *     1. Tank-full detection is done with a float switch.
 *     2. Tank activation is via a relay.
 *     3. Measuring water quantity is with a flowMeter.
 * RX/TX OUTPUT TO HOST
 *     "DJ00" tank not full, hook off
 *     "DJ11" tank full, hook on
 *     "DJ00" tank not full, hook off
 *     "DJ11" tank full, hook on
 *     "DKnnnnnn" pulse counter count (eg K12345 means counter is at 12345).
 *     "DP0" pump turned off due to no (or little) flow.
 * RX/TX INPUT FROM HOST
 *      A line starting with:
 *      "D0" pump off.
 *      "D1" pump on.
 *      All other input ignored.
 * THE HOOK
 *      Untested and und unbuilt - the hook is just a microswitch on the rover's tank which is intended to change state when the rover is in the fill position.
 * TESTING
 *      UNTESTED AS AT 2018-11-11
 * REQUIREMENTS
 *      Assumes that Serial.begin(...) has been called before setup() is run.
 * DIGITAL_PIN_TO_INTERRUPT
 *     digitalPinToInterrupt() doesn't seem to be available in the arduino environment, so we have to hardwire values.
 *     On the Nano (and Uno etc) there are two interrupts: interrupt 0 which can be used on pin D2, and interrupt 1 which can be used on pin D3.
 *     On the Nano (and Uno etc) digitalPinToInterrupt(2) should return 0, and digitalPinToInterrupt(3) should return 1.
 */

#ifndef WaterDispenser_h
#define WaterDispenser_h

#include <Arduino.h>

class WaterDispenser {
public:
    WaterDispenser(byte floatPin, byte pumpPin, byte flowMeterPin, byte flowMeterPinInterrupt, byte hookPin);
    void setup();
    void loop(uint32_t now);
    static void pulseReceivedFromFlowMeter();
    void sendFlowMeterCountViaSerialPort();
private:
    byte floatPin;                       // Float switch tests for full tank.
    byte pumpPin;                        // To turn pump on / off.
    byte flowMeterPin;                   // The pin connected to the flow meter.
    byte flowMeterPinInterrupt;          // For Nano and similar, use 0 for D2, 1 for D3
    byte hookPin;                        // A switch which should activate when we are in receiving position.
    uint32_t pumpStartedAt = 0;          // 0 => pump is off, otherwise, the time it was started.
    volatile static uint32_t pulseCount;
    uint32_t lastReportedPulseCount = 0;
    uint32_t nextReportAt = 0L;
    void initializePins(void);
    void switchPump(byte mode, uint32_t now);
};

#endif /* WaterDispenser_h */
