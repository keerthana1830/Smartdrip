#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WiFiClientSecure.h>


#include "secrets.h" // Create a local secrets.h from secrets.example.h and do NOT commit it
// ---------- Configuration ----------
// WiFi credentials (override by defining in secrets.h)
#ifndef WIFI_SSID
const char* WIFI_SSID = "YOUR_WIFI_SSID";
#endif
#ifndef WIFI_PASSWORD
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
#endif

// Firebase configuration (Realtime Database) (override by defining in secrets.h)
#ifndef FIREBASE_HOST
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#endif
#ifndef FIREBASE_AUTH
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET_OR_AUTH_TOKEN"
#endif
FirebaseData fbdo;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// Telegram Bot configuration (override by defining in secrets.h)
#ifndef TELEGRAM_BOT_TOKEN
const char* TELEGRAM_BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN"; // e.g., 123456789:ABCdefGHIjklMNOpqrSTUvwxYZ
#endif
#ifndef TELEGRAM_CHAT_ID
const char* TELEGRAM_CHAT_ID = "YOUR_CHAT_ID"; // numeric ID or @username
#endif
const char* TELEGRAM_API_HOST = "api.telegram.org";

// ---------- Pin Definitions ----------
const uint8_t PIN_DRIP_SENSOR = 27;   // IR sensor for drip detection (interrupt)
const uint8_t PIN_LEVEL_SENSOR = 26;  // IR sensor for low fluid level (digital)
const uint8_t PIN_BUZZER = 25;        // Buzzer output
const uint8_t PIN_STATUS_LED = 2;     // Status LED

// ---------- Global State ----------
volatile unsigned long lastDropTimestamp = 0; // ms of last detected drop
volatile unsigned int dropCountMinute = 0;   // drops counted in current minute
unsigned long minuteStartMillis = 0;
unsigned long lastFirebaseUpload = 0;
bool lowLevelAlertSent = false;
bool dripStopAlertSent = false;

// ---------- Helper Functions ----------
void sendTelegramMessage(const String& message) {
    WiFiClientSecure client;
    // Prefer providing `TELEGRAM_ROOT_CERT` in `secrets.h` (PEM) for proper TLS verification.
#ifdef TELEGRAM_ROOT_CERT
    client.setCACert(TELEGRAM_ROOT_CERT);
#else
    client.setInsecure(); // WARNING: Insecure fallback — provide TELEGRAM_ROOT_CERT in secrets.h for secure TLS
#endif
    if (!client.connect(TELEGRAM_API_HOST, 443)) {
        Serial.println("[Telegram] Connection failed");
        return;
    }
    String url = "/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) + "&text=" + message;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + TELEGRAM_API_HOST + "\r\n" +
                 "Connection: close\r\n\r\n");
    // Simple response handling – just wait for server to close connection
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "") break;
    }
    client.stop();
    Serial.println("[Telegram] Message sent");
}

void uploadToFirebase(float dripRate, bool lowLevel, bool alarmActive) {
    if (Firebase.ready()) {
        String path = "/smartdrip"; // root node for this device
        Firebase.setFloat(fbdo, path + "/dripRate", dripRate);
        Firebase.setBool(fbdo, path + "/lowLevel", lowLevel);
        Firebase.setBool(fbdo, path + "/alarmActive", alarmActive);
        // Add timestamp
        Firebase.setUint64(fbdo, path + "/lastUpdate", millis());
        Serial.println("[Firebase] Data uploaded");
    } else {
        Serial.println("[Firebase] Not ready");
    }
}

// ---------- ISR ----------
void IRAM_ATTR dripISR() {
    dropCountMinute++;
    lastDropTimestamp = millis();
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_DRIP_SENSOR, INPUT_PULLUP);
    pinMode(PIN_LEVEL_SENSOR, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_STATUS_LED, LOW);

    // Attach interrupt for drip detection (falling edge when IR beam interrupted)
    attachInterrupt(digitalPinToInterrupt(PIN_DRIP_SENSOR), dripISR, FALLING);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.println(" connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Firebase setup
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.auth = FIREBASE_AUTH;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);

    minuteStartMillis = millis();
    lastFirebaseUpload = millis();
}

void loop() {
    unsigned long currentMillis = millis();

    // ----- Check low fluid level -----
    bool lowLevel = digitalRead(PIN_LEVEL_SENSOR) == LOW; // assuming active LOW when fluid low
    if (lowLevel) {
        digitalWrite(PIN_BUZZER, HIGH);
        digitalWrite(PIN_STATUS_LED, HIGH);
        if (!lowLevelAlertSent) {
            sendTelegramMessage("⚠️ SmartDrip: Low fluid level detected!");
            lowLevelAlertSent = true;
        }
    } else {
        lowLevelAlertSent = false; // reset for next event
    }

    // ----- Check drip stop (no drops for 10 seconds) -----
    bool dripStopped = (currentMillis - lastDropTimestamp) > 10000UL; // 10,000 ms
    if (dripStopped) {
        digitalWrite(PIN_BUZZER, HIGH);
        digitalWrite(PIN_STATUS_LED, HIGH);
        if (!dripStopAlertSent) {
            sendTelegramMessage("⏱️ SmartDrip: No drops detected for 10 seconds!");
            dripStopAlertSent = true;
        }
    } else {
        // If drops are occurring, clear alarm (but keep buzzer for low level if applicable)
        if (!lowLevel) {
            digitalWrite(PIN_BUZZER, LOW);
            digitalWrite(PIN_STATUS_LED, LOW);
        }
        dripStopAlertSent = false;
    }

    // ----- Compute drip rate per minute -----
    if (currentMillis - minuteStartMillis >= 60000UL) { // 60 seconds elapsed
        float ratePerMinute = (float)dropCountMinute; // drops counted in the last minute
        // Reset counters for next minute
        dropCountMinute = 0;
        minuteStartMillis = currentMillis;
        Serial.printf("[Info] Drop rate: %.2f drops/min\n", ratePerMinute);
        // Upload immediately after computing rate
        uploadToFirebase(ratePerMinute, lowLevel, dripStopped || lowLevel);
    }

    // ----- Periodic Firebase upload every 5 seconds -----
    if (currentMillis - lastFirebaseUpload >= 5000UL) {
        // Approximate current rate based on drops in last 60 sec window
        float approxRate = (float)dropCountMinute * (60.0 / ((currentMillis - minuteStartMillis) / 1000.0));
        uploadToFirebase(approxRate, lowLevel, dripStopped || lowLevel);
        lastFirebaseUpload = currentMillis;
    }
}
