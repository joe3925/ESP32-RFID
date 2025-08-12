#include <Arduino.h>
#include <SPI.h>
#include <LiquidCrystal.h>

#include <const.hpp>
#include <fpga.hpp>
#include <WiFi.h>
#include <wifi.hpp>
#include <Firebase_ESP_Client.h>
#include <env.hpp>

LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

#define FIREBASE_PROJECT_ID "cecs-project-b8bfe"

#define FIREBASE_RTDB_URL "cecs-project-b8bfe-default-rtdb.firebaseio.com"
TaskHandle_t mainTask = nullptr;
FirebaseData db;
FirebaseConfig cfg;
FirebaseAuth auth;
MB_String user_base;

void IRAM_ATTR rfid_isr()
{
    if (mainTask)
    {
        BaseType_t hpw = pdFALSE;
        vTaskNotifyGiveFromISR(mainTask, &hpw);
        if (hpw)
            portYIELD_FROM_ISR();
    }
}

static inline void arm_irq()
{
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, FPGA_RX, FPGA_TX);

    dacWrite(LCD_VO, 0);
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello, world!");

    wifi_connect(10000);
    if (wifi_test(100000) != WifiHealth::Ok)
    {
        Serial.println(F("WIFI FAIL"));
    }
    else
    {
        Serial.println(F("WIFI CONNECTED"));
    }

    pinMode(RFID_RST, OUTPUT);
    digitalWrite(RFID_RST, HIGH);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    pinMode(RFID_IRQ, INPUT_PULLUP);

    mainTask = xTaskGetCurrentTaskHandle();
    attachInterrupt(digitalPinToInterrupt(RFID_IRQ), rfid_isr, FALLING);

    arm_irq();

    cfg.api_key = API_KEY_REAL;
    cfg.database_url = DATABASE_URL_REAL;
    auth.user.email = EMAIL;
    auth.user.password = PASSWORD;

    Firebase.begin(&cfg, &auth);
    Firebase.reconnectNetwork(true);

    uint32_t t0 = millis();
    while (!Firebase.ready() && millis() - t0 < 15000)
    {
        delay(50);
    }

    if (!Firebase.ready())
    {
        Serial.println("Firebase not ready after 15s — check API key, Email/Password provider, and DB URL.");
    }
    else
    {
        Serial.println("Signed in");
    }

    user_base = MB_String("/users/") + auth.token.uid;
    Firebase.RTDB.setString(&db, (user_base + "/status").c_str(), "Online");

    Serial.println(F("Waiting for cards..."));
}

void loop()
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (false)
    {
        Serial.print(F("UID: "));
        // for (byte i = 0; i < mfrc522.uid.size; ++i)
        //     Serial.printf("%02X ", mfrc522.uid.uidByte[i]);
        // Serial.println();

        // bool allowed = fpga_is_allowed(mfrc522.uid.uidByte, mfrc522.uid.size);
        // log_scan_event(mfrc522.uid.uidByte, mfrc522.uid.size, allowed);
        bool allowed = false;
        if (allowed)
        {
            digitalWrite(LED, HIGH);
            delay(1000);
            digitalWrite(LED, LOW);
        }
    }

    arm_irq();
}

static MB_String uid_hex(const byte *uid, byte len)
{
    MB_String s;
    for (byte i = 0; i < len; ++i)
    {
        char buf[3];
        sprintf(buf, "%02X", uid[i]);
        s += buf;
        if (i + 1 < len)
            s += ":";
    }
    return s;
}

static void log_scan_event(const byte *uid, byte len, bool allowed)
{
    if (!Firebase.ready() || auth.token.uid.length() == 0)
    {
        Serial.println(F("log skipped: Firebase not ready or no UID token"));
        return;
    }

    FirebaseJson json;
    json.set("uid", uid_hex(uid, len).c_str());
    json.set("allowed", allowed);

    time_t ts = Firebase.getCurrentTime();
    if (ts > 0)
    {
        json.set("ts", (int)ts);
    }
    else
    {
        json.set("ts_ms", (int64_t)millis());
    }

    MB_String path = user_base + "/scans";
    if (!Firebase.RTDB.pushJSON(&db, path.c_str(), &json))
    {
        Serial.printf("RTDB push failed: %s\n", db.errorReason().c_str());
    }
}