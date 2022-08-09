#pragma once
#if FCOS_ESP32_C3
#include <WiFi.h>
#elif FCOS_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <display.hpp>

class InfoDisplay : public Display {
    enum Type_e {
        INFO_VERSION,
        INFO_LIGHT_SENSOR,
        INFO_IP_ADDR,
        INFO_UPTIME_HOURS,
        INFO_TOTAL,
    };

    size_t m_type{INFO_VERSION};

  public:
    InfoDisplay() : Display() { m_name = "INFO"; }

    virtual void Update() override {
        m_pixels->Clear();
        char str[10];
        switch (m_type) {
            case INFO_VERSION:
                m_pixels->DrawText(0, " 0.94", PURPLE);
                break;

            case INFO_LIGHT_SENSOR: {
                size_t brightnessPercent =
                    (int)(m_pixels->GetBrightness() * 100);
                size_t mantissa = (int)(m_pixels->GetBrightness() * 1000) % 10;
                sprintf(str, "%2d.%d", brightnessPercent, mantissa);
                m_pixels->DrawText(0, str, RED);
                break;
            }

            case INFO_IP_ADDR:
                sprintf(str, "%03d", WiFi.localIP()[3]);
                m_pixels->DrawText(20, str, BLUE);
                break;

            case INFO_UPTIME_HOURS:
                sprintf(str, "%4d", m_rtc->Uptime() / 60 / 60);
                m_pixels->DrawText(0, str, GREEN);
                break;
        }
    }

    virtual void Up(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_type < INFO_TOTAL - 1) {
                m_type++;
            }
        }
    }
    virtual void Down(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_type > 0) {
                m_type--;
            }
        }
    }
    // any left/right button press will exit this display

    virtual bool ShouldTimeout() override { return false; }
};
