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

#define DATABASE_SECRET "ulvxFFkB2cq749SyoDknXf4VkD0kor8EdBYlY1Hp"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"

#define LED_PIN D2        // LED'in bağlı olduğu pin
#define BUTTON_PIN D1     // Butonun bağlı olduğu pin

WiFiClientSecure ssl1, ssl2;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1, result2;
NoAuth noAuth;

bool lastButtonState = HIGH; // Son okunan buton durumu
bool ledState = LOW;         // LED'in durumu

// Firebase'den gelen veriyi işleyen fonksiyon
void handleStream(AsyncResult &aResult) {
    if (aResult.available()) {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream() && RTDB.dataPath() == "/led") {
            bool newLedState = RTDB.to<bool>(); // Firebase'den gelen LED durumu
            if (newLedState != ledState) {      // Eğer durum farklıysa LED'i güncelle
                ledState = newLedState;
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                Serial.printf("Firebase'den LED güncellendi: %s\n", ledState ? "ON" : "OFF");
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nWi-Fi bağlantısı kuruldu!");

    ssl1.setInsecure();
    ssl2.setInsecure();
#if defined(ESP8266)
    ssl1.setBufferSizes(1024, 1024);
    ssl2.setBufferSizes(1024, 1024);
#endif

    initializeApp(client1, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);

    // Firebase veritabanı dinlemesi başlatılıyor
    Database.get(client1, "/led", result1, true); // "/led" için Stream bağlantısı başlatılıyor
}

void loop() {
    // Firebase veritabanını dinleme
    Database.loop();

    // Buton durumu kontrol ediliyor
    bool buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH) { // Butona basıldı
        ledState = !ledState;                            // LED durumunu değiştir
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);    // LED'i güncelle

        // Firebase'deki "/led" değerini güncelle
        Database.set<bool>(client2, "/led", ledState, result2);
        Serial.printf("Butona basıldı, LED durumu: %s\n", ledState ? "ON" : "OFF");
        delay(300); // Titreşim önleme
    }
    lastButtonState = buttonState;

    // Firebase işlem sonuçlarını yazdır ve Stream güncellemelerini işleme al
    handleStream(result1);
}
