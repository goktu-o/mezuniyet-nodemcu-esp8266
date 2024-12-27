#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "Jar_Jar_Linksys"
#define WIFI_PASSWORD "rkf9sXNUPF4n"
#define API_KEY "AIzaSyD1QtzC1xJ0QC2KDQNawpYxp0zc1_eVG3w"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"

#define RED_BUTTON_PIN D1
#define RED_LED_PIN D2
#define BLUE_BUTTON_PIN D5
#define BLUE_LED_PIN D6
#define EXTRA_BUTTON_PIN D7

#define SSL_RX_BUFFER 4096
#define SSL_TX_BUFFER 1024

WiFiClientSecure ssl1, ssl2;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1, result2, result3;
NoAuth noAuth;

bool redLedState = false;
bool blueLedState = false;
bool extraButtonState = false;

unsigned long lastRedButtonPress = 0;
unsigned long lastBlueButtonPress = 0;
unsigned long lastExtraButtonPress = 0;
const unsigned long debounceDelay = 500;

void updateLEDState(const String& path, bool state) {
    if (path == "/red_led") {
        digitalWrite(RED_LED_PIN, state ? HIGH : LOW);
    } else if (path == "/blue_led") {
        digitalWrite(BLUE_LED_PIN, state ? HIGH : LOW);
    }
}

void printResult(AsyncResult &aResult)
{
    if (aResult.available())
    {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream())
        {
            if (RTDB.dataPath() == "/red_led")
            {
                redLedState = RTDB.to<bool>();
                updateLEDState("/red_led", redLedState);
            }
            else if (RTDB.dataPath() == "/blue_led")
            {
                blueLedState = RTDB.to<bool>();
                updateLEDState("/blue_led", blueLedState);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(EXTRA_BUTTON_PIN, INPUT_PULLUP);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
    }

    ssl1.setInsecure();
    ssl2.setInsecure();
#if defined(ESP8266)
    ssl1.setBufferSizes(SSL_RX_BUFFER, SSL_TX_BUFFER);
    ssl2.setBufferSizes(SSL_RX_BUFFER, SSL_TX_BUFFER);
#endif

    initializeApp(client1, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    
    // Set up stream listener with the correct method signature
    Database.get(client1, "/", result1, true);
}

void loop() {
    Database.loop();
    unsigned long currentMillis = millis();

    if (digitalRead(RED_BUTTON_PIN) == LOW && (currentMillis - lastRedButtonPress) > debounceDelay) {
        redLedState = !redLedState;
        updateLEDState("/red_led", redLedState);
        Database.set<bool>(client2, "/red_led", redLedState, result2);
        lastRedButtonPress = currentMillis;
    }

    if (digitalRead(BLUE_BUTTON_PIN) == LOW && (currentMillis - lastBlueButtonPress) > debounceDelay) {
        blueLedState = !blueLedState;
        updateLEDState("/blue_led", blueLedState);
        Database.set<bool>(client2, "/blue_led", blueLedState, result2);
        lastBlueButtonPress = currentMillis;
    }

    if (digitalRead(EXTRA_BUTTON_PIN) == LOW && (currentMillis - lastExtraButtonPress) > debounceDelay) {
        extraButtonState = !extraButtonState;
        Database.set<bool>(client2, "/extra_button", extraButtonState, result3);
        lastExtraButtonPress = currentMillis;
    }

    printResult(result1);
    printResult(result2);
    printResult(result3);
}