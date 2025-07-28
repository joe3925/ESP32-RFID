#include <WiFiType.h>
#include <esp32-hal.h>
#include <WiFi.h>
#include <const.hpp>

bool wifi_connect(uint32_t timeout_ms = 10000) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}
