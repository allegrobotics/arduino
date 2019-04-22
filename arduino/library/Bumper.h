//-*- mode: c -*-
/* 
 * NAME
 *     Bumper
 * PURPOSE
 *     Monitor a bumper connected to the Arudino.
 * PROTOCOL FROM HOST
 *     None.
 * PROTOCOL TO HOST
 *     "Zns" where n is buffer number (eg '0') and s is state ('0' collision, '1' notCollision)
 *     And Sends a "Zn0" or "Zn1" when the state changes, or send it periodically anyway.
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#ifndef Bumper_h
#define Bumper_h

#include <Arduino.h>
#include "King.h"

class Bumper : public King {
public:
    Bumper(byte pin = 12);
    virtual void setup();
    virtual void loop(uint32_t now);
    virtual void command(char *commandLine) {};
private:
    byte pin;                              // Which pin the bumper is on (bumper is a N/C switch between GND and this pin).
    byte reportedValue;                    // Most recent value reported to host.
    byte previousValue;                    // Previously read bumper pin.
    uint32_t nextReportAt;                 // When millis() is at least this value, we send the status again, regardless of whether it has changed.
    void report(byte value, uint32_t now); // Report the value to Serial
};

#endif /* Bumper_h */
