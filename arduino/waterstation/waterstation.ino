//-*- mode: c -*-
/**
 * FILE
 *     waterstation.ino
 * PURPOSE
 *    Turns a solenoid on and off at a rover's request to enable watering.
 *    Assumes we are using either a LCTECH "ESP8266 RELAY V3" (ESP-01 on a single relay board) or a LCTECH "X2" (ESP-01 on a dual relay board).
 * HTTP REQUESTS
 *    "GET /water/on HTTP/1.0"      (water please)
 *    "GET /water/off HTTP/1.0"     (stop water)
 *    "GET /anything/else HTTP/1.0" (return a status page)
 * PIN MAPPING ON THE ESP-01
 *    Different ESPs have different (and weird) pin mapping.
 *    In this case we are using the TX pin to send to the board controller.
 * RULES
 *    0. Off/restart/resting position is inactive relay, no water flows.
 *    1. The rover makes a "GET /water/on" request to start filling, and relay is activated to provide water.
 *    2. The rover must keep making "GET /water/on" requests at least every SQUIRT_DURATION_MS milliseconds (10s?), or the relay inactivates and the water shuts off.
 *    3. If the rover misses an ON request and the water shuts off, that's okay, the next ON request will restart the flow, but ..
 *    4. If the rover takes longer than MAX_FILL_TIME_MS to fill, it gets locked out for LOCKOUT_DURATION_MS.
 *    5. If the rover ends a fill using "GET /water/off" it gets locked out for LOCKOUT_DURATION_MS.
 *    6. If the LOCKOUT is on, any "GET /water/on" request starts the lockout period all over again.
 * BLINKING
 *    Blinking may be DISABLED if the pin is required (eg LED sharing with TX line, which controls the relays).
 *    1+2 STATE_IDLE           waiting for a fill request.
 *    2+1 STATE_FILLING        water is running.
 *    2+2 STATE_FILL_SUSPENDED because we didn't receive a fill request request recently.
 *    1+3 STATE_LOCKOUT        timeout until we fill again.
 * COPYRIGHT
 *    Scott BARNES. 2018 IP freely on non-commercial applications.
 * MQTT
 *    This is not really suited to MQTT, as the rover wants a status value on a '/water/on' request immediately.
 *    Hence MQTT is just used for logging to the mosquito server.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

////////////////
// DEBUG
////////////////

#define DEBUG(x)

////////////////
// BLINKER
// Let the human know that all is well ... or not.
// The older ESP-01s have the built-in LEDs sharing the TX line, and TX is required for relay operation on the X2, so they are really not compatible.
////////////////

int ledPin = 0; // 1 is the LED, but it is actually TX, so making this 1 means that Serial.print*() will fail.

#define BLINK_PATTERN_12 0x00000501 // 1 blink  then 2
#define BLINK_PATTERN_13 0x00001501 // 1 blink  then 3
#define BLINK_PATTERN_21 0x00000105 // 2 blinks then 1
#define BLINK_PATTERN_22 0x00000505 // 2 blinks then 2
#define BLINK_PATTERN_23 0x00001505 // 2 blinks then 3
#define BLINK_PATTERN_31 0x00000415 // 3 blinks then 1
#define BLINK_PATTERN_32 0x00000C15 // 3 blinks then 2
#define BLINK_PATTERN_33 0x00001C15 // 3 blinks then 3

uint32_t blinkMask = BLINK_PATTERN_12;
#define NUM_BITS_IN_BLINKMASK 24

byte currentBlinkMaskBit = 0;
uint32_t blinkNextChangeAt = 0L;
#define BLINK_RATE_MS 83

////////////////
// WIFI
////////////////

// Replace with your network credentials
const char* ssid     = "MYNETWORKNAME";
const char* password = "MYNETWORKPASWORD";

////////////////
// BLINKER
// Built-in LED on ESP-01 seems to share with TX, so we have to choose.
////////////////

void setupBlinker() {
    pinMode(ledPin, OUTPUT);     // Initialize the ledPin as an output
}

/**
 * Flashes the LED if it's time has come.
 */
void loopBlinker(uint32_t now) {
    if (now < blinkNextChangeAt)
        return;
    currentBlinkMaskBit = (currentBlinkMaskBit + 1) % NUM_BITS_IN_BLINKMASK;
    digitalWrite(ledPin, (blinkMask >> currentBlinkMaskBit) & 0x01 ? LOW : HIGH);
    blinkNextChangeAt = now + BLINK_RATE_MS;
}

////////////////
// RELAY OPERATION - LCTech "ESP8266 RELAY V3" or "X2"
// The X2 actually has two relays, but we assume one for this code.
// This board works by having the ESP-0 send serial instructions at 115200baud out on the TX line.
////////////////

void setupRelay() {
    Serial.begin(9600); // Use this for LCTECH "ESP8266 RELAY V3" single relay board.
    //Serial.begin(115200); // Use this for LCTECH "X2" 2-relay board.
}

void loopRelay() {
}

void setRelay0(int relay, int state) {
    byte openClose = state == 0 ? 0x00 : 0x01;
    Serial.write(0xA0);                         // Packet header.
    Serial.write(relay + 1);                    // 0x01 is first relay, 0x02 is second relay, etc.
    Serial.write(openClose);                    // 0x00 is close, 0x01 is open.
    Serial.write(0xA0 + relay + 1 + openClose); // Checksum.
}

/**
 * Special protocol for relay setting on the LCTECH boards.
 * @param relay [0, 1, 2 ...] If only one relay, use 0
 * @param state 0=>open, 1=>close
 */
void setRelay(int relay, int state) {
    // Yes this is weird logic, but I saw someone else do it for this board, suggesting that 115200 communication with the
    // relay board is less that 100% reliable, so here goes ..
    setRelay0(relay, state); // Do it
    delay(2);                // Let stuff settle
    setRelay0(relay, state); // Do it again in case the first "do it" didn't.
}

////////////////
// WiFi
////////////////

IPAddress mqttBroker(192, 168, 1, 128); // mosquitto.livesoftware.com.au
IPAddress localIp(192, 168, 1, 176);    // waterstation.livesoftware.com.au
IPAddress gateway(192, 168, 1, 1);      // local adsl router
IPAddress subnet(255, 255, 255, 0);     // 24/8 split.

/**
 * Connect to the local WiFi, with the local IP address.
 */
void setupWiFi() {
    WiFi.config(localIp, gateway, subnet);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG(Serial.print("."));
    }
    // Print local IP address and start web server
    DEBUG(Serial.println("I WiFi connected."));
    DEBUG(Serial.println("I IP address: "));
    DEBUG(Serial.println(WiFi.localIP()));
    // Connect to Wi-Fi network with SSID and password
    DEBUG(Serial.print("I Connecting to "));
    DEBUG(Serial.println(ssid));
}

void loopWiFi(uint32_t now) {
    // Does nothing.
}

/////////////////////////
// MQTT
// Just used for logging.
// We don't subscribe to anything, just send logging info.
/////////////////////////

WiFiClient wiFiClient;
PubSubClient mqttClient(wiFiClient);

/**
 * We call this to publish a message.
 * @message the message to send.
 */
void mqttPublish(char *message) {
    mqttClient.publish("/waterstation", message);
}

/**
 * Not actually used.
 * Just here for posterity.
 * @param topic
 * @param payload
 * @param length
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [\"");
    Serial.print(topic);
    Serial.print("\" \"");
    for (int i = 0; i < length; i++)
        Serial.print((char) payload[i]);
    Serial.println("\"]");
}

/**
 * @return 0==OKAY, -1==FAIL
 */
int mqttReconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect("WaterStation")) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            mqttPublish("reconnected");
            // ... and resubscribe
            //mqttClient.subscribe("inTopic"); don't subscribe to anything - we just log our events.
            return 0; // OKAY
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 3 seconds before retrying
            delay(3000);
        }
    }
    return -1; // FAIL
}

void setupMqtt() {
    mqttClient.setServer(mqttBroker, 1883);
    mqttClient.setCallback(mqttCallback);
}

void loopMqtt(uint32_t now) {
    if (!mqttClient.connected())
        mqttReconnect();
    if (mqttClient.connected())
        mqttClient.loop(); // May not be called if we couldn't connect.
}

////////////////
// FILL LOGIC
// We don't just open and close the relay on instruction.
// We check timeouts of relays etc to avoid catostrophic water loss on the farm, so this is a state machine.
////////////////

#define MAX_FILL_TIME_MS    (1000 * 60 * 10)  // If we haven't finished in this time, we have cast our seed upon the ground.
#define SQUIRT_DURATION_MS  (1000 * 10)       // We must get another water request within this time, or we stop the water.
#define LOCKOUT_DURATION_MS (1000 * 10 * 10)  // When we lock-down, we do it for this long.

// State variables
#define STATE_IDLE           0                // Anyone can start filling.
#define STATE_FILLING        1                // We are filling, tap is on.
#define STATE_FILL_SUSPENDED 2                // We timed out (didn't get a request for 10s), but will start again if asked.
#define STATE_LOCKOUT        3                // We won't give any in this state, and requests prolong the lockout.

int state = STATE_IDLE;

uint32_t suspendSquirtAt   = 0L;
uint32_t fillDeadlineAt    = 0L;
uint32_t lockoutEndsAt     = 0L;

/**
 * We have received a water request.
 * @param now what millis() recently returned.
 */
char *requestWater(uint32_t now) {
    switch (state) {
    case STATE_IDLE:
        state = STATE_FILLING;
        blinkMask = BLINK_PATTERN_12;
        setRelay(0, 1);
        suspendSquirtAt = now + SQUIRT_DURATION_MS;
        fillDeadlineAt = now + MAX_FILL_TIME_MS;
        mqttPublish("OKAY Started filling");
        return "OKAY Started filling";
        break;
    case STATE_FILL_SUSPENDED:
        state = STATE_FILLING;
    case STATE_FILLING:
        setRelay(0, 1);
        blinkMask = BLINK_PATTERN_22;
        suspendSquirtAt = now + SQUIRT_DURATION_MS;
        mqttPublish("OKAY Continuing filling");
        return "OKAY Continuing filling";
        break;
    case STATE_LOCKOUT:
        setRelay(0, 0);
        lockoutEndsAt = now + LOCKOUT_DURATION_MS; // We are getting bad requests, so for safety, keep delaying.
        mqttPublish("FAIL Locked out");
        return "FAIL Locked out";
        break;
    }
}

/**
 * We have received a request to stop the water.
 * @param now what millis() recently returned.
 */
char *fillComplete(uint32_t now) {
    switch (state) {
    case STATE_IDLE:
        mqttPublish("OKAY But I wasn't filling");
        setRelay(0, 0);
        return "OKAY But I wasn't filling";
        // This is just weird, and is to be ignored.
        break;
    case STATE_FILLING:
    case STATE_FILL_SUSPENDED:
        setRelay(0, 0);
        state = STATE_LOCKOUT;
        blinkMask = BLINK_PATTERN_31;
        lockoutEndsAt = now + LOCKOUT_DURATION_MS;
        mqttPublish("OKAY Lockout begun");
        return "OKAY Lockout begun";
        break;
    case STATE_LOCKOUT:
        setRelay(0, 0);
        state = STATE_LOCKOUT;
        blinkMask = BLINK_PATTERN_31;
        lockoutEndsAt = now + LOCKOUT_DURATION_MS;
        mqttPublish("OKAY But you were locked out anyway");
        return "OKAY But you were locked out anyway";
        break;
    }
}

/**
 * Called periodically to see if the timeouts should be active.
 */
void checkFillTimeout(uint32_t now) {
    switch (state) {
    case STATE_IDLE:
        // Do nothing.
        break;
    case STATE_FILLING:
        // We might have reached either the fill deadline or the squirt deadline.
        if (now >= fillDeadlineAt) {
            state = STATE_LOCKOUT;
            blinkMask = BLINK_PATTERN_31;
            setRelay(0, 0);
            lockoutEndsAt = now + LOCKOUT_DURATION_MS;
            mqttPublish("WARN Deadline passed, lockout started");
        } else if (now >= suspendSquirtAt) {
            state = STATE_FILL_SUSPENDED;
            blinkMask = BLINK_PATTERN_22;
            setRelay(0, 0);
        }
        break;
    case STATE_FILL_SUSPENDED:
        // Check that the fill deadline hasn't gone overtime.
        if (now >= fillDeadlineAt) {
            state = STATE_LOCKOUT;
            mqttPublish("WARN Deadline passed, lockout started");
            blinkMask = BLINK_PATTERN_31;
            setRelay(0, 0);
            lockoutEndsAt = now + LOCKOUT_DURATION_MS;
        }
        break;
    case STATE_LOCKOUT:
        // The lockout time might have ended.
        if (now >= lockoutEndsAt) {
            state = STATE_IDLE;
            mqttPublish("INFO Lockout ended");
            blinkMask = BLINK_PATTERN_21;
        }
        break;
    }
}

////////////////
// HTTPd
////////////////

WiFiServer server(80); // Set web server port number to 80

/**
 * Display the HTML web page
 * @param httpClient where to write to.
 * @param contentType one of "text/html" or "text/plain"
 * @param body the body of the HTTP message returned to the client.
 */
void writeHttpResponse(WiFiClient httpClient, char *contentType, String body) {
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    httpClient.println("HTTP/1.1 200 OK");
    httpClient.print("Content-type: ");
    httpClient.println(contentType);
    httpClient.println("Connection: close");
    httpClient.println();
    httpClient.println(body);
    httpClient.println(); // The HTTP response ends with another blank line
}

/**
 * Process a full HTTP request.
 * The headers lines (ie the 'Name: Value' lines) are not available (just the first line).
 * @param now what millis() recently returned.
 * @param header just the first header line, ie the "GET ...", "POST ..." line.
 * @param httpClient where to write response to.
 */
void processHttpRequest(uint32_t now, String header, WiFiClient httpClient) {
    DEBUG(Serial.print("I "));
    DEBUG(Serial.println(header));
    // Turns the wireless switches on and off
    if (header.indexOf("GET /test/") >= 0) {
        int pin = header[10] - '0';
        Serial.print("testing pin "); Serial.println(pin);
        pinMode(pin, OUTPUT);
        Serial.print("going HIGH ..");
        digitalWrite(pin, HIGH);
        delay(200);
        Serial.print("going LOW ..");
        digitalWrite(pin, LOW);
        delay(800);
        Serial.print("test complete");
        writeHttpResponse(httpClient, "text/plain", "test complete");
    } else if (header.indexOf("GET /water/on") >= 0) {
        writeHttpResponse(httpClient, "text/plain", requestWater(now));
    } else if (header.indexOf("GET /water/off") >= 0) {
        writeHttpResponse(httpClient,  "text/plain", fillComplete(now));
    } else {
        String body = "<html><head><title>WaterStation</title></head><body><h1>Allegrobotics Water Station</h1><ul><li><a href='/water/on'>Water on</a> - give a ";
        body += (SQUIRT_DURATION_MS / 1000);
        body += "s squirt of water</li><li><a href='/water/off'>Water off</a> - stop the squirt</li><li>now ";
        body += now;
        body += "</li><li>state ";
        body += state == STATE_IDLE ? "IDLE"
            : state == STATE_FILLING ? "FILLING"
            : state == STATE_FILL_SUSPENDED ? "FILL_SUSPENDED"
            : state == STATE_LOCKOUT ? "LOCKOUT"
            : "UNKNOWN";
        body += "</li><li>fillDeadlineAt ";
        body += fillDeadlineAt;
        body += "</li><li>lockoutEndsAt ";
        body += lockoutEndsAt;
        body += "</li><li>suspendSquirtAt ";
        body += suspendSquirtAt;
        body += "</li></ul></body></html>";
        writeHttpResponse(httpClient, "text/html", body);
    }
}

/**
 * Called once, by setup() to set up the HTTP daemon.
 */
void setupHttpd() {
    server.begin();
}

WiFiClient httpClient;
String header = "";
String currentLine = "";

/**
 * @param now what millis() recently returned.
 */
void loopHttpd(uint32_t now) {
    if (!httpClient) {
        httpClient = server.available();   // Listen for incoming clients
        if (!httpClient)
            return;
    }
    if (!httpClient.connected()) {
        httpClient.stop();
        return;
    }
    // httpClient is connected.
    while (httpClient.available()) {
        char c = httpClient.read();        // Will not block
        header += c;
        if (c == '\n') {
            if (currentLine.length() == 0) {
                // Two newline characters in a row, ie end of the httpClient HTTP GET request. Process this.
                processHttpRequest(now, header, httpClient);
                header = "";
                currentLine = "";
                httpClient.stop();
                //httpClient = NULL;
                DEBUG(Serial.println("I Client disconnected."));
                return;
            } else                         // Got a newline -> clear currentLine
                currentLine = "";
        } else if (c != '\r')              // Anything else (except carriage return).
            currentLine += c;              // Append to currentLine
    }
}

/////////////////////////
// MAIN
/////////////////////////

void setup() {
    delay(2000);
    setupRelay();
    setupBlinker();
    setupWiFi();
    setupHttpd();
    setupMqtt();
    delay(1500); // Let hardware sort itself out (suggested by MQQT example).
}

void loop() {
    uint32_t now = millis();
    loopBlinker(now);
    loopWiFi(now);
    loopHttpd(now);
    loopMqtt(now);
    loopRelay();
    checkFillTimeout(now);
}
