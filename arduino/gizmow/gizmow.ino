//-*- mode: c -*-
/*
 * NAME
 *     gizmow.ino
 * PURPOSE
 *     The code to run on the Arduino Nano in the GizMow.
 *     The GizMow uses the ParkingSensor, the Rpm and the collision Bumper.
 * SEE ALSO
 *     http://allegrobotics.com/gizMow.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>
#include "Blinker.h"
//#include "ParkingSensor1.h"
#include "ParkingSensor2.h"
#include "Bumper.h"
#include "Rpm.h"

#define PARKING_SENSOR_PIN           2 /* 2 for pin D2, 3 for pin D3 */
#define PARKING_SENSOR_PIN_INTERRUPT 0 /* digitalPinToInterrupt(PARKING_SENSOR_PIN) */
#define RPM_PIN                      3 /* 2 for pin D2, 3 for pin D3 */
#define RPM_PIN_INTERRUPT            1 /* digitalPinToInterrupt(RPM_PIN) */
#define BUMPER_PIN                  12

Blinker        blinker(LED_BUILTIN);
// Use either ParkingSensor1 or ParkingSensor2 - whatever turns up in the box from eBay :)
//ParkingSensor1 parkingSensor(PARKING_SENSOR_PIN_INTERRUPT);
ParkingSensor2 parkingSensor(PARKING_SENSOR_PIN_INTERRUPT);
Rpm            rpm(RPM_PIN, RPM_PIN_INTERRUPT);
Bumper         bumper(BUMPER_PIN);

// The setup routine runs once when you reset.
void setup() {
    delay(3000); // Delay startup to be sure we can get in first to re-flash.
    Serial.begin(19200);
    while (!Serial) delay(1);
    parkingSensor.setup();
    rpm.setup();
    bumper.setup();
    blinker.setup();
}

// The loop routine runs over and over again forever.
void loop() {
    unsigned long now = millis();
    parkingSensor.loop(now);
    bumper.loop(now);
    rpm.loop(now);
    blinker.loop(now);
}
