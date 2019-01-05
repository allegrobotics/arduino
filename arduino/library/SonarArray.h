//-*- mode: c -*-
/* 
 * NAME
 *     SonarArray.h
 * PRECIS
 *     Five HCSR04s/HCSR05s for obstacle detection .. carefully placed to avoid the control pins used by the motor driver.
 * AUTHOR
 *     Copyright Live Software 2018-11-30 All Rights Reserved.
 *     IP Freely on non-commercial applications.
 */

#include <Arduino.h>

#define NUM_SONARS                     5

class SonarArray {
    
public:
    SonarArray();
    void setup();
    void loop(uint32_t now);
    int sonarDistanceCm[NUM_SONARS];
private:
    int state = 0;
    int pingCm(byte n);
};
