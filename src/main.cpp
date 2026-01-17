#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SPI.h>

#include <Adafruit_PN532.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <Wire.h>
#include <const.hpp>
#include <env.hpp>
#include <fpga.hpp>
#include <wifi.hpp>
#include <util.hpp>
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

Adafruit_PN532 nfc = Adafruit_PN532(PN532_IRQ, PN532_RSC);

#define FIREBASE_PROJECT_ID "cecs-project-b8bfe"

TaskHandle_t mainTask = nullptr;
FirebaseData db;
FirebaseConfig cfg;
FirebaseAuth auth;
static void log_scan_event(const byte* uid, byte len, bool allowed);
static void fpga_ensure_authorized(const byte* uid, byte len);
static FbCheckStatus firebase_try_check_allowed(const byte* uid, byte len, bool* allowed_out);
static inline bool firebase_authed()
{
    return Firebase.ready() && auth.token.uid.length() > 0;
}
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

void lcdPrintf(const char* fmt, ...)
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
    while (!Serial);

    Serial2.begin(115200, SERIAL_8N1, ESP_32_RX, ESP_32_TX);
    pinMode(PN532_SDA, INPUT_PULLUP);
    pinMode(PN532_SCL, INPUT_PULLUP);
    pinMode(PN532_RSC, OUTPUT);

    pinMode(FPGA_WAKE, OUTPUT);
    digitalWrite(FPGA_WAKE, HIGH);

    Wire.begin(PN532_SDA, PN532_SCL);
    Wire.setClock(100000);

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
    MB_String base = auth.token.uid;
    Firebase.RTDB.setString(&db, (base + "/status").c_str(), "Online");
    arm_irq();

    Serial.println(F("Waiting for cards..."));
    lcdPrintf("Ready");
    // bool allowed = fpga_is_allowed(test_uid, test_uid_len);
}
void loop()
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    uint8_t uid[10];
    uint8_t uidLength;

    if ((nfc).readDetectedPassiveTargetID(uid, &uidLength))
    {
        Serial.println(F("Card detected!"));
        lcdPrintf("Card detected");

        for (byte i = 0; i < uidLength; ++i) Serial.printf("%02X ", uid[i]);
        Serial.println();

        bool allowed = false;

        if (wifi_ok())
        {
            FbCheckStatus st = firebase_try_check_allowed(uid, uidLength, &allowed);

            if (st == FbCheckStatus::Worked)
            {
                Serial.printf("FIREBASE worked: allowed=%d\n", allowed);

                if (allowed)
                {
                    lcdPrintf("Auth (cloud)");
                    fpga_ensure_authorized(uid, uidLength);
                }
                else
                {
                    lcdPrintf("Not auth");
                    // TODO (NO-OP): remove from FPGA if present
                    // fpga_deauthorize_uid(uid, uidLength);
                }
            }
            else if (st == FbCheckStatus::PermissionDeny || st == FbCheckStatus::NotAuthed || st == FbCheckStatus::UnknownFail)
            {

                Serial.println(F("Firebase not authorized/denied -> fail closed"));
                lcdPrintf("Not auth");
            }
            else 
            {
                Serial.println(F("Firebase unavailable -> FPGA fallback"));
                goto FPGA_FALLBACK;
            }
        }
        else
        {
            // No WiFi => FPGA fallback
            Serial.println(F("No WiFi -> FPGA fallback"));
            goto FPGA_FALLBACK;
        }

        Serial.println();
        vTaskDelay(3000);
        lcdPrintf("Ready");
        arm_irq();
        return;

    FPGA_FALLBACK:
        {
            auto ar = fpga_is_allowed(uid, uidLength);
            Serial.printf("FPGA CHECK: ok=%d status=%u result=0x%02X\n",
                          ar.success, (uint8_t)ar.status, ar.result);

            bool fpga_allowed = (ar.success && ar.result == 0x01);

            if (fpga_allowed)
            {
                lcdPrintf("Auth (fpga)");
                Serial.println(F("Authorized via FPGA (offline fallback)."));
            }
            else
            {
                lcdPrintf("Not auth");
                Serial.println(F("Not authorized (FPGA)."));
            }

            Serial.println();
            vTaskDelay(3000);
            lcdPrintf("Ready");
            arm_irq();
            return;
        }
    }

    lcdPrintf("Ready");
    arm_irq();
}

static MB_String uid_hex(const byte* uid, byte len)
{
    MB_String s;
    for (byte i = 0; i < len; ++i)
    {
        char buf[3];
        sprintf(buf, "%02X", uid[i]);
        s += buf;

    }
    return s;
}


static void fpga_ensure_authorized(const byte* uid, byte len)
{
    auto ar = fpga_is_allowed(uid, len);
    Serial.printf("FPGA CHECK: ok=%d status=%u result=0x%02X\n",
                  ar.success, (uint8_t)ar.status, ar.result);

    bool already_ok = (ar.success && ar.result == 0x01);

    if (!already_ok)
    {
        auto add = fpga_authorize_uid(uid, len);
        Serial.printf("FPGA ADD:   ok=%d status=%u result=0x%02X\n",
                      add.success, (uint8_t)add.status, add.result);
    }
}

static FbCheckStatus firebase_try_check_allowed(const byte* uid, byte len, bool* allowed_out)
{
    *allowed_out = false;

    if (!wifi_ok(1500))
        return FbCheckStatus::WifiFail;

    if (!firebase_authed())
        return FbCheckStatus::NotAuthed;

    MB_String key  = uid_hex(uid, len);
    MB_String base = auth.token.uid;
    MB_String path = base + "/auth/" + key;

    Serial.printf("Checking Firebase RTDB path: %s\n", path.c_str());

    FirebaseJson json;
    bool ok_json = Firebase.RTDB.getJSON(&db, path.c_str(), &json);

    int code = db.httpCode();
    const char* reason = db.errorReason().c_str();

    Serial.printf("RTDB getJSON ok=%d http=%d reason=%s\n", ok_json, code, reason);

    if (ok_json)
    {
        *allowed_out = true;
        return FbCheckStatus::Worked;
    }


    bool ok_bool = Firebase.RTDB.getBool(&db, path.c_str());
    code = db.httpCode();
    reason = db.errorReason().c_str();

    Serial.printf("RTDB getBool ok=%d http=%d reason=%s\n", ok_bool, code, reason);

    if (ok_bool)
    {
        *allowed_out = db.to<bool>();
        return FbCheckStatus::Worked;
    }

    // Handle "exists but wrong type" / "parse" cases: sometimes http=200 but conversion fails.
    // If http is 200 here, Firebase replied, so this is NOT "unavailable".
    if (code == 200)
    {
        *allowed_out = false;
        return FbCheckStatus::Worked;
    }

    if (code == 404 || code == 400 ||
        code == FIREBASE_ERROR_HTTP_CODE_NOT_FOUND ||
        code == FIREBASE_ERROR_HTTP_CODE_BAD_REQUEST)
    {
        *allowed_out = false;
        return FbCheckStatus::Worked;
    }

    if (code == 401 || code == 403 ||
        code == FIREBASE_ERROR_HTTP_CODE_UNAUTHORIZED ||
        code == FIREBASE_ERROR_HTTP_CODE_FORBIDDEN)
    {
        *allowed_out = false;
        return FbCheckStatus::PermissionDeny;
    }

    if (code == 0 || code < 0)
    {
        return FbCheckStatus::UnknownFail;
    }

    return FbCheckStatus::UnknownFail;
}