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

#define RED_LED_PIN D2
#define BLUE_LED_PIN D6
#define GREEN_LED_PIN D1
#define YELLOW_LED_PIN D5

WiFiClientSecure ssl1, ssl2;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1, result2;
NoAuth noAuth;

bool redLedState = false;
bool blueLedState = false;
bool greenLedState = false;
bool yellowLedState = false;

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
            else if (RTDB.dataPath() == "/green_led")
            {
                greenLedState = RTDB.to<bool>();
                digitalWrite(GREEN_LED_PIN, greenLedState ? HIGH : LOW);
            }
            else if (RTDB.dataPath() == "/yellow_led")
            {
                yellowLedState = RTDB.to<bool>();
                digitalWrite(YELLOW_LED_PIN, yellowLedState ? HIGH : LOW);
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
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

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
    printResult(result1);
    printResult(result2);
}