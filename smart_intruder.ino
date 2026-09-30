/*
  Intruder Alarm System - Dual Zone + LCD + Firebase + Phone Notifications
  ------------------------------------------------------------------------
  NEW in this version:
  - Real time from the internet (NTP), so the dashboard can say "5 minutes ago"
  - Push notification to your phone via ntfy.sh when an intruder is detected

  LIBRARIES: FirebaseClient (mobizt), LiquidCrystal I2C (Frank de Brabander)

  PHONE NOTIFICATION SETUP:
  1. Install the free "ntfy" app on your phone (Android/iPhone)
  2. Tap "+" and subscribe to the SAME topic name as NTFY_TOPIC below
  3. Treat the topic name like a password: anyone who knows it can read
     your alerts. Change it to something only you know (then subscribe to
     the new name).

  FILL IN: WIFI_SSID and WIFI_PASSWORD
*/

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <FirebaseClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Tell FirebaseClient which network client to use for 
#define SSL_CLIENT WiFiClientSecure

// ---------------- Configuration ----------------
const char* WIFI_SSID     = "your wifi name";
const char* WIFI_PASSWORD = "your wifi pasword";

#define API_KEY      "your firebse API key"
#define DATABASE_URL "your firebase URL"
#define USER_EMAIL   "firebase email"
#define USER_PASS    "firebase user password"

// ntfy topic (subscribe to this exact name in the ntfy app)
const char* NTFY_TOPIC = "smart-intruder-alarm- topic from the app ";

// Names shown on dashboard, LCD and notification
const char* ZONE_A_NAME = "Zone A";
const char* ZONE_B_NAME = "Zone B";

// Pins (placeholders, change to match your wiring)
const int PIN_DECODER_VT = 14;
const int PIN_DECODER_D0 = 32;
const int PIN_DECODER_D1 = 33;
const int PIN_BUZZER     = 23;
#define PIN_LCD_SDA 21
#define PIN_LCD_SCL 22

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

const unsigned long ALERT_COOLDOWN_MS  = 3000;
const unsigned long BUZZER_DURATION_MS = 15000;
const unsigned long LCD_REFRESH_MS     = 1000;

const String PATH_ARMED     = "alarm/armed";
const String PATH_STATUS    = "alarm/status";
const String PATH_ZONE      = "alarm/lastZone";
const String PATH_TRIGGERED = "alarm/lastTriggered";

bool systemArmed = true;
bool buzzerActive = false;
bool everTriggered = false;
String lastZoneTriggered = "None";
unsigned long lastAlertTime = 0;
unsigned long buzzerStartTime = 0;
unsigned long lastLcdUpdate = 0;

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
  if (buzzerActive) lcd.print("!! INTRUDER !!");
  else lcd.print(systemArmed ? "Status: ARMED" : "Status: DISARM");
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
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(WiFi.localIP());
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("WiFi Connected");
  delay(1000);
}

// Get real clock time from the internet (needed for "x minutes ago")
void initTime() {
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Syncing time...");
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  unsigned long start = millis();
  while (time(nullptr) < 1700000000 && millis() - start < 10000) delay(250);
  Serial.println(time(nullptr) > 1700000000 ? "Time synced" : "Time sync failed");
}

// Push notification to phone (ntfy.sh)
void sendPushNotification(const String &zoneName) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (http.begin(client, String("https://ntfy.sh/") + NTFY_TOPIC)) {
    http.addHeader("Title", "Intruder detected!");
    http.addHeader("Priority", "urgent");
    http.addHeader("Tags", "rotating_light");
    int code = http.POST("Movement detected at " + zoneName);
    Serial.printf("Push notification sent, HTTP %d\n", code);
    http.end();
  }
}

void triggerAlarm(const String &zoneName) {
  unsigned long now = millis();
  if (everTriggered && now - lastAlertTime < ALERT_COOLDOWN_MS) return;

  everTriggered = true;
  lastAlertTime = now;
  buzzerActive = true;
  buzzerStartTime = now;
  lastZoneTriggered = zoneName;
  digitalWrite(PIN_BUZZER, HIGH);
  updateLCD();

  time_t t = time(nullptr);
  int epoch = (t > 1700000000) ? (int)t : 0;   // 0 = clock not available

  Database.set<String>(aClient, PATH_STATUS, "Intruder detected!", processData, "setStatusTask");
  Database.set<String>(aClient, PATH_ZONE, zoneName, processData, "setZoneTask");
  Database.set<int>(aClient, PATH_TRIGGERED, epoch, processData, "setTimeTask");

  sendPushNotification(zoneName);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_DECODER_VT, INPUT);
  pinMode(PIN_DECODER_D0, INPUT);
  pinMode(PIN_DECODER_D1, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);
  lcd.init();
  lcd.backlight();

  initWiFi();
  initTime();

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
    if (digitalRead(PIN_DECODER_D0) == HIGH) triggerAlarm(ZONE_A_NAME);
    else if (digitalRead(PIN_DECODER_D1) == HIGH) triggerAlarm(ZONE_B_NAME);
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