//-*- mode: c -*-
/* 
 * NAME
 *     Bumper
 * PURPOSE
 * Monitor a bumper connected to the Arudino.
 * OUTPUT TO HOST
 * "Zns" where n is buffer number (eg '0') and s is state ('0' collision, '1' notCollision)
 * Sends a "Zn0" or "Zn1" when the state changes, or send it periodically anyway.
 * PHILOSOPHY
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#ifndef Bumper_h
#define Bumper_h

#include <Arduino.h>

class Bumper {
public:
    Bumper(byte pin = 12);
    void setup();
    void loop(uint32_t now);
private:
    byte pin;                              // Which pin the bumper is on (bumper is a N/C switch between GND and this pin).
    byte reportedValue;                    // Most recent value reported to host.
    byte previousValue;                    // Previously read bumper pin.
    uint32_t nextReportAt;                 // When millis() is at least this value, we send the status again, regardless of whether it has changed.
    void report(byte value, uint32_t now); // Report the value to Serial
};

#endif /* Bumper_h */
