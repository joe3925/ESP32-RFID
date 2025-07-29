#include <Arduino.h>
#include <SPI.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>
#include <const.hpp>
#include <fpga.hpp>
#include <WiFi.h>
#include <wifi.hpp>
#include <Firebase_ESP_Client.h>
MFRC522DriverPinSimple ss_pin(RFID_SS);
SPIClass &spiClass = SPI;
const SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);
MFRC522DriverSPI driver{ss_pin, spiClass, spiSettings};
MFRC522 mfrc522{driver};
#define FIREBASE_PROJECT_ID "cecs-project-b8bfe"

#define FIREBASE_RTDB_URL "cecs-project-b8bfe-default-rtdb.firebaseio.com"
TaskHandle_t mainTask = nullptr;
FirebaseData db;
FirebaseConfig cfg;
FirebaseAuth auth;

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
    mfrc522.PCD_ClearRegisterBitMask(MFRC522Constants::ComIEnReg, 0xFF);
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::ComIEnReg, 0x20);

    // Clear pending IRQ flags
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::ComIrqReg, 0x7F);
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, FPGA_RX, FPGA_TX);

    wifi_connect(100000);
    if (wifi_test(100000) != WifiHealth::Ok)
    {
        Serial.println(F("WIFI FAIL"));
    }
    else
    {
        Serial.println(F("WIFI CONNECTED"));
    }
    SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);

    pinMode(RFID_RST, OUTPUT);
    digitalWrite(RFID_RST, HIGH);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    mfrc522.PCD_Init();

    MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
    if (mfrc522.PCD_PerformSelfTest())
    {
        Serial.println(F("self test success"));
    }
    else
    {
        Serial.println(F("self test fail"));
    }

    pinMode(RFID_IRQ, INPUT_PULLUP);
    mainTask = xTaskGetCurrentTaskHandle();
    attachInterrupt(digitalPinToInterrupt(RFID_IRQ), rfid_isr, FALLING);

    mfrc522.PCD_Init();

    mfrc522.PCD_AntennaOn();

    arm_irq();

    mfrc522.PCD_ClearRegisterBitMask(MFRC522Constants::TxControlReg, 0xFF);
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::TxControlReg, 0x03);

    mfrc522.PCD_ClearRegisterBitMask(MFRC522Constants::GsNReg, 0xFF);
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::GsNReg, 0x44);

    mfrc522.PCD_ClearRegisterBitMask(MFRC522Constants::CWGsPReg, 0xFF);
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::CWGsPReg, 0x0F);

    mfrc522.PCD_ClearRegisterBitMask(MFRC522Constants::ModGsPReg, 0xFF);
    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::ModGsPReg, 0x0F);
    cfg.api_key = API_KEY_REAL;
    cfg.database_url = DATABASE_URL_REAL;
    auth.user.email = EMAIL;
    auth.user.password = PASSWORD;
    Firebase.begin(&cfg, &auth);
    Firebase.reconnectNetwork(true);
    Serial.print("Current time: ");
    Serial.println(Firebase.getCurrentTime());
    while (Firebase.ready());
    Serial.println("Signed in");
    MB_String path = "/users/" + auth.token.uid + "/status";
    Firebase.RTDB.setString(&db, path.c_str(), "Online");
    mfrc522.PCD_Init();

    Serial.println(F("Waiting for cards..."));
}

void loop()
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    mfrc522.PCD_SetRegisterBitMask(MFRC522Constants::ComIrqReg, 0x7F);

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
        Serial.print(F("UID: "));
        for (byte i = 0; i < mfrc522.uid.size; ++i)
            Serial.printf("%02X ", mfrc522.uid.uidByte[i]);
        Serial.println();

        if (fpga_is_allowed(mfrc522.uid.uidByte, mfrc522.uid.size))
        {
            digitalWrite(LED, HIGH);
            delay(1000);
            digitalWrite(LED, LOW);
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    arm_irq();
}
