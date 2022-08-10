#pragma once
#if FCOS_ESP32_C3
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#elif FCOS_ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#define httpUpdate ESPhttpUpdate
#endif
#include <display.hpp>
#include <elapsed_time.hpp>

class WebUpdate : public Display {
    enum State_e {
        CHECKING,
        ASKING,
        UPDATING,
        DONE,
        TOTAL_STATES,
    };

    size_t m_state{CHECKING};
    WiFiClientSecure m_client;
    String m_webVersion;
    ElapsedTime m_timeSinceAnimation;

  public:
    WebUpdate() : Display() { m_name = "INFO"; }

    virtual void Activate() override { m_state = CHECKING; }

    virtual void Update() override {
        m_pixels->Clear();
        switch (m_state) {
            case CHECKING:
                m_pixels->Clear();
                m_pixels->Set(LED_OPT_UPDT, YELLOW);
                m_pixels->DrawChar(8, ':', YELLOW);
                m_pixels->Update(true);
                m_webVersion = GetVersionFromServer();
                m_state = ASKING;
                m_timeSinceAnimation.Reset();
                break;

            case ASKING: {
                m_pixels->DrawText(20, m_webVersion, ORANGE);
                m_pixels->Set(LED_OPT_UPDT, BLUE);
                static bool isOn{true};
                m_pixels->Set(LED_JOY_UP, isOn ? GREEN : BLACK);
                if (m_timeSinceAnimation.Ms() > 500) {
                    m_timeSinceAnimation.Reset();
                    isOn = !isOn;
                }
                break;
            }

            case UPDATING:
                // progress is shown by HTTPUpdate
                httpUpdate.onProgress([&](int cur, int total) {
                    m_pixels->Clear();
                    size_t percentComplete =
                        ((float)cur / (float)total) * 100.0f;
                    char str[10];
                    sprintf(str, "%3d", percentComplete);
                    m_pixels->DrawText(20, str,
                                       percentComplete < 100 ? PURPLE : GREEN);
                    m_pixels->Set(LED_OPT_UPDT,
                                  percentComplete < 100 ? PURPLE : GREEN);
                    m_pixels->Update(true);
                });
                httpUpdate.onError([&](int error) {
                    char str[10];
                    sprintf(str, "%03d", error);
                    m_pixels->Clear();
                    m_pixels->DrawText(20, str, RED);
                    m_pixels->Update(true);
                    ElapsedTime::Delay(5000);
                });
                httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
                httpUpdate.rebootOnUpdate(true);

                m_pixels->Clear();
                m_pixels->Set(LED_OPT_UPDT, BLUE);
                m_pixels->DrawText(62, "0", PURPLE);
                m_pixels->Update(true);

                m_client.setInsecure();
                httpUpdate.update(m_client,
                                  "https://" + String(FW_DOWNLOAD_ADDRESS));
                break;
        }
    }

    virtual void Up(const Button::Event_e evt) override {
        if (evt == Button::PRESS) {
            if (m_state == ASKING) {
                m_state = UPDATING;
            }
        }
    }
    virtual void Down(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
        }
    }
    // any left/right button press will exit this display

  private:
    String GetVersionFromServer() {
        String version;
        HTTPClient https;
        https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        m_client.setInsecure();
        if (https.begin(m_client, "https://" + String(FW_VERSION_ADDR))) {
            const int httpCode = https.GET();

            // file found at server
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                // success!
                version = https.getString();
                version = version.substring(0, 4);
            } else {
                DPRINT("ERROAR: %d  \n", httpCode);
                // TODO: ERROAR DISPLAY of httpCode
            }

            https.end();
        }

        return version;
    }
};
