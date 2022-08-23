#pragma once
#include <ArduinoOTA.h>

#if FCOS_ESP32_C3
#include <ESPmDNS.h>
#include <WiFi.h>
#elif FCOS_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <elapsed_time.hpp>
#include <pixels.hpp>

class FwUpdate {
  private:
    std::shared_ptr<Pixels> m_pixels;
    bool m_isInitialized{false};

  public:
    FwUpdate(std::shared_ptr<Pixels> pixels) : m_pixels(pixels) {}

  public:
    void Update() {
        if (WiFi.isConnected() && !m_isInitialized) {
            InitializeOTA();
        } else if (m_isInitialized) {
            ArduinoOTA.handle();
        }
    }

  private:
    void InitializeOTA() {
        ArduinoOTA.onStart([&]() {
            // String type;
            // if (ArduinoOTA.getCommand() == U_FS) {
            //     LittleFS.end();
            // }
        });
        ArduinoOTA.onEnd([&]() { ElapsedTime::Delay(500); });
        ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
            m_pixels->Clear();
            size_t percentComplete = ((float)progress / (float)total) * 100.0f;
            char str[10];
            sprintf(str, "%3d", percentComplete);
            m_pixels->DrawText(20, str, percentComplete < 100 ? PURPLE : GREEN);
            m_pixels->Set(LED_OPT_UPDT, percentComplete < 100 ? PURPLE : GREEN);
            m_pixels->Update();
        });
        ArduinoOTA.onError([&](ota_error_t error) {
            // SENS-OARS detected an ERR-OAR, show user
            char str[10];
            sprintf(str, "%03d", error);
            m_pixels->Clear();
            m_pixels->DrawText(20, str, RED);
            m_pixels->Update();
            ElapsedTime::Delay(5000);
        });

        MDNS.begin(GetUniqueMDNSName().c_str());
        ArduinoOTA.setHostname(GetUniqueMDNSName().c_str());
        ArduinoOTA.begin();
        m_isInitialized = true;
    }

    String GetUniqueMDNSName() {
        return "FoxieClock_" + String(WiFi.localIP()[3], DEC);
    }
};
