#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

// Wi-Fi ve Firebase bilgileri
#define WIFI_SSID "Jar_Jar_Linksys"
#define WIFI_PASSWORD "rkf9sXNUPF4n"

#define API_KEY "AIzaSyD1QtzC1xJ0QC2KDQNawpYxp0zc1_eVG3w"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"

#define RED_BUTTON_PIN D1
#define RED_LED_PIN D2
#define BLUE_BUTTON_PIN D5
#define BLUE_LED_PIN D6
#define EXTRA_BUTTON_PIN D7  // New button pin

WiFiClientSecure ssl1, ssl2;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1, result2, result3;  // Added result3 for the new button
NoAuth noAuth;

bool redLedState = false;
bool blueLedState = false;
bool extraButtonState = false;  // State for the new button

unsigned long lastRedButtonPress = 0;
unsigned long lastBlueButtonPress = 0;
unsigned long lastExtraButtonPress = 0;  // Debounce time for the new button
const unsigned long debounceDelay = 500;  // Updated debounce delay

void printResult(AsyncResult &aResult)
{
    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream())
        {
            Serial.println("----------------------------");
            Firebase.printf("task: %s\n", aResult.uid().c_str());
            Firebase.printf("event: %s\n", RTDB.event().c_str());
            Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
            Firebase.printf("data: %s\n", RTDB.to<const char *>());
            Firebase.printf("type: %d\n", RTDB.type());

            if (RTDB.dataPath() == "/red_led")
            {
                redLedState = RTDB.to<bool>();
                digitalWrite(RED_LED_PIN, redLedState ? HIGH : LOW);
            }
            else if (RTDB.dataPath() == "/blue_led")
            {
                blueLedState = RTDB.to<bool>();
                digitalWrite(BLUE_LED_PIN, blueLedState ? HIGH : LOW);
            }
        }
        else
        {
            Serial.println("----------------------------");
            Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        }
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);

    pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(EXTRA_BUTTON_PIN, INPUT_PULLUP);  

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl1.setInsecure();
    ssl2.setInsecure();
#if defined(ESP8266)
    ssl1.setBufferSizes(1024, 1024);
    ssl2.setBufferSizes(1024, 1024);
#endif

    initializeApp(client1, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.get(client1, "/", result1, true);
}

void loop()
{
    Database.loop();
    unsigned long currentMillis = millis();

    if (digitalRead(RED_BUTTON_PIN) == LOW && (currentMillis - lastRedButtonPress) > debounceDelay)
    {
        redLedState = !redLedState;
        digitalWrite(RED_LED_PIN, redLedState ? HIGH : LOW);
        Database.set<bool>(client2, "/red_led", redLedState, result2);
        lastRedButtonPress = currentMillis;
    }

    if (digitalRead(BLUE_BUTTON_PIN) == LOW && (currentMillis - lastBlueButtonPress) > debounceDelay)
    {
        blueLedState = !blueLedState;
        digitalWrite(BLUE_LED_PIN, blueLedState ? HIGH : LOW);
        Database.set<bool>(client2, "/blue_led", blueLedState, result2);
        lastBlueButtonPress = currentMillis;
    }

    if (digitalRead(EXTRA_BUTTON_PIN) == LOW && (currentMillis - lastExtraButtonPress) > debounceDelay)
    {
        extraButtonState = !extraButtonState;
        Database.set<bool>(client2, "/extra_button", extraButtonState, result3);
        lastExtraButtonPress = currentMillis;
    }

    printResult(result1);
    printResult(result2);
    printResult(result3); 
}