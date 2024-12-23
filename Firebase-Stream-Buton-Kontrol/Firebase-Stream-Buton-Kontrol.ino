#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "Jar_Jar_Linksys"
#define WIFI_PASSWORD "rkf9sXNUPF4n"

#define DATABASE_SECRET "ulvxFFkB2cq749SyoDknXf4VkD0kor8EdBYlY1Hp"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"


#define LED_PIN D2    
#define BUTTON_PIN D1 

WiFiClientSecure ssl1;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1;
NoAuth noAuth;

bool lastButtonState = HIGH;
bool buttonState;
bool ledState = LOW;

void printResult(AsyncResult &aResult) {
    if (aResult.available()) {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream()) {
            if (RTDB.dataPath() == "/led") {
                ledState = RTDB.to<bool>();
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                Serial.print("LED State: ");
                Serial.println(ledState);
            }
        }
    }

    if (aResult.isError()) {
        Serial.print("Error: ");
        Serial.println(aResult.error().message().c_str());
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    ssl1.setInsecure();
    ssl1.setBufferSizes(1024, 1024);

    Serial.println("Connecting to Firebase...");
    initializeApp(client1, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.get(client1, "/led", result1, true);
}

void loop() {
    Database.loop();
    
    buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState != lastButtonState) {
        if (buttonState == LOW) {
            ledState = !ledState;
            Database.set<bool>(client1, "/led", ledState, result1);
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            Serial.println("Button pressed - LED toggled");
        }
        delay(50);
    }
    
    lastButtonState = buttonState;
    printResult(result1);
}