//-*- mode: c -*-
/* 
 * NAME
 *     WaterDispenser.h
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
 *     "DJ0" tank not full "DJ1" tank full.
 *     "DKnnnnnn" pulse counter count (eg K12345 means counter is at 12345).
 *     "DP0" pump turned off due to no (or little) flow.
 * RX/TX INPUT FROM HOST
 *      A line starting with:
 *      "D0" pump off.
 *      "D1" pump on.
 *      All other input ignored.
 * TESTING
 *      UNTESTED AS AT 2018-11-11
 * REQUIREMENTS
 *      Assumes that Serial.begin(...) has been called before setup() is run.
 * BUGS
 * Ignores the flowMeterPin passed to constructor. Always uses pin D3.
 */

#ifndef WaterDispenser_h
#define WaterDispenser_h

#include <Arduino.h>

class WaterDispenser {
private:
    byte floatPin;                       // Float switch tests for full tank.
    byte pumpPin;                        // To turn pump on / off.
    byte flowMeterPin;                   // Ignored - we use first interrupt.
    uint32_t pumpStartedAt = 0;          // 0 => pump is off, otherwise, the time it was started.
    volatile static uint32_t pulseCount;
    uint32_t lastReportedPulseCount = 0;
    uint32_t nextReportAt = 0L;
    void initializePins(void);
    void switchPump(byte mode, uint32_t now);
public:
    WaterDispenser(byte floatPin, byte pumpPin, byte flowMeterPin);
    void setup();
    void loop(uint32_t now);
    static void pulseReceivedFromFlowMeter();
    void sendFlowMeterCountViaSerialPort();
};

#endif /* WaterDispenser_h */
