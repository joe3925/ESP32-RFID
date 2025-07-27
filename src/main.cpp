#include <Arduino.h>
#include <SPI.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>


bool send_uid_to_fpga(const uint8_t *uid, uint8_t len);
bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms);
uint8_t crc8(const uint8_t* d, size_t n);

constexpr uint8_t RFID_SS = 5;
constexpr uint8_t RFID_SCK = 18;
constexpr uint8_t RFID_MOSI = 23;
constexpr uint8_t RFID_MISO = 19;
constexpr uint8_t RFID_RST = 21;
constexpr uint8_t RFID_IRQ = 22;

// Only one dio is needed for the led. High is auth
constexpr uint8_t LED = 32;

constexpr uint8_t FPGA_WAKE = 13;

constexpr int FPGA_RX = 16; 
constexpr int FPGA_TX = 17; 

constexpr uint32_t READY_TIMEOUT_MS = 10;
constexpr uint32_t RESP_TIMEOUT_MS  = 10;

constexpr uint8_t FPGA_READY  = 0x52;
constexpr uint8_t FRAME_MAGIC = 0xA5;
constexpr uint8_t RES_ALLOW   = 0x01;
constexpr uint8_t RES_DENY    = 0x00;

MFRC522DriverPinSimple ss_pin(RFID_SS);
SPIClass &spiClass = SPI;
const SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);
MFRC522DriverSPI driver{ss_pin, spiClass, spiSettings};
MFRC522 mfrc522{driver};

TaskHandle_t mainTask = nullptr;

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

    SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);

    pinMode(RFID_RST, OUTPUT);
    digitalWrite(RFID_RST, HIGH);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    mfrc522.PCD_Init();

    MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
    if (mfrc522.PCD_PerformSelfTest())
    {
        Serial.println(F("self test succses"));
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

        if(send_uid_to_fpga(mfrc522.uid.uidByte, mfrc522.uid.size)){
            digitalWrite(LED, HIGH);
            delay(1000);
            digitalWrite(LED, LOW);
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    arm_irq();
}


// Lines:
//   FPGA_WAKE  : ESP32 -> FPGA (active low wake/interrupt)
//   UART    : Bi-directional (Serial2)
//
// Byte-level protocol (UART):
//   1) FPGA  -> ESP32 : READY (0x52)   
//   2) ESP32 -> FPGA  : 0xA5  LEN  UID[LEN]  CRC8
//   3) FPGA  -> ESP32 : RESULT (0x01 = allow, 0x00 = deny)
//
// Timing:
//   - ESP32 pulls FPGA_WAKE low, waits for READY within T_READY_MS.
//   - After RESULT received, ESP32 releases WAKE_N high.


bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms) {
    uint32_t start = millis();
    while ((millis() - start) < timeout_ms) {
        if (port.available()) {
            out = static_cast<uint8_t>(port.read());
            return true;
        }
    }
    return false;
}

bool send_uid_to_fpga(const uint8_t *uid, uint8_t len)
{
    digitalWrite(FPGA_WAKE, LOW);
    delayMicroseconds(5);          

    while (Serial2.available()) Serial2.read();

    uint8_t b;
    if (!wait_byte(Serial2, b, READY_TIMEOUT_MS) || b != FPGA_READY) {
        digitalWrite(FPGA_WAKE, HIGH);
        return false; 
    }

    uint8_t crc = crc8(uid, len);
    Serial2.write(FRAME_MAGIC);
    Serial2.write(len);
    Serial2.write(uid, len);
    Serial2.write(crc);

    uint8_t resp;
    if (!wait_byte(Serial2, resp, RESP_TIMEOUT_MS)) {
        digitalWrite(FPGA_WAKE, HIGH);
        return false; 
    }

    digitalWrite(FPGA_WAKE, HIGH);

    if (resp == RES_ALLOW) {

        return true;
    } else if (resp == RES_DENY) {

        return false;
    } else {
        return false;
    }
}
uint8_t crc8(const uint8_t* d, size_t n){
    uint8_t c = 0x00;
    for(size_t i=0;i<n;i++){
        c ^= d[i];
        for(int b=0;b<8;b++)
            c = (c & 0x80) ? (c<<1) ^ 0x31 : (c<<1);
    }
    return c;
}