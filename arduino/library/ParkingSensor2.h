//-*- mode: c -*-
/* 
 * NAME
 *     parkingsensor2.h
 * SEE
 *     http://allegrobotics.com/parkingSensor.html
 * PURPOSE
 * Arduino Nano connected to a "TYPE-2" parking sensor controller to decode the PWM and send it via USB to a host (eg a RPi).
 * It will NOT WORK with the "TYPE-1" parking sensor controller (use ParkingSensor1 for that).
 * It assumes that the sensor is in 'reverse' mode - ie that the red and yellow wires are held high (at 12V).
 * (Making a more sophisticated version with reversing controls would be easy enough, but not yet done).
 * AUTHOR
 *     Scott BARNES
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 * HOW IT WORKS
 * It connects to a host (eg a Raspberry Pi) via USB (which provides power as well as comms.)
 * 12V must be supplied to the parking sensor via other means.
 * OUTPUT TO HOST
 * No data is sent from the host. Arduino to host is 9600baud.
 * OUTPUT FORMAT TO HOST
 * Sends to host text Lines 30 times a second of the form "SXX" where S is the sensor (A,B,C,D,E,F,G,H) and XX is the distance in cm expressed as hex ('FF' is max distance - ie no measurement).
 * This will normally be in cycles of six (A,B,C,D,E,H sensors F and G normally don't work in 'reverse' mode).
 * eg A15\r\nBFF\r\nCFF\r\nDFF\r\nE40\r\nH44\r\n
 * Means A has detected a 21cm object, E has detected a 64cm object, H has detected a 68cm object and the rest have detected nothing.
 * 
 * SEE ALSO
 * ParkingSensor2.java - some Java code which talks to this. If you change this code you may have to change that too.

 * HOW TO CONNECT IT
 *
 *   POWER       MONITOR       E        F        G        H        A        B        C        D
 * +---------+ +---------+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+
 * | . . . . | | . . . . |  | . . |  | . . |  | . . |  | . . |  | . . |  | . . |  | . . |  | . . |
 * +---------+ +---------+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+  +-----+
 *   B B Y R     G S 5 ?
 * Key: B=Black, B=Blue, Y=Yellow, R=Red, G=GND, S=SIGNAL, 5=5VDT, ?=WTF above.
 * Don't bother plugging in sensors F and G - they don't work in reverse mode.
 * 
 * Connect the GND and SIGNAL from the parking sensor controller (the left-most and second to left-most pins in the MONITOR socket) to GND and D2 on the Arduino Nano respectively.
 * The USB host (eg a Raspberry Pi) conection, will provide power to the Arduino as well as comms, but 12VDC power will still be required for the controller.
 * 
 * THE PROTOCOL - FROM THE parking sensor (see http://allegrobotics.com/parkingSensor.html)
 *
 * The protocol - from the parking sensor is 33%/66% duty cycle PWM, LOW is 0V, HIGH is 5V.
 * A 16-bit packet is sent every 33ms or so.
 * Pins D2 (interrupt 0) and D3 (interrupt 1) on the Nano can be triggered by rising or falling inputs, so we watch one of these
 * and analyse the time between rises and falls.
 * Between each packet, there is:  25000us LOW, 900us HIGH, 1500us LOW, 100us HIGH.
 * Each packet consists of 16 bits, and we read MSB first.
 * Each bit is 266us HIGH, 266us { LOW if 1, HIGH if 0 }, 266us LOW.
 * So the easiest way to read the data is to measure the duration of the HIGH periods. 266us is '0', 532us is '1', > 900us is a break between packets (but be warned there are two of them).
 *
 * BUGS
 * Ignores the SENSOR pin passed to constructor. Always uses pin D2.
 */

#ifndef ParkingSensor2_h
#define ParkingSensor2_h

#include <Arduino.h>

#define DEFAULT_PARKING_SENSOR_PIN                  2  /* Use 2 for D2, 3 for D3 */
#define DEFAULT_PARKING_SENSOR_PIN_INTERRUPT        0  /* digitalPinToInterrupt(DEFAULT_PARKING_SENSOR_PIN) */

class ParkingSensor2 {
public:
    ParkingSensor2(byte pin = DEFAULT_PARKING_SENSOR_PIN, byte pinInterrupt = DEFAULT_PARKING_SENSOR_PIN_INTERRUPT);
    void setup();
    void loop(uint32_t now);
 private:
    static byte pin;
    static byte pinInterrupt;
    uint32_t dataLastSentAt;             // The time (millis()) that we last sent data to the host.
    volatile int sensorTimesDistanceCm; // Cubby for value to send to the host. This is the sensor (a = 0, b = 1 .. h = 7
    static void risingEdge();
    static void fallingEdge();
    //void initializePins(void);
    static char *hex;
};

#endif /* ParkingSensor2_h */
