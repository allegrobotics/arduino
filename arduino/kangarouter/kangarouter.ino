//-*- mode: c -*-
/*
 * FILE
 *     kangarouter.ino
 * PURPOSE
 *     The Arduino code running in the Arduino Nano on the kangarouter functions.
 * SEE
 *     http://allegrobotics.com/kangarouter.html
 * COPYRIGHT
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * NOTES
 * This consists of 
 * * A LSM9DS1/LSM9DS0 for orientation measurement.
 * * Two ZS-X11B motor controllers (controlling for left and right BLDC motors).
 * The kangarouter does the Helm adjustment on-board because the motors have to updated / corrected very quickly to maintain direction.
 * Other rovers do the Helm adjustment in the Raspberry Pi.
 * Technically this means squirting 0-5V into the SDA/SCL instead of 3.3V, but the LSM chips seem to survive this.
 *
 * This does NOT work with the LSM9DS1. There are still issues with this.
 *
 * PIN ASSIGNMENTS
 *   0 - RX
 *   1 - TX
 *   2 -
 *   3 - Left motor  speed
 *   4 - Left motor  direction
 *   5 - Right motor speed
 *   6 - Right motor direction
 *   7 - Hall effect left  motor pin A (optional)
 *   8 - Hall effect left  motor pin B (optional)
 *   9 - Hall effect left  motor pin C (optional)
 *  10 - Hall effect right motor pin A (optional)
 *  11 - Hall effect right motor pin B (optional)
 *  12 - Hall effect right motor pin C (optional)
 *  13 - Blinker
 *  A4 - SDA (I2C LSM9DS1/LSM9DS0)
 *  A5 - SCL (I2C LSM9DS1/LSM9DS0)
 */

#include "Blinker.h"
#include "Lsm9ds0Imu.h"
//#include "Lsm9ds1Imu.h"
#include "Ahrs.h"
#include "HoverboardDrive.h"
#include "Helm.h"

Blinker blinker(13);
HoverboardDrive drive(false, true, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
Lsm9ds0Imu imu;
//Lsm9ds1Imu imu;
Ahrs ahrs(&imu);
Helm helm(&ahrs, &drive, 50, 1000);

void setup() {
    delay(1000);
    Serial.begin(19200);
    while (!Serial) delay(1);
    Serial.println("KI Kangarouter setting up");
    delay(1000);
    blinker.setup();
    imu.setup();
    imu.setReportInterval(0);   // Not actually interested in IMU report
    ahrs.setup();
    ahrs.setReportInterval(200);
    drive.setup();
    helm.setup();
    Serial.println("KI Kangarouter setup complete");
    blinker.setBlinkPattern(BLINK_PATTERN_13); // Setup complete, but never fed.
}

// The loop routine runs over and over again forever.
void loop() {
    uint32_t now = millis();
    imu.loop(now);
    ahrs.loop(now);
    drive.loop(now);
    helm.loop(now);
    blinker.loop(now);
    checkCommandInput(now);
}

#define MAX_COMMAND_LENGTH 32 /* The maximum length of a command line from the host */
char commandLine[MAX_COMMAND_LENGTH];
byte commandLinePopulation = 0;
uint32_t lastCommandReadAt = 0L;

void checkCommandInput(uint32_t now) {
    if (Serial.available()) {
        byte b = Serial.read();
        if (b == '\n' || b == '\r' || commandLinePopulation >= MAX_COMMAND_LENGTH - 1) {
            commandLine[commandLinePopulation] = '\0';
            Serial.print("KD interpret command: "); Serial.println(commandLine);
            imu.command(commandLine);
            ahrs.command(commandLine);
            drive.command(commandLine);
            helm.command(commandLine);
            commandLinePopulation = 0;
            blinker.setBlinkPattern(BLINK_PATTERN_22); // We are being fed.
            lastCommandReadAt = now;
        } else
            commandLine[commandLinePopulation++] = b;
    } else {
        if (now - lastCommandReadAt > 5000)
            blinker.setBlinkPattern(BLINK_PATTERN_21); // We are hungry.
    }
}
