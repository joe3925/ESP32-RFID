#include <Arduino.h>
#include <SPI.h>
#include <LiquidCrystal.h>

#include <const.hpp>
#include <fpga.hpp>
#include <WiFi.h>
#include <wifi.hpp>
#include <Firebase_ESP_Client.h>
#include <env.hpp>
#include <Adafruit_PN532.h>
#include <Wire.h>
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

Adafruit_PN532 nfc = Adafruit_PN532(PN532_IRQ, PN532_RSC);

#define FIREBASE_PROJECT_ID "cecs-project-b8bfe"

TaskHandle_t mainTask = nullptr;
FirebaseData db;
FirebaseConfig cfg;
FirebaseAuth auth;
MB_String user_base;
static void log_scan_event(const byte *uid, byte len, bool allowed);
void IRAM_ATTR rfid_isr()
{
    if (mainTask)
    {
        BaseType_t hpw = pdFALSE;
        vTaskNotifyGiveFromISR(mainTask, &hpw);
        if (hpw)
        {
            portYIELD_FROM_ISR();
        }
    }
}

static inline void arm_irq()
{
    (nfc).SAMConfig();
    (nfc).startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

void lcdPrintf(const char *fmt, ...)
{
    char buf[33];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    lcd.clear();
    lcd.setCursor(0, 0);

    for (int i = 0; i < 16 && buf[i] != '\0'; i++)
    {
        lcd.print(buf[i]);
    }

    if (strlen(buf) > 16)
    {
        lcd.setCursor(0, 1);
        for (int i = 16; i < 32 && buf[i] != '\0'; i++)
        {
            lcd.print(buf[i]);
        }
    }
}
void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    Serial2.begin(115200, SERIAL_8N1, ESP_32_RX, ESP_32_TX);
    pinMode(PN532_SDA, INPUT_PULLUP);
    pinMode(PN532_SCL, INPUT_PULLUP);
    pinMode(PN532_RSC, OUTPUT);
    Wire.begin(PN532_SDA, PN532_SCL);
    // Wire.setClock(100000);

    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("starting...");

    wifi_connect(10000);
    if (wifi_test(100000) != WifiHealth::Ok)
    {
        Serial.println(F("WIFI FAIL"));
        lcdPrintf("No wifi");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    else
    {
        Serial.println(F("WIFI CONNECTED"));
    }

    (nfc).begin();

    uint32_t versiondata = (nfc).getFirmwareVersion();
    if (!versiondata)
    {
        Serial.println("Didn't find PN53x board");
        lcdPrintf("Faulted");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    else
    {
        Serial.println("found PN53x");
    }

    pinMode(PN532_IRQ, INPUT_PULLUP);

    mainTask = xTaskGetCurrentTaskHandle();
    attachInterrupt(digitalPinToInterrupt(PN532_IRQ), rfid_isr, FALLING);

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
        lcdPrintf("Firebase failed");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    else
    {
        Serial.println("Signed in");
    }

    user_base = MB_String("/users/") + auth.token.uid;
    Firebase.RTDB.setString(&db, (user_base + "/status").c_str(), "Online");
    arm_irq();

    Serial.println(F("Waiting for cards..."));
    lcdPrintf("Ready");
    static const uint8_t test_uid[7] = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56};
    static const uint8_t test_uid_len = 7;
    bool allowed = fpga_is_allowed(test_uid, test_uid_len);
}

void loop()
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    uint8_t uid[7];
    uint8_t uidLength;
    if ((nfc).readDetectedPassiveTargetID(uid, &uidLength))
    {
        lcdPrintf("Card detected");
        for (byte i = 0; i < uidLength; ++i)
            Serial.printf("%02X ", uid);
        Serial.println();

        bool allowed = fpga_is_allowed(uid, uidLength);
        if (allowed)
        {
            lcdPrintf("Approved!");
            Serial.println(F("Approved!"));
        }
        else
        {
            lcdPrintf("Not auth");
            Serial.println(F("Not auth"));
        }
        // log_scan_event(uid, uidLength, allowed);
        Serial.print(F("UID: "));
        for (byte i = 0; i < uidLength; i++)
        {
            Serial.printf("%02X ", uid[i]);
        }
        Serial.println();
        vTaskDelay(3000);
    }
    lcdPrintf("Ready");

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