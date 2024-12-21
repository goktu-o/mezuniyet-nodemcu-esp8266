//https://github.com/mobizt/FirebaseClient/blob/main/examples/RealtimeDatabase/Simple/StreamNoAuth/StreamNoAuth.ino
/**
 * This example is for new users which are familiar with other legacy Firebase libraries.
 *
 * The example shows how to listen the data changes in your Firebase Realtime database
 * while the database was set periodically.
 * 
 * All functions used in this example are non-blocking (async) functions.
 *
 * This example will not use any authentication method included database secret.
 *
 * It needs to change the security rules to allow read and write.
 *
 * This example is for ESP32, ESP8266 and Raspberry Pi Pico W.
 *
 * You can adapt the WiFi and SSL client library that are available for your devices.
 *
 * For the ethernet and GSM network which are not covered by this example,
 * you have to try another elaborate examples and read the library documentation thoroughly.
 *
 */

/** Change your Realtime database security rules as the following.
 {
  "rules": {
    ".read": true,
    ".write": true
  }
}
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "Jar_Jar_Linksys"
#define WIFI_PASSWORD "rkf9sXNUPF4n"

#define DATABASE_SECRET "ulvxFFkB2cq749SyoDknXf4VkD0kor8EdBYlY1Hp"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"

const int LED_PIN = 2; // D4 pini (ESP8266'da GPIO2)
const int BUTTON_PIN = 0;  // D3 pini (ESP8266'da GPIO0)

// Button durumu için değişkenler
int lastButtonState = HIGH;
int buttonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// The SSL client used for secure server connection.
WiFiClientSecure ssl1, ssl2;

// The default network config object that used in this library.
DefaultNetwork network;

// The client, aka async client, is the client that handles many tasks required for each operation.
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));

// The authentication task handler, aka FirebaseApp.
FirebaseApp app;

// The Realtime database class object that provides the functions.
RealtimeDatabase Database;

// The class that stores the operating result, aka AsyncResult.
AsyncResult result1, result2;

// The no-authentication provider class used for authentication initialization.
NoAuth noAuth;

unsigned long ms = 0;

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
             if (RTDB.dataPath() == "/test/stream/toggle_value")
            {
                bool ledState = RTDB.to<bool>();
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                Serial.printf("LED State changed to: %s\n", ledState ? "ON" : "OFF");
            }

            Serial.println("----------------------------");
            Firebase.printf("task: %s\n", aResult.uid().c_str());
            Firebase.printf("event: %s\n", RTDB.event().c_str());
            Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
            Firebase.printf("data: %s\n", RTDB.to<const char *>());
            Firebase.printf("type: %d\n", RTDB.type());

            // The stream event from RealtimeDatabaseResult can be converted to the values as following.
            bool v1 = RTDB.to<bool>();
            int v2 = RTDB.to<int>();
            float v3 = RTDB.to<float>();
            double v4 = RTDB.to<double>();
            String v5 = RTDB.to<String>();
        }
        else
        {
            Serial.println("----------------------------");
            Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        }
    }
}

void toggleLED() {
    // Mevcut LED durumunu oku
    Database.get(client2, "/test/stream/toggle_value", result2);
    if (result2.available()) {
        RealtimeDatabaseResult &RTDB = result2.to<RealtimeDatabaseResult>();
        bool currentState = RTDB.to<bool>();
        // LED durumunu tersine çevir
        Database.set<bool>(client2, "/test/stream/toggle_value", !currentState, result2);
    }
}

void setup()
{

    Serial.begin(115200);
     // Pin modlarını ayarla
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

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
    ssl1.setBufferSizes(1024, 1024);
    ssl2.setBufferSizes(1024, 1024);

    // Initialize the authentication handler.
    initializeApp(client1, app, getAuth(noAuth));

    // Binding the authentication handler with your Database class object.
    app.getApp<RealtimeDatabase>(Database);

    // Set your database URL
    Database.url(DATABASE_URL);

    // Initiate the Stream connection to listen the data changes.
    // This function can be called once.
    // The Stream was connected using async get function (non-blocking) which the result will assign to the function in this case.
    //Database.get(client1, "/test/stream", result1, true /* this option is for Stream connection */);
     // LED durumunu dinlemeye başla
    Database.get(client1, "/test/stream/toggle_value", result1, true);
}

void loop()
{
    // Polling for internal task operation
    // This required for Stream in this case.
    Database.loop();

    // We don't have to poll authentication handler task using app.loop() as seen in other examples
    // because the database secret is the priviledge access key that never expired.
    
    // Button kontrolü ve debounce
    int reading = digitalRead(BUTTON_PIN);

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    // Set the random int value to "/test/stream/int" every 20 seconds.
    if (millis() - lastDebounceTime > debounceDelay )
      if (reading != buttonState) {
        buttonState = reading;
          if (buttonState == LOW) {  // Button basıldığında
            toggleLED();
      }
    }

    lastButtonState = reading;
    
    // Polling print the result if it is available.
    printResult(result1);
    printResult(result2);
}