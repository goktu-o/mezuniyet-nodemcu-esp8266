#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#define WIFI_SSID "Jar_Jar_Linksys"
#define WIFI_PASSWORD "rkf9sXNUPF4n"

#define DATABASE_SECRET "ulvxFFkB2cq749SyoDknXf4VkD0kor8EdBYlY1Hp"
#define DATABASE_URL "https://mezuniyet-projesi-iot-firebase-default-rtdb.europe-west1.firebasedatabase.app/"

const int LED_PIN = 2;     // D4 pini (ESP8266'da GPIO2)
const int BUTTON_PIN = 0;  // D3 pini (ESP8266'da GPIO0)

int lastButtonState = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

WiFiClientSecure ssl1, ssl2;
DefaultNetwork network;
AsyncClientClass client1(ssl1, getNetwork(network)), client2(ssl2, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result1, result2;
NoAuth noAuth;

// NTP sunucu ayarları
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

void setupTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("NTP sunucusuna bağlanılıyor...");
    while (!time(nullptr)) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nZaman senkronize edildi");
}

void printResult(AsyncResult &aResult)
{
    if (aResult.isDebug())
    {
        Serial.println("DEBUG: " + String(aResult.debug().c_str()));
    }

    if (aResult.isError())
    {
        Serial.println("ERROR: " + String(aResult.error().message().c_str()));
        Serial.println("Error Code: " + String(aResult.error().code()));
    }

    if (aResult.available())
    {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream())
        {
            if (String(RTDB.to<const char *>()) != "null" && RTDB.type() != 0)
            {
                Serial.println("\n----- Stream Data -----");
                Serial.println("Event: " + String(RTDB.event().c_str()));
                Serial.println("Path: " + String(RTDB.dataPath().c_str()));
                Serial.println("Data: " + String(RTDB.to<const char *>()));
                
                bool ledState = RTDB.to<bool>();
                digitalWrite(LED_PIN, !ledState);
                Serial.println("LED durumu -> " + String(ledState ? "AÇIK" : "KAPALI"));
            }
        }
    }
}

void toggleLED() {
    Serial.println("\nButona basıldı!");
    bool currentState = digitalRead(LED_PIN);
    bool newState = !currentState;
    
    Serial.println("LED durumu değiştiriliyor...");
    Database.set<bool>(client2, "test/stream/toggle_value", newState, result2);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\nBaşlatılıyor...");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // LED başlangıçta kapalı
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    Serial.print("WiFi'ya bağlanılıyor");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nWiFi Bağlandı: " + WiFi.localIP().toString());

    // NTP ile zaman senkronizasyonu
    setupTime();

    // SSL ayarları
    ssl1.setInsecure();
    ssl2.setInsecure();
    ssl1.setBufferSizes(4096, 1024); // Buffer boyutunu artırdık
    ssl2.setBufferSizes(4096, 1024);

    Serial.println("Firebase'e bağlanılıyor...");
    initializeApp(client1, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);

    // 5 saniye bekle
    Serial.println("Bağlantı stabilizasyonu için bekleniyor...");
    delay(5000);

    // İlk değeri ayarla
    Serial.println("Başlangıç değeri ayarlanıyor...");
    Database.set<bool>(client2, "test/stream/toggle_value", false, result2);
    delay(1000);

    // Stream'i başlat
    Serial.println("Stream başlatılıyor...");
    Database.get(client1, "test/stream/toggle_value", result1, true);
    
    Serial.println("Hazır!");
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi bağlantısı koptu! Yeniden bağlanılıyor...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(5000);
        return;
    }

    Database.loop();

    // Button kontrolü
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) {
                toggleLED();
            }
        }
    }
    lastButtonState = reading;

    printResult(result1);
    printResult(result2);
    delay(10);
}