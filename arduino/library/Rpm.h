//-*- mode: c -*-
/* 
 * UNTESTED/WIP
 *     ************ UNTESTED AS AT 2018-10-03.
 * NAME
 *     Rpm
 * PRECIS
 *     An arduino-based RPM meter - which recieves pulses on the interrupt pin (probably from a spark plug clamp), and periodically sends the interrupts-per-minute as bytes on the Serial port.
 * COPYRIGHT
 *     Scott BARNES 2016. IP freely on non-commercial applications.
 * DETAILS
 *     Some RPM meters have a inductive clamp which goes around the spark-plug lead.
 *     This will produce a short pulse whenever the spark plug fires.
 *     This pulse may be very high voltage, and is likely to fry the arduino chip, so it must be regulated - (A clamping diode and voltage divider may work, or something more sophisticated may be required).
 *     This (regulated) pulse causes an interrupt routine to be called on the arduino, which records the time between pulses.
 *     Periodically, a byte is sent on the Serial output representing the current (or smoothed/recent) RPM.
 * WARNING
 *      The Arduino MUST be protected from high voltage.
 *      Some 4-stroke engines use a 'wasted spark', some don't. So it may be necessary to double the reported RPM to get the actual value, or it may not. Suck it and see.
 * OUTPUT TO HOST
 *      Host will receive a packet every REFRESH_INTERVAL_MS milliseconds (eg 100 == 10 times per second, check the code), in the form
 *      "Rxxx" (xxx is rpm expressed as hex).
 *      Values above 8192 are expressed as "FFF"
 *      When the RPM drops to zero, the module will report low values (< 0x00F) due to the smoothing used.
 * ALGORITHM
 *      Smoothing is done by a modified exponential smoothing filter, which tries to smooth over REFRESH_INTERVAL_MS (200ms by default).
 * PHILOSOPHY
 *      The sparks cause a voltage drop, which calls an interrupt, the interrupt maintains the 'smoothedRpm' value.
 *      The normal Arduino 'loop()' call sends the current value to the host every REFRESH_INTERVAL_MS.
 *      This means the host will be receiving updates at a constant rate regardless of the RPM of the motor.
 * DIGITAL_PIN_TO_INTERRUPT
 *     digitalPinToInterrupt() doesn't seem to be available in the arduino environment, so we have to hardwire values.
 *     On the Nano (and Uno etc) there are two interrupts: interrupt 0 which can be used on pin D2, and interrupt 1 which can be used on pin D3.
 *     On the Nano (and Uno etc) digitalPinToInterrupt(2) should return 0, and digitalPinToInterrupt(3) should return 1.
 * DOUBLING UP
 *     On most 4-stroke motors, it is necessary to double the reported RPM because the spark plug only fires every second revolution
 *     (SOME motors have a wasted spark, and will not need to be doubled - check this).
 *     Maybe this module should actually be called SPM (Sparks Per Minute) instead of RPM, but it is what it is.
 */

#ifndef Rpm_h
#define Rpm_h

#include <Arduino.h>

#define DEFAULT_SPARKPLUG_PIN            2 /* Use 2 for D2, 3 for D3 */
#define DEFAULT_SPARKPLUG_PIN_INTERRUPT  0 /* digitalPinToInterrupt(DEFAULT_SPARKPLUG_PIN) */

class Rpm {
public:
    Rpm(byte pin = DEFAULT_SPARKPLUG_PIN, byte pinInterrupt = DEFAULT_SPARKPLUG_PIN_INTERRUPT);
    void setup();
    void loop(uint32_t now);
private:
    byte pin;
    byte pinInterrupt;
    void initializePins(void);
    void sendRpmViaSerialPort(uint32_t now);
    static char *hex;
    uint32_t nextReportAt;
    static volatile uint32_t lastSparkAt;
    static volatile int smoothedRpm;
    static void pulseReceived();
};

#endif /* Rpm_h */
