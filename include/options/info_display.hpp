#pragma once
#if FCOS_ESP32_C3
#include <WiFi.h>
#include <Zanshin_BME680.h>
#include <dprint.hpp>
#elif FCOS_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <display.hpp>
#include <elapsed_time.hpp>
#include <options/breakout.hpp>

class InfoDisplay : public Display {
    enum Type_e {
        INFO_VERSION,
        INFO_LIGHT_SENSOR,
        INFO_IP_ADDR,
        INFO_UPTIME_HOURS,
#if FCOS_ESP32_C3
        INFO_BME680_TEMPERATURE_F,
        INFO_BME680_TEMPERATURE_C,
    //  INFO_BME680_PRESSURE,
    //  INFO_BME680_HUMIDITY,
    //  INFO_BME680_GAS,
#endif
        INFO_TOTAL,
    };

    size_t m_type{INFO_VERSION};

#if FCOS_ESP32_C3
    BME680_Class m_BME680;
#endif
    ElapsedTime m_sinceLastSensorData;
    int32_t m_temperatureC{0};
    int32_t m_temperatureF{0};
    bool m_isBME680Present{false};

    std::shared_ptr<Animator> m_demoAnim;
    uint8_t m_demoAnimSelection{ANIM_NORMAL};

  public:
    InfoDisplay() : Display() {
        m_name = "INFO";

#if FCOS_ESP32_C3
        m_isBME680Present = m_BME680.begin();
        if (m_isBME680Present) {
            m_BME680.setOversampling(TemperatureSensor, Oversample16);
        }
#endif
    }

    virtual void Update() override {
#if FCOS_ESP32_C3
        if (m_isBME680Present && m_sinceLastSensorData.Ms() > 250) {
            m_sinceLastSensorData.Reset();
            int32_t humidity, pressure, gas;  // placeholders
            m_BME680.getSensorData(m_temperatureC, humidity, pressure, gas);
            m_temperatureC /= 100;  // temp is in 0.01 deg C
            m_temperatureF = ((float)m_temperatureC * 1.8f) + 32;
        }
#endif

        m_pixels->Clear();
        char str[10] = {0};
        int yPos = 0;
        int xPos = 0;
#if FCOS_CARDCLOCK2
        yPos = 3;
        xPos = 1;
#endif
        switch (m_type) {
            case INFO_VERSION:
                m_pixels->DrawText(0, yPos, " " + String(FW_VERSION), PURPLE);
                break;

            case INFO_LIGHT_SENSOR: {
                size_t brightnessPercent =
                    (int)(m_pixels->GetBrightness() * 100);
                size_t mantissa = (int)(m_pixels->GetBrightness() * 1000) % 10;
                sprintf(str, "%2d.%d", brightnessPercent, mantissa);
                m_pixels->DrawText(0, yPos, str, RED);
                break;
            }

            case INFO_IP_ADDR:
#if FCOS_FOXIECLOCK
                sprintf(str, "%03d", WiFi.localIP()[3]);
                m_pixels->DrawText(20, yPos, str, BLUE);
#else
                sprintf(str, "*.%03d", WiFi.localIP()[3]);
                m_pixels->DrawText(0, yPos, str, BLUE);
#endif
                break;

            case INFO_UPTIME_HOURS:
                sprintf(str, "%4d", m_rtc->Uptime() / 60 / 60);
                m_pixels->DrawText(xPos, yPos, str, GREEN);
                break;

#if FCOS_ESP32_C3
            case INFO_BME680_TEMPERATURE_F:
                sprintf(str, "%4d", m_temperatureF);
                m_pixels->DrawText(xPos, yPos, str, CYAN);
                break;

            case INFO_BME680_TEMPERATURE_C:
                sprintf(str, "%4d", m_temperatureC);
                m_pixels->DrawText(xPos, yPos, str, CYAN);
                break;
#endif
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
            } else if (m_type == 0) {
                Breakout game(m_pixels, m_settings, m_rtc);
                game.Start();
            }
        }
    }

    // any left/right button press will exit this display

    virtual bool ShouldTimeout() override { return false; }
};
