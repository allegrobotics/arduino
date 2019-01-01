//-*- mode: c -*-
/* 
 * NAME
 *     aquarius.ino
 * PURPOSE
 *     The code to run on the Nano in the Aquarius rover.
 * NOTE
 *     UNTESTED AS AT 2018-11-11
 * SEE
 *     http://allegrobotics.com/aquarius.html
 *     http://allegrobotics.com/parkingSensor.html
 * COPYRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 */

#include <Arduino.h>

#include "Blinker.h"
#include "WaterDispenser.h"
// Use either ParkingSensor1 or ParkingSensor2 depending on what came in the mail from Mr eBay.
#include "ParkingSensor1.h"
//#include "ParkingSensor2.h"
#include "Bumper.h"

#define PARKING_SENSOR_PIN           2 /* 2 for pin D2, 3 for pin D3. */
#define PARKING_SENSOR_PIN_INTERRUPT 0 /* digitalPinToInterrupt(PARKING_SENSOR_PIN) */
#define FLOW_METER_PIN               3 /* 2 for pin D2, 3 for pin D3. */
#define FLOW_METER_PIN_INTERRUPT     1 /* digitalPinToInterrupt(FLOW_METER_PIN) */
#define FLOAT_PIN                    4
#define PUMP_PIN                     5
#define BUMPER_PIN                   6
#define HOOK_PIN                     7

Blinker blinker(LED_BUILTIN);
WaterDispenser waterDispenser(FLOAT_PIN, PUMP_PIN, FLOW_METER_PIN, FLOW_METER_PIN_INTERRUPT, HOOK_PIN);
ParkingSensor1 parkingSensor(PARKING_SENSOR_PIN, PARKING_SENSOR_PIN_INTERRUPT);
//ParkingSensor2 parkingSensor(PARKING_SENSOR_PIN, PARKING_SENSOR_PIN_INTERRUPT);
Bumper bumper(BUMPER_PIN);

void setup() {
    delay(2000);
    Serial.begin(19200);
    Serial.println("I Aquarius starting.");
    blinker.setup();
    waterDispenser.setup();
    parkingSensor.setup();
    bumper.setup();
    Serial.println("I Aquarius started.");
}

void loop() {
    uint32_t now = millis();
    waterDispenser.loop(now);
    parkingSensor.loop(now);
    bumper.loop(now);
    blinker.loop(now);
}
