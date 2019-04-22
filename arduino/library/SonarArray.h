//-*- mode: c -*-
/* 
 * NAME
 *     SonarArray.h
 * PURPOSE
 *     Five HCSR04s/HCSR05s for obstacle detection .. carefully placed to avoid the control pins used by the motor driver.
 * AUTHOR
 *     Scott BARNES 2018. IP Freely on non-commercial applications.
 * PROTOCOL FROM HOST
 *     None
 * PROTOCOL TO HOST
 *     None
 */

#ifndef SonarArray_h
#define SonarArray_h

#include <Arduino.h>
#include "King.h"

#define NUM_SONARS                     5

class SonarArray : public King {
    
public:
    SonarArray();
    void setup();
    void loop(uint32_t now);
    void command(char *commandLine) {};
    int sonarDistanceCm[NUM_SONARS];
private:
    int state = 0;
    int pingCm(byte n);
};

#endif /* SonarArray_h */
