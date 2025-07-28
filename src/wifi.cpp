#include <WiFiType.h>
#include <esp32-hal.h>
#include <WiFi.h>
#include <const.hpp>
#include <wifi.hpp>

bool wifi_connect(uint32_t timeout_ms) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

WifiHealth wifi_test(uint32_t tcp_timeout_ms) {
    if (WiFi.status() != WL_CONNECTED) return WifiHealth::NotConnected;

    IPAddress ip;
    if (!WiFi.hostByName("google.com", ip))  
        return WifiHealth::DnsFail;

    WiFiClient c;
    if (!c.connect(ip, 80, tcp_timeout_ms))
        return WifiHealth::TcpFail;

    c.stop();
    return WifiHealth::Ok;
}
