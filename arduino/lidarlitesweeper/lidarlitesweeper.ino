// -*- mode: c -*-
/**
 * NAME
 *     lidarlitesweeper.ino
 * PURPOSE
 *     A LidarLite V1 mounted on a stepper rotator.
 *     Code for a stepper LidarLite-V1 sweeper - that is, a LidarLite-V1 mounted on a stepper to sweep around 360 degrees.
 * VERSION
 *     V2.0 ('Oliver' - because he is a sweeper and his name is 'Twist', get it ?)
 * TARGET HARDWARE
 *     Arduino Nano
 * COPYRIGHT
 *     Scott BARNES 2017/2018. IP freely on non-commercial applications.
 * DETAILS
 * Protocol is documented in LidarLiteSweeper.java.
 * This breaks the 'normal' protocal standards for the Arduinos - its too fast and furious.
 * Basically:
 * + it sends packets (of 1 or 2 bytes)
 * + first byte in packet always has MSB set, subsequent bytes don't (like MIDI).
 * + 1 byte packets are status packets.
 * + 2 byte packets are distance measurements.
 * 
 * BUGS
 * Currently the system crashes if it is in binary mode - something to do with the timings.
 * It works fine in 'P' (printable) mode.
 * 
 * +------+            +-------------------------------------+
 * |      |            |                                     +-----I2C---- LidarLite
 * | Host +----USB-----+ Nano running 'lidarlitesweeper.cpp' |
 * |      |            |                                     +-----pins---StepperMotorController-----StepperMotor-----Turret
 * +------+            +-------------------------------------+
 * 
 * WARNING
 * This is for a LidarLite V1, not a V2 or V3, which requires different code.
 * 
 * LIMITATIONS
 * The LidarLite-V1 can only return 100 readings per second.
 * The Sweeper has a 1.8 degree step, ie 200 steps per revolution, hence this can ONLY do one revolution per 2 seconds.
 * The LidarLite-V2 can do 500 readings per second, but I don't have one.
 */

#include <Arduino.h>

#define BYTE uint8_t

int debug = 0;  /* if true, write out debugging messsages */

//void sendByte(int b);
//void sendPacket(BYTE *packet);
void test1();
void test2();
void test3();
void test4();
void test5();
void test6();
void test7();

void initializeLidarLite();
int  readLidarLiteRange();
void sendDistance(int distance);
int  requestLidarLiteRange();

void initializeStepper();
void disableStepper();
void enableStepper();
void wakeStepper();
void sleepStepper();
void step0();

/**
 * Define I2C ports.
 * Use the standard ones.
 */
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5

#define INTERRUPTER_PIN 3 // D3
//#include "SoftI2CMaster.h"
// Why do we use I2C.h and not Wire.h?
// I2C has a I2c.timeout()
#include "I2C.h"

#define LIDARLITE_ADDRESS     0x62          // Default I2C Address of LIDAR-Lite.
#define REGISTER_MEASURE      0x00          // Register to write to initiate ranging.
#define MEASURE_VALUE         0x04          // Value to initiate ranging.
#define REGISTER_HIGH_LOW_B   0x8F          // Register to get both High and Low bytes in 1 call.

#define STATE_STOPPED            0
#define STATE_WORKING            1
#define DELAY_TIME_US          2000           // Delay between each motor step (u-sec, not m-sec).

// Sets a timeout to ensure no locking up of sketch if I2c communication fails.
// Making this lower will reduce the amount of time 'in stall' where the Lidar cannot get a value, and waits and waits, but may also
// increase the number of bad reads.
#define I2C_TIMEOUT_MS          50

int state = STATE_STOPPED;
int stepperPosition = 0;       // [0 .. SWEEP_STEPS - 1] the current position of the neck.

/* Special characters in the binary (production) protocol. */

//#define LD_STATUS_ALIVE        0b10100000 /* 0xA0 Lidarduino is alive, program is running and initializing */
//#define LD_STATUS_READY        0b10100001 /* 0xA1 Lidarduino is ready to recieve instructions (maybe 'G' and 'S') */
//#define LD_STATUS_WORKING      0b10100010 /* 0xA2 Lidarduino is working - scanning and sending ranges */
//#define LD_STATUS_STOPPED      0b10100011 /* 0xA3 Lidarduino is stopped - no longet scanning and sending ranges */
//#define LD_LIDAR_FAIL          0b10100100 /* 0xA4 Lidar has failed (I2C communication with Lidar failed) */
//#define LD_SYNCRONIZE          0b10100101 /* 0xA5 Photointerrupter has gone from high to low - ie we have synched. */

#define LD_STATUS_ALIVE        'a'
#define LD_STATUS_READY        'r'
#define LD_STATUS_WORKING      'w'
#define LD_STATUS_STOPPED      's'
#define LD_LIDAR_FAIL          'e'
#define LD_SYNCHRONIZE         'z'
#define LD_DEBUG               'd'

int lidarErrorsInARow = 0; // Number of bad lidar reads in a row. If this gets too high we are boned, and should let the host know.

#define LED_PIN 13            /* The LED which we can flash */

//int ledState;

/*
 * *******************************
 * The blinking logic
 */

#define BLINK_PATTERN_INIT    0x00000501  // 1,2
#define BLINK_PATTERN_STOPPED 0x00001501  // 1,3
#define BLINK_PATTERN_RUNNING 0x00001405  // 2,2
#define BLINK_PATTERN_ERROR   0x00005415  // 3,3

uint32_t blinkPattern = BLINK_PATTERN_INIT;
uint8_t blinkPosition = 0;
unsigned long nextBlinkAt = 0;

/**
 * Change the blinker if necessary.
 */
void blinkIfNecessary() {
    unsigned long now = millis();
    if (now >= nextBlinkAt) {
        //Serial.print("d blink change. blinkPosition "); Serial.print(blinkPosition); Serial.print(" blinkPattern "); Serial.print(blinkPattern); Serial.print(" nextBlinkAt "); Serial.println(nextBlinkAt);
        blinkPosition = (blinkPosition + 1) % (sizeof(blinkPattern) * 8);
        digitalWrite(LED_PIN, (blinkPattern >> blinkPosition) & 0x01);
        nextBlinkAt = now + 75;
    }
}


/**
 * Set the blink pattern.
 * @param newBlinkPattern on of the BLINK_PATTERN_s above
 */
void setBlinkPattern(uint32_t newBlinkPattern) {
    blinkPattern = newBlinkPattern;
}

BYTE infoPacket[3] = { 'L', 0x00, '\n' };

/**
 * A one-character line. Use lower case letters, so as to differentiate from distance packets.
 */
void sendInfo(BYTE b) {
    infoPacket[1] = b;
    Serial.write(infoPacket, 3);
}

/**
 * Called initially at startup.
 */
void setup() {
    delay(2000);                          // Always put a delay in to enable reprogramming, or risk bricking the Nano! :(
    //Serial.begin(9600);                 // This is a good reliable speed, and hopefully fast enough when we are using the binary protocol.
    Serial.begin(38400);                  // This is a fast speed - for production.
    while (!Serial) delay(1);
    if (debug) Serial.println("D Starting");
    //sendByte(LD_STATUS_ALIVE);
    sendInfo(LD_STATUS_ALIVE);
    delay(100);
    initializeLidarLite();
    initializeStepper();
    // Interupter
    pinMode(INTERRUPTER_PIN, INPUT);
    // Led Pin
    pinMode(LED_PIN, OUTPUT);
    setBlinkPattern(BLINK_PATTERN_INIT);
    //sendByte(LD_STATUS_READY);
    if (debug) Serial.println("d Ready");
    sendInfo(LD_STATUS_READY);

    /*
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    */
}

/*
void changeLed() {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}
*/

/**
 * Process a command from the host.
 */
void processCommand(char c) {
    switch (c) {
    case 'D': // Debug mode. Debuging commands.
        Serial.println("d Debugging mode on");
        debug = 1;
        break;
    case 'P': // Production mode, but no debugging.
        //if (debug) Serial.println("d Going to production");
        debug = 0;
        break;
        /*
    case 'B': // Binary
        if (debug) Serial.println("d Going to binary mode");
        debug = 0;
        binary = 1;
        break;
    case 'P': // Printable, but no debugging.
        if (debug) Serial.println("d Going to printable");
        binary = 0;
        debug = 0;
        break;
        */
    case 'G': // Go
        //sendByte(LD_STATUS_WORKING);
        sendInfo(LD_STATUS_WORKING);
        setBlinkPattern(BLINK_PATTERN_RUNNING);
        if (debug) Serial.println("d INFO_STATE_WORKING");
        state = STATE_WORKING;
        wakeStepper();
        enableStepper();
        delay(1); // Wait for stepper to wake.
        break;
    case 'S': // Stop
        //sendByte(LD_STATUS_STOPPED);
        sendInfo(LD_STATUS_STOPPED);
        setBlinkPattern(BLINK_PATTERN_STOPPED);
        if (debug) Serial.println("d INFO_STATE_STOPPED");
        state = STATE_STOPPED;
        //disableStepper(); not used in V2
        sleepStepper();
        disableStepper();
        break;
    case '2': // Test 2
        if (debug) Serial.println("d Test 2");
        test2();
        if (debug) Serial.println("d Test Complete");
        break;
    case '3': // Test 3
        if (debug) Serial.println("d Test 3");
        test3();
        if (debug) Serial.println("d Test Complete");
        break;
    case '4': // Test 4
        if (debug) Serial.println("d Test 4");
        test4();
        if (debug) Serial.println("d Test Complete");
        break;
    case '5': // Test 5
        if (debug) Serial.println("d Test 5");
        test5();
        if (debug) Serial.println("d Test Complete");
        break;
    case '6': // Test 6
        if (debug) Serial.println("d Test 6");
        test6();
        if (debug) Serial.println("d Test Complete");
        break;
    case '7': // Test 7
        if (debug) Serial.println("d Test 7");
        test7();
        if (debug) Serial.println("d Test Complete");
        break;
    default:
        // Ignore, could be LF, CR, noise, (char) 0 at startup, whatever.
        break;
    }
}

/**
 * Test2 - just step the motor quickly.
 */
void test2() {
    // How fast can we step the motor
    wakeStepper();
    enableStepper();
    delay(1); // Wait for stepper to wake up.
    if (debug) Serial.println("d stepper speed test");
    for (int i = 0; i < 2000; i++) { // 200 is number of steps. On full step, this is one turn.
        delayMicroseconds(2000);
        step0();
    }
    sleepStepper();
    disableStepper();
}

/**
 * Test3 - just read 100 lidar values as quickly as possible.
 */
void test3() {
    // Just read lidar values.
    wakeStepper();
    enableStepper();
    int i;
    int distance;
    int results[100];
    long started = millis();
    for (i = 0; i < 100; i++) {
        requestLidarLiteRange();
        step0();
        delayMicroseconds(100);
        distance = readLidarLiteRange();
        results[i] = distance;
        step0();
        delayMicroseconds(100);
        Serial.write(".");
    }
    long now = millis();
    sleepStepper();
    disableStepper();
    if (debug) {
        Serial.println();
        Serial.print(" duration ");
        Serial.println(now - started);
    }
    if (debug) {
        for (int i = 0; i < 100; i++) {
            Serial.print(" ");
            Serial.print(results[i]);
        }
        Serial.println();
        Serial.print("d Lidar distance is ");
        Serial.print(distance);
        Serial.print(" lidarErrorsInARow ");
        Serial.println(lidarErrorsInARow);
    }
}

/**
 * Test4 - toggling stepper "ENABLE" as a test.
 */
void test4() {
    /*
    // toggle Enable/sleep on the Stepper Driver to see what happens.
    if (DEBUG) Serial.println("d toggle A2 on ZD-M42");;
    for (int i = 0; i < 20; i++) {
        Serial.println("enable");
        enableStepper();
        wakeStepper();
        delay(3000);
        Serial.println("disable");
        disableStepper();
        sleepStepper();
        disableStepper();
        delay(3000);
    }
    */
}

/**
 * Test5 - return the  value of the photo interrupter' pin (ie is the blade in the detector?).
 */
void test5() {
    // Monitor the INTERRUPTER pin to debug the loop detector.
    /* if (debug) */ Serial.println("d Monitor interrupter");
    int value = digitalRead(INTERRUPTER_PIN);
    Serial.print("value is ");
    Serial.println(value);
}

#define T6_DURATION 10000

/**
 * Test6 - read the lidar values as quickly as possible.
 */
void test6() {
    // How fast can we read lidar values?
    unsigned long stopAt = millis() + T6_DURATION;
    int count = 0;
    int numZeros = 0;
    int max = 0;
    int min = 9999;
    if (debug) Serial.println("d Test6 - TIMING TEST");
    if (debug) { Serial.print("d duration "); Serial.println(T6_DURATION); }
    if (debug) Serial.println("d STARTING ..");
    while (millis() < stopAt) {
        requestLidarLiteRange();
        delayMicroseconds(1);
        int distance = readLidarLiteRange();
        if (max < distance)
            max = distance;
        if (min > distance)
            min = distance;
        //delayMicroseconds(10);
        //delayMicroseconds(25);      // zeroCount: 37, perSecond: 94
        //delayMicroseconds(37);      // zeroCount: 35, perSecond: 85
        //delayMicroseconds(40);      // zeroCount: 24, perSecond: 76
        //delayMicroseconds(41);      // zeroCount: 0, per second: 74
        //delayMicroseconds(42);      // zeroCount: 0, per second: 77
        //delayMicroseconds(45);      // zeroCount: 0, per second: 77 distance works (sometimes).
        //delayMicroseconds(50);      // zeroCount: 0, per second: 74 distance works (mostly).
        //delayMicroseconds(75);       // distance works
        delayMicroseconds(100);       // distance works
        //delayMicroseconds(180);       // distance works
        //delayMicroseconds(250);       // distance works
        //delayMicroseconds(1000);      // distance works
        count++;
        if (distance == 0)
            numZeros++;
    }
    if (debug) {
        Serial.println("d .. COMPLETED");
        Serial.print("d Count ");
        Serial.println(count);
        Serial.print("d Zero count ");
        Serial.println(numZeros);
        Serial.print("d per second ");
        Serial.println(count / 10);
        Serial.print("d good per second ");
        Serial.println((count - numZeros) / 10);
        Serial.print("d min ");
        Serial.println(min);
        Serial.print("d max ");
        Serial.println(max);
    }
}

/**
 * Test7 - how fast can we read the lidar values.
 */
void test7() {
    // How fast can we read lidar values?
    unsigned long stopAt = millis() + 10000; // go for 10
    int count = 0;
    int numZeros = 0;
    if (debug) Serial.println("d Starting timing test");
    while (millis() < stopAt) {
        requestLidarLiteRange();
        delayMicroseconds(1);
        int distance = readLidarLiteRange();
        if (debug) {
            Serial.print("d distance ");
            Serial.println(distance);
        }
        //delayMicroseconds(10);
        //delayMicroseconds(25);      // zeroCount: 37, perSecond: 94 
        //delayMicroseconds(37);      // zeroCount: 35, perSecond: 85
        //delayMicroseconds(40);      // zeroCount: 24, perSecond: 76
        //delayMicroseconds(41);      // zeroCount:  0, perSecond: 74
        //delayMicroseconds(42);      // zeroCount:  0, perSecond: 77
        //delayMicroseconds(45);      // zeroCount:  0, perSecond: 77 distance works (sometimes).
        //delayMicroseconds(50);      // zeroCount:  0, perSecond: 74 distance works (mostly).
        //delayMicroseconds(75);      // distance works
        delayMicroseconds(100);     // distance works
        //delayMicroseconds(180);     // distance works
        //delayMicroseconds(250);     // distance works
        //delayMicroseconds(1000);    // distance works
        count++;
        if (distance == 0)
            numZeros++;
    }
}

BYTE lastInterrupterState = 0;

/**
 * Do a unit of work in between checking for commands from the host.
 */
void work() {
    //changeLed();
    requestLidarLiteRange();
    if (lidarErrorsInARow > 200) {
        //sendByte(LD_LIDAR_FAIL);
        Serial.println(LD_LIDAR_FAIL);
        if (debug) Serial.println("d ERROR_LIDAR_FAIL");
        //sendByte(LD_STATUS_STOPPED);
        Serial.println(LD_STATUS_STOPPED);
        if (debug) Serial.println("d STATUS_STOPPED");
        state = STATE_STOPPED;
    }
    step0();
    delayMicroseconds(10);
    //delayMicroseconds(DELAY_TIME_US); // Removed the delay - doesn't seem to be required due to delays in lidar stuff.
    int distance = readLidarLiteRange();
    //int distance = 99;
    step0();
    delayMicroseconds(10);
    sendDistance(distance);
    int currentInterrupterState = digitalRead(INTERRUPTER_PIN);
    if (currentInterrupterState == 1 && lastInterrupterState == 0) {
        // We are at the synchronizationposition (in V1 this was -158 degrees, ie 158 degrees ACW of straight ahead, or 202 degrees CW of straight ahead) in V2 it's ... something else.
        //sendByte(LD_SYNCRONIZE);
        Serial.println(LD_SYNCHRONIZE);
        if (debug) Serial.println("d LD_SYNCHRONIZE");
    }
    lastInterrupterState = currentInterrupterState;
}

byte inputState = 0; // 0 => idle; 1 => about to read first char in line; 2 => read 'L' at start of line, awaiting command

/**
 * Called continuously after setup() returns.
 */
void loop() {
    // Process commands that have an 'L' at the start of the line.
    if (Serial.available()) {
        char c = Serial.read(); // Will not block
        if (inputState == 0 && c == '\n') {
            inputState = 1;
        } else if (inputState == 1 && c == 'L') {
            inputState = 2;
        } else if (inputState == 2) {
            processCommand(c);
            inputState = 0;
        }
    }
    if (state == STATE_WORKING) {
        work();
    } else {
        // We are not actually running.
        // Hang around for a while then return.
        delayMicroseconds(DELAY_TIME_US);
    }
    blinkIfNecessary();
}

BYTE distancePacket[4] = { 'L', 0x00, 0x00, '\n' };

/**
 * Send the distance measurement to the host.
 */
void sendDistance(int distanceCm) {
    distancePacket[1] = ((distanceCm >> 6) + 32);   // [0..64] maps to char [' '..'_']
    distancePacket[2] = ((distanceCm & 0x3f) + 32); // [0..64] maps to char [' '..'_']
    Serial.write(distancePacket, 4);
    /*
    // Distance should only go up to 40m (4000cm), so higher numbers get compressed to the maximum.
    if (distance > 4100)
        distance = 4100;
    if (debug) {
        Serial.print("d d=");
        Serial.println(distance);
    }
    packet[0] = 0x80 | (distance >> 7);
    packet[1] = distance & 0x7F;
    sendPacket(packet);
    */
}

/**
 * Send a packet (2 byte packet) to the host.
 * If this is binary, we send the two bytes.
 * @param packet the bytes to send (must be 2 bytes)
 */
/*
void sendPacket(BYTE *packet) {
    if (binary) {
        Serial.write(packet, PACKET_SIZE);
    } else {
        sendByte(packet[0]);
        sendByte(packet[1]);
    }
}
*/

// Keep this pre-fab text line around for sending bytes.
// This will mean fewer calls to "Serial.write", and hence (hopefully) will make things go faster.
// This may be tweakable with the USB latency_timer settings in the host, but it's probably not worth it.
//BYTE nonBinaryByte[4] = { 'B', 0x00, 0x00, '\n' };

/**
 * Send a byte to the host.
 * If we are not in binary mode, this is sent as a line 'Bxx' where xx is the hexidecimal
 */
/*
void sendByte(int b) {
    if (binary) {
        Serial.write(b);
    } else {
        nonBinaryByte[1] = "0123456789ABCDEF"[(b >> 4) & 0x0F];
        nonBinaryByte[2] = "0123456789ABCDEF"[b & 0x0F];
        Serial.write(nonBinaryByte, 4); // Do we need to flush?
//        Serial.print("B");
//        Serial.print("0123456789ABCDEF"[(b >> 4) & 0x0F]);
//        Serial.println("0123456789ABCDEF"[b & 0x0F]);
    }
}
*/

/**************************************************/
/* STEPPER MOTOR CONTROL ROUTINES                 */
/**************************************************/

//#define STEPPER_MOTOR_STEP_PIN      14 /* Nano A0 */
//#define STEPPER_MOTOR_DIRECTION_PIN 15 /* Nano A1 */
//#define STEPPER_MOTOR_ENABLE_PIN    16 /* Nano A2 */

#define STEPPER_MOTOR_DIRECTION_PIN A0 /* EasyDriver DIR    */
#define STEPPER_MOTOR_STEP_PIN      A1 /* EasyDriver STEP   */
#define STEPPER_MOTOR_SLEEP_PIN     A3 /* EasyDriver SLP    */
#define STEPPER_MOTOR_ENABLE_PIN     7 /* EasyDriver Enable */
#define STEPPER_MOTOR_MODE_MS1_PIN  A2 /* EasyDriver MS1    */
#define STEPPER_MOTOR_MODE_MS2_PIN   6 /* EasyDriver MS2    */

void initializeStepper() {
    if (debug) Serial.println("d initializeStepper ..");
    pinMode(STEPPER_MOTOR_DIRECTION_PIN, OUTPUT);
    pinMode(STEPPER_MOTOR_STEP_PIN, OUTPUT);
    pinMode(STEPPER_MOTOR_MODE_MS1_PIN, OUTPUT);
    pinMode(STEPPER_MOTOR_MODE_MS2_PIN, OUTPUT);
    pinMode(STEPPER_MOTOR_SLEEP_PIN, OUTPUT);
    // MS1=low,  MS2=low  => Full Step (2 phase)
    // MS1=high, MS2=low  => Half Step
    // MS1=low,  MS2=high => Quarter Step
    // MS1=high, MS2=high => Eighth Step
    digitalWrite(STEPPER_MOTOR_MODE_MS1_PIN, HIGH);    // LOW is fast, HIGH is slow. Default is high (for some strange reason). Pull this high to mess with the timing.
    digitalWrite(STEPPER_MOTOR_MODE_MS2_PIN, LOW);     // LOW is fast, HIGH is slow. Default is high (for some strange reason). Pull this high to mess with the timing.
    digitalWrite(STEPPER_MOTOR_DIRECTION_PIN, HIGH);   // Make it go CW because it's kind of intuitive.
    sleepStepper();
    disableStepper();
    if (debug) Serial.println("d .. initializeStepper");
}

int stepperPinState = 0;

/**
 * Step the stepper once.
 */
void step0() {
    // Just dipping the pin instantaneously seems to be enough for the EasyDriver.
    digitalWrite(STEPPER_MOTOR_STEP_PIN, LOW);
    digitalWrite(STEPPER_MOTOR_STEP_PIN, HIGH);
    /*
    digitalWrite(STEPPER_MOTOR_STEP_PIN, stepperPinState == 1 ? LOW : HIGH);
    stepperPinState = !stepperPinState;
    */
    //delayMicroseconds(STEP_DELAY_US); // Only needed if there is not a delay before step0 called again.
}

/**
 * Wake up the stepper driver (using the SLP / sleep pin).
 */
void wakeStepper() {
    digitalWrite(STEPPER_MOTOR_SLEEP_PIN, HIGH); // Wake stepper driver from sleep
}

/**
 * Put stepper to driver to sleep (using the SLP / sleep pin)
 */
void sleepStepper() {
    digitalWrite(STEPPER_MOTOR_SLEEP_PIN, LOW); // Sleep stepper driver (conserve energy), rest motors.
}

/**
 * Enable the stepper driver (with the EN / enable pin).
 */
void enableStepper() {
    digitalWrite(STEPPER_MOTOR_ENABLE_PIN, LOW); // Enable stepper driver
}

/**
 * Disable the stepper driver (with the EN / enable pin).
 */
void disableStepper() {
    digitalWrite(STEPPER_MOTOR_ENABLE_PIN, HIGH); // disable stepper driver (conserve energy)
}

/**************************************************/
/* LIDAR LITE ROUTINES                            */
/**************************************************/

/**
 * Initialization I2C stuff for the lidar lite.
 */
void initializeLidarLite() {
    if (debug) Serial.println("d InitializeLidarLite ..");
    I2c.begin(); // Opens & joins the I2C bus as master.
    if (debug) Serial.println("d Delay 10ms ..");
    delay(10); // Waits to make sure everything is powered up before sending or receiving data.
    if (debug) Serial.println("d set I2c.timeOut 50 ..");
    I2c.timeOut(I2C_TIMEOUT_MS); // Sets a timeout to ensure no locking up of sketch if I2c communication fails.
    if (debug) Serial.println("d .. initializeLidarLite");
}

/**
 * Request the range from the Lidar Lite (cm)
 * @return range (cm) -1 is error.
 */
int requestLidarLiteRange() {
    //Serial.println("d readLidarLite ..");
    int count = 100;
    int cc = 0;
    while (count > 0 && (cc = I2c.write(LIDARLITE_ADDRESS, REGISTER_MEASURE, MEASURE_VALUE)) != 0) {
        count--;
        //if (debug) Serial.print("W");
        delay(1); // Stop overpolling.
        blinkIfNecessary();
    }
    if (count == 0) {
        if (debug) {
            Serial.print("E I2c.write failed: ");
            Serial.println(cc);
        }
        return cc; // Error
    }
    return 0; // SUCCESS
}

/**
 * Request the range from the Lidar Lite (cm).
 * requestLidarLiteRange must be called before this.
 * @return range (cm) -1 is error. 0 is 'no opinion'. + is range in cm.
 */
int readLidarLiteRange() {
    int count = 100;
    // Generally, this I2c.read will fail 8 times, and work on the 9th - it's a limitation of the LidarLite-V1 (100 times per second).
    // We could set count = 10, and then just drop out in cases where it was going to take longer than that. Maybe for a later experiment.
    while (count > 0 && I2c.read(LIDARLITE_ADDRESS, REGISTER_HIGH_LOW_B, 2) != 0) {
        //DEBUG(Serial.print("R"));
        count--;
        delay(1); // Stop overpolling.
        blinkIfNecessary();
    }
    if (count > 0) {
        lidarErrorsInARow = 0;
        return (I2c.receive() << 8) | I2c.receive();
    }
    lidarErrorsInARow++;
    setBlinkPattern(BLINK_PATTERN_ERROR);
    //return -1; // Error.
    return 0; // Error (no opinion on distance)
}
