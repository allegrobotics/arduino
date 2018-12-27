//-*- mode: c -*-
/* 
 * NAME
 *     waterdispenser.ino
 * PURPOSE
 *     Testbed for the water dispenser Aquarius.
 * NOTE
 *     UNTESTED AS AT 2018-11-11
 * SEE
 *     http://allegrobotics.com/aquarius.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "WaterDispenser.h"
// Use either ParkingSensor1 or ParkingSensor2 depending on what came in the mail from Mr eBay.
//#include "ParkingSensor1.h"
#include "ParkingSensor2.h"
#include "Bumper.h"

#define PARKING_SENSOR_READ_PIN 2
#define FLOW_METER_PIN          3
#define FLOAT_PIN               4
#define PUMP_PIN                5
#define BUMPER_PIN              6

Blinker blinker(LED_BUILTIN);
WaterDispenser waterDispenser(FLOAT_PIN, PUMP_PIN, FLOW_METER_PIN);
//ParkingSensor1 parkingSensor(PARKING_SENSOR_READ_PIN);
ParkingSensor2 parkingSensor(PARKING_SENSOR_READ_PIN);
Bumper bumper(BUMPER_PIN);

void setup() {
    delay(2000);
    Serial.begin(19200);
    blinker.setup();
    waterDispenser.setup();
    parkingSensor.setup();
    bumper.setup();
}

void loop() {
    uint32_t now = millis();
    waterDispenser.loop(now);
    parkingSensor.loop(now);
    bumper.loop(now);
    blinker.loop(now);
}
