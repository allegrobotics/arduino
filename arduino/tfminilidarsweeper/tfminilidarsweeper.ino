//-*- mode: c -*-
/**
 * FILE
 *     tfminilidarsweeper.ino
 * PURPOSE
 *     Arduino Nano controlling a TF Mini Lidar on a rotating turret. http://allegrobotics.com/lidarLiteSweeper.html
 * COPYWRIGHT
 *     Scott BARNES 2018. IP freely on non-commercial applications.
 * DETAIL
 *     Setup consists of 
 *     1. TF Mini Lidar mounted on a slip ring and connected to an input pin, and communicating at 19200 baud.
 *     2. Steeper motor to rotate the lidar through 360 degrees, controlled by an Easy Driver.
 *     3. A photointerrupter (or switch or something) which gets set once per revolution.
 * PHILOSOPHY
 * This is not presented as a C++ module, which can be integrated with other modules as it is unlikely to play well with modules - the timing is too tight.
 * This application is actually a bit of challenge with just an Arduino Nano.
 * It would be a lot easier with a Mega (or Mega clone), but part of the challenge is to use the smallest and cheapest device available to man, beast or bot.
 * This even works with the cheap Nanos (with the non-buffered USB serial converters) from Sum Ting Wong (my favourite supplier), which are available through eBay for around $AU3.30 each.
 * SEE
 * http://allegrobotics.com/lidarLiteSweeper.html
 * http://allegrobotics.com/tfMiniLidar.html
 */

#include <Arduino.h>

/**
 * This is the pin layout. It looks very illogical. It is. It was designed around the physical placement, which optimized for space on a V-board to fit into the smallest space.
 */
const int ledPin = 13;                 // Not much choice here - it's what the Nano does.
const int photointerrupterPin = 4;     // The pin to read the status of the photointerrupter.
const int serialReadPin = 5;           // The pin to read from the TF Mini Lidar at 19200 baud.
const int stepPin = 11;                // Easy Driver STEP pin
const int directionPin = 12;           // Easy Driver DIR pin
const int enablePin = 7;               // Easy Driver ENABLE pin
const int sleepPin = 2;                // Easy Driver SLP pin
const int ms1Pin = 3;                  // Easy Driver MS1 pin
const int ms2Pin = 8;                  // Easy Driver MS2 pin


// Precomputed memory positions to do fast reads of the photointerrupter and serial read ports.
volatile uint8_t *photointerrupterPinRegister;
uint8_t photointerrupterPinBitmask;
volatile uint8_t *serialReadPinRegister;
uint8_t serialReadPinBitmask;

byte stepperPinState = 0;              // Current state of stepPin. This alternates to raise and lower the STP pin to step the motor.

const int serialInputBaudRate = 19200; // Baud rate of input stream from Lidar. This MUST be changed from the default 115200 because 115200 is just too fast for a Nano.
int cyclesPerBit;                      // Number of clock cycles per bit of serial input. Set to "(int) (clockCyclesPerMicrosecond() * (uint32_t) 1000000 / serialInputBaudRate)"
int serialReadState = 0;               // [0..38] What bit we are expecting to read in the serial input stream byte.
                                       // 0 => looking for falling edge signifying startBit;
                                       // 5  => time to sample bit 1
                                       // 9 => time to sample bit 2, 13=>b2; 17=>b3; 21=>b4; 25=>b5; 29=>b6; 33=>b7; 37=>sb; 38=>err_scan_hi

volatile int turretAngle = 0;          // Gets reset at start point. Half-pulses, which may be 1/2 step 1/4 step 1/8 step 1/16 step depending on microstepping.
volatile char cubby;                   // The most recent character read from the lidar serial input stream gets dumped here.
volatile char cubbyFull = 0;           // This is set when a new char is dumped in cubby. It is unset when it is dealt with.

byte byteBeingRead = 0;                // Byte currently being read by sampler.
byte previousInterrupterState = 0;     // The state of the photointerrupter on the last read.
int stepperTimer = 0;                  // Counts 1/4th of bits. Motor is stepped when it gets to zero, and then it gets set back to 'stepperSpeed', and is decremented every interrupt tick.

// These two determine the speed of the stepper motor (ie rotational speed of the turret)
int stepperSpeed = 25;                // Number of 1/4 of bit between toggles of the stepper driver step pin. (smaller number is faster speed)
const byte microsteppingLog = 2;       // [0 => no microstepping (ie fastest); 1 => 2:1 microstepping; 2 => 4:1 microstepping; 3 => 8:1 microstepping (ie slowest)
                                       // microsteppingLog  ms1  ms2
                                       // 0                0    0
                                       // 1                1    0
                                       // 2                0    1
                                       // 3                1    1

////////////////
// BLINKER
////////////////
uint32_t nextBlinkAt = 0L;
uint32_t blinkMask = 0x00000501;
byte blinkPointer = 0;

/**
 * Blinks the LED to convey the status.
 */
void blinkIfNecessary(uint32_t now) {
    if (now >= nextBlinkAt) {
        blinkPointer = (blinkPointer + 1) % 32;
        digitalWrite(ledPin, (blinkMask & (0x01 << blinkPointer)) != 0);
        nextBlinkAt += 80; // 80ms works pretty well
    }
}

// The interrupt service routine.
// It is called whenever the timer goes off. ie often. Actually around every 208 clock cycles.
ISR(TIMER1_COMPA_vect) {
    // This code is time efficient, not space efficient. Bear that in mind before opining.
    // We have 208 clock cycles to to a bunch of stuff here or our little Nano world falls apart.

    // We need the values of the serialReadPin and photointerrupterPins.
    // In principle, we should be able to do this with following two commands ...
    // byte value = digitalRead(serialReadPin);
    // byte interrupterState = digitalRead(photointerrupterPin);
    // ... but that fails (either because it's too slow or the calls mess with the timer .. not sure which)
    // Instead we do it with these two quick little incantations:
    byte value = ((*serialReadPinRegister) & serialReadPinBitmask) ? 1 : 0;
    byte interrupterState = ((*photointerrupterPinRegister) & photointerrupterPinBitmask) ? 1 : 0;
    
    // Do we need to reset the turret angle to zero?
    if (previousInterrupterState > interrupterState) { // previous state 1, current state 0 means we have gone to zero.
        turretAngle = 0;
    }
    previousInterrupterState = interrupterState;

    // Do we need to toggle the the 'step' pin?
    if (stepperTimer <= 0) {
        stepperTimer += stepperSpeed;
        digitalWrite(stepPin, stepperPinState = !stepperPinState); // Yes, we change state and get value, not != :).
        turretAngle++;
    } else
        stepperTimer--;

    // Do we have new data on the serial input from the Lidar?
    serialReadState++;
    switch (serialReadState) {
    case 1: // Check for falling edge
        if (value)
            serialReadState = 0; // We are still high - no falling edge, no start bit.
        break;
    case 6: // expecting 1st bit
        byteBeingRead = value;  // << 0;
        break;
    case 10: // expecting 2nd bit
        byteBeingRead |= value << 1;
        break;
    case 14: // expecting 3rd bit
        byteBeingRead |= value << 2;
        break;
    case 18: // expecting 4th bit
        byteBeingRead |= value << 3;
        break;
    case 22: // expecting 5th bit
        byteBeingRead |= value << 4;
        break;
    case 26: // expecting 6th bit
        byteBeingRead |= value << 5;
        break;
    case 30: // expecting 7th bit
        byteBeingRead |= value << 6;
        break;
    case 34: // expecting 8th bit
        byteBeingRead |= value << 7;
        break;
    case 38: // Expecting stop bit, which should be 1
        if (value) {
            cubby = byteBeingRead;
            cubbyFull = 1;
            //            goodByteCount++;
        } else // WTF? Stop bit is 0! Keep reading until we get 1.
            //            badByteCount++;
            ;
    case 39: // We have read a LOW stop bit, so we keep reading until we see HIGH to try to resynchronize.
        if (value)
            serialReadState = 0; // Finally! Now we can look for next start bit.
        break;
    }
}

/**
 * Set up the time interrupt on TIMER1.
 * These incantations are copied from the interweb.
 */
void setupInterrupt(int ocr1a) {
    cli();      // disable global interrupts
    TCCR1A = 0;     // set entire TCCR1A register to 0
    TCCR1B = 0;     // same for TCCR1B
    TCNT1  = 0; // initialize counter value to 0
    OCR1A = ocr1a;           // Clock cycles between interrupts.
    TCCR1B |= (1 << WGM12);  // turn on CTC mode:
    TCCR1B |= (1 << CS10);   // Set CS10 bit for no prescaler:
    TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt:
    sei();      // enable global interrupts
}

// We want to send _either_ binary or text to the host.
byte binaryHostPacket[5]; // The binary packet to send to the host.
byte textHostPacket[8];   // The text packet to send to the host.

/**
 * Called once at startup.
 */
void setup() {
    delay(2000);                                                        // Delay before anything starts, because that's good practice.
    Serial.begin(9600);                                                 // Communicate back to the host ... nice ... and ... slow.
    pinMode(ledPin, OUTPUT);                                            // BLINKER
    pinMode(serialReadPin, INPUT);
    pinMode(photointerrupterPin, INPUT_PULLUP);

    // Pins to control the EasyDriver
    pinMode(stepPin, OUTPUT);                                           // STEP
    pinMode(directionPin, OUTPUT); digitalWrite(directionPin, HIGH);    // DIR
    pinMode(enablePin, OUTPUT); digitalWrite(enablePin, LOW);           // EN
    pinMode(sleepPin, OUTPUT); digitalWrite(sleepPin, HIGH);            // SLP
    pinMode(ms1Pin, OUTPUT); digitalWrite(ms1Pin, microsteppingLog &  1 ? HIGH : LOW); // MS1
    pinMode(ms2Pin, OUTPUT); digitalWrite(ms2Pin, microsteppingLog >> 1 ? HIGH : LOW); // MS2

    // Precompute memory positions for photointerrupter and serial read ports so we avoid having to call digitalRead() inside the interrupt.
    photointerrupterPinRegister = portInputRegister(digitalPinToPort(photointerrupterPin));
    photointerrupterPinBitmask = digitalPinToBitMask(photointerrupterPin);
    serialReadPinRegister = portInputRegister(digitalPinToPort(serialReadPin));
    serialReadPinBitmask = digitalPinToBitMask(serialReadPin);

    Serial.print("photointerrupterPinRegister="); Serial.println((int) photointerrupterPinRegister);
    Serial.print("photointerrupterPinBitmask="); Serial.println((int) photointerrupterPinBitmask);
    Serial.print("serialReadPinRegister="); Serial.println((int) serialReadPinRegister);
    Serial.print("serialReadPinBitmask="); Serial.println((int) serialReadPinBitmask);

    binaryHostPacket[0] = 0xFA; // First byte of every binary host packet.
    cyclesPerBit = (int) (clockCyclesPerMicrosecond() * (uint32_t) 1000000 / serialInputBaudRate);
    Serial.print("clockCyclesPerMicrosecond() = "); Serial.println(clockCyclesPerMicrosecond());
    Serial.print("cyclesPerBit = "); Serial.println(cyclesPerBit);
    delay(100); // TESTING ONLY.
    setupInterrupt(cyclesPerBit / 4);
}

uint32_t nextDebugMessageAt = 0L;

byte lidarPacket[9];                     // Packet being receieved from lidar.
byte lidarPacketPopulation = 0;          // Number of bytes currently in packet.
int recentLidarDistanceCm = 0;           // The most recent distance read from lidar.
int recentLidarStrength = 0;             // The most recent strength read from lidar.
uint32_t lastLidarPacketReceivedAt = 0L; // The time (ms) the most recent well-formed packet was read from lidar.
int packetChecksum = 0;                  // Checksum of bytes in packet so far.
int goodPacketCount = 0; // DEBUGGING
int badPacketCount = 0; // DEBUGGING

/**
 * process the next input byte from the serialPort (if one is available)
 * @return non-zero if a new (well-formed) packet is now available.
 */
int processNextInputByte(uint32_t now) {
    if (cubbyFull == 0) // No byte to read yet.
        return 0; // Nothing to do.
    // So cubbyFull is true, meaning have a new byte to process.
    register byte c = cubby; // Assume we have been quick enough to grab the most recent byte.
    cubbyFull = 0;
    if (lidarPacketPopulation == 0)
        packetChecksum = 0;
    lidarPacket[lidarPacketPopulation++] = c;
    if (lidarPacketPopulation < 3 && c != 0x59)
        lidarPacketPopulation = 0; // wrong header(s), start again.
    else if (lidarPacketPopulation == 9) {
        lidarPacketPopulation = 0;
        if ((c & 0x00ff) == (packetChecksum & 0x00ff)) { // We have a good packet.
            int distanceCm = lidarPacket[3] << 8 | (0x00ff & lidarPacket[2]);
            int strength = lidarPacket[5] << 8 | (0x00ff & lidarPacket[4]);
            recentLidarDistanceCm = distanceCm;
            recentLidarStrength = strength;
            lastLidarPacketReceivedAt = now;
            //blinker.setBlinkPattern(0x00000505);
            goodPacketCount++;
            if (goodPacketCount % 200 == 0) {
                Serial.print(" lidar(");
                Serial.print(lidarPacket[0]);
                Serial.print(",");
                Serial.print(lidarPacket[1]);
                Serial.print(",");
                Serial.print(lidarPacket[2]);
                Serial.print(",");
                Serial.print(lidarPacket[3]);
                Serial.print(",");
                Serial.print(lidarPacket[4]);
                Serial.print(",");
                Serial.print(lidarPacket[5]);
                Serial.print(",");
                Serial.print(lidarPacket[6]);
                Serial.print(",");
                Serial.print(lidarPacket[7]);
                Serial.print(",");
                Serial.print(lidarPacket[8]);
                Serial.println(")");
            }
            return 1; // We have a new packet.
        } else
            badPacketCount++;
    }
    packetChecksum += c;
    return 0; // no new packet.
}

uint32_t myCounter = 0;
byte hex[] = "0123456789ABCDEF";

void loop() {
    //byte interrupterState = ((*photointerrupterPinRegister) & photointerrupterPinBitmask) ? 1 : 0;
    //digitalWrite(ledPin, interrupterState);
    uint32_t now = millis();
    blinkIfNecessary(now);
    if (processNextInputByte(now)) {
        // We have a choice here - send a 5 byte binary packet:  HEADER TURRET_ANGLE DISTANCE_5CM STRENGTH CHECKSUM.
        // Or an 6 byte hex printable: TURRET_ANGLE DISTANCE_5M STRENGTH followed by CR LF.
        // We create a new binary host packet - we just assume there would have been time to send the previous one.
        binaryHostPacket[1] = turretAngle >> (microsteppingLog + 1);
        binaryHostPacket[2] = recentLidarDistanceCm / 5; // 5cm distance.
        binaryHostPacket[3] = recentLidarStrength / 8;
        binaryHostPacket[4] = (byte) (0xFA + binaryHostPacket[1] + binaryHostPacket[2] + binaryHostPacket[3]);
        //Serial.write(binaryHostPacket, 5);

        // Send a new printable host packet.
        textHostPacket[0] = hex[binaryHostPacket[1] >> 4];
        textHostPacket[1] = hex[binaryHostPacket[1] & 0x0F];
        textHostPacket[2] = hex[binaryHostPacket[2] >> 4];
        textHostPacket[3] = hex[binaryHostPacket[2] & 0x0F];
        textHostPacket[4] = hex[binaryHostPacket[3] >> 4];
        textHostPacket[5] = hex[binaryHostPacket[3] & 0x0F];
        textHostPacket[6] = '\r';
        textHostPacket[7] = '\n';
        Serial.write(textHostPacket, 8);
    }
    byte turretGood = (turretAngle >> (microsteppingLog + 1)) > -10 || (turretAngle >> (microsteppingLog + 1)) < 220;
    byte lidarGood = lastLidarPacketReceivedAt + 100 > now;
    blinkMask = turretGood
        ? lidarGood ? 0x00000505 : 0x00001015
        : lidarGood ? 0x00005015 : 0x00015015;
    // Okay, if we enable this stuff, then the host has to be smart enough to know that a line starting with "D " is a debug line.
    if (now > nextDebugMessageAt) {
        Serial.print("D goodPacketCount=");
        Serial.print(goodPacketCount);
        Serial.print(" badPacketCount=");
        Serial.print(badPacketCount);
        Serial.print(" photointerrupterPin=");
        Serial.print(digitalRead(photointerrupterPin));
        Serial.print(" recentLidarDistanceCm=");
        Serial.print(recentLidarDistanceCm);
        Serial.print(" turretAngle=");
        Serial.println(turretAngle >> (microsteppingLog + 1));
        nextDebugMessageAt += 500;
    }
}
