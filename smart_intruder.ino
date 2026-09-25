/*
  Intruder Alarm System — Dual Zone (mmWave Radar 1 & 2) + LCD Display
  ESP32 with WiFi + Firebase Realtime Database
  ------------------------------------------------------------------------
  16x2 I2C LCD shows system status (Armed/Disarmed, last zone triggered).

  ADDITIONAL LIBRARY REQUIRED:
  - "LiquidCrystal I2C" by Frank de Brabander (Arduino Library Manager)

  LCD WIRING (I2C, only 4 wires):
  - VCC -> 5V (or 3.3V depending on your module, check the backpack)
  - GND -> GND
  - SDA -> GPIO21
  - SCL -> GPIO22

  If your LCD doesn't show anything once powered, most I2C backpacks have a
  small potentiometer on the back — turn it to adjust contrast.

  NOTE: Default I2C address is usually 0x27, but some modules use 0x3F.
  If the screen stays blank, try changing LCD_ADDR below.
*/

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- Configuration ----------------
const char* WIFI_SSID     = "REEDOX";
const char* WIFI_PASSWORD = "Reedox2$@2";

#define API_KEY      "AIzaSyAZOayqV4NluWYSeXV7QKEAJfZUz5ZbaDU",
#define DATABASE_URL "https://smart-intruder-alarm-default-rtdb.firebaseio.com"
#define USER_EMAIL   "ismailbamidele2002@gmail.com"
#define USER_PASS    "Reedox2029"

const int PIN_DECODER_VT = 14;
const int PIN_DECODER_D0 = 33;
const int PIN_DECODER_D1 = 32;
const int PIN_BUZZER     = 23;

// LCD I2C pins
#define PIN_LCD_SDA 21
#define PIN_LCD_SCL 22

#define LCD_ADDR 0x27   // change to 0x3F if screen stays blank
#define LCD_COLS 16
#define LCD_ROWS 2

const unsigned long ALERT_COOLDOWN_MS  = 60000;
const unsigned long BUZZER_DURATION_MS = 15000;
const unsigned long LCD_REFRESH_MS     = 1000;

const String PATH_ARMED     = "alarm/armed";
const String PATH_STATUS    = "alarm/status";
const String PATH_ZONE      = "alarm/lastZone";
const String PATH_TRIGGERED = "alarm/lastTriggered";

bool systemArmed        = true;
bool buzzerActive       = false;
String lastZoneTriggered = "None";
unsigned long lastAlertTime    = 0;
unsigned long buzzerStartTime  = 0;
unsigned long lastLcdUpdate    = 0;

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASS);
SSL_CLIENT ssl_client, stream_ssl_client;

FirebaseApp app;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client), streamClient(stream_ssl_client);
RealtimeDatabase Database;

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (buzzerActive) {
    lcd.print("!! INTRUDER !!");
  } else {
    lcd.print(systemArmed ? "Status: ARMED" : "Status: DISARM");
  }
  lcd.setCursor(0, 1);
  lcd.print("Zone: " + lastZoneTriggered);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;

  if (aResult.isError()) {
    Serial.printf("Firebase error: %s\n", aResult.error().message().c_str());
  }

  if (aResult.available()) {
    RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
    if (RTDB.type() == realtime_database_data_type_boolean) {
      systemArmed = RTDB.to<bool>();
      Serial.printf("System is now %s\n", systemArmed ? "ARMED" : "DISARMED");
      updateLCD();
    }
  }
}

void initWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(1500);
}

void triggerAlarm(const String &zoneName) {
  unsigned long now = millis();
  if (now - lastAlertTime < ALERT_COOLDOWN_MS) return;

  lastAlertTime      = now;
  buzzerActive       = true;
  buzzerStartTime    = now;
  lastZoneTriggered  = zoneName;
  digitalWrite(PIN_BUZZER, HIGH);
  updateLCD();

  Database.set<String>(aClient, PATH_STATUS, "Intruder detected!", processData, "setStatusTask");
  Database.set<String>(aClient, PATH_ZONE, zoneName, processData, "setZoneTask");
  Database.set<int>(aClient, PATH_TRIGGERED, (int)(millis() / 1000), processData, "setTimeTask");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_DECODER_VT, INPUT);
  pinMode(PIN_DECODER_D0, INPUT);
  pinMode(PIN_DECODER_D1, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL); // SDA=21, SCL=22
  lcd.init();
  lcd.backlight();

  initWiFi();

  ssl_client.setInsecure();
  stream_ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  stream_ssl_client.setConnectionTimeout(1000);
  stream_ssl_client.setHandshakeTimeout(5);

  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
  Database.get(streamClient, PATH_ARMED, processData, true, "armedStreamTask");

  updateLCD();
}

void loop() {
  app.loop();

  if (app.ready() && systemArmed && digitalRead(PIN_DECODER_VT) == HIGH) {
    if (digitalRead(PIN_DECODER_D0) == HIGH) {
      triggerAlarm("Zone A");
    } else if (digitalRead(PIN_DECODER_D1) == HIGH) {
      triggerAlarm("Zone B");
    }
  }

  if (buzzerActive && millis() - buzzerStartTime >= BUZZER_DURATION_MS) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerActive = false;
    updateLCD();
  }

  if (millis() - lastLcdUpdate >= LCD_REFRESH_MS) {
    lastLcdUpdate = millis();
    if (!buzzerActive) updateLCD();
  }
}