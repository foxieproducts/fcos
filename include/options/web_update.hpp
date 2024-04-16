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
    WebUpdate() : Display() { m_name = "UPDT"; }

    virtual void Activate() override { m_state = CHECKING; }

    virtual void Update() override {
        m_pixels->Clear();

        int yPos = 0;
#ifdef FCOS_CARDCLOCK2
        yPos = 3;
#endif

        switch (m_state) {
            case CHECKING:
                m_pixels->Clear();
#if FCOS_FOXIECLOCK
                m_pixels->Set(LED_OPT_UPDT, YELLOW);
                m_pixels->DrawChar(8, ':', YELLOW);
#else
                m_pixels->DrawText(6, yPos, "...", YELLOW);
#endif
                m_pixels->Update();
                m_webVersion = GetVersionFromServer();
                if (m_webVersion.length() > 0) {
                    m_state = ASKING;
                    m_timeSinceAnimation.Reset();
                } else {
#if FCOS_FOXIECLOCK
                    m_pixels->Set(LED_OPT_UPDT, RED);
                    m_pixels->DrawChar(8, ':', RED);
                    m_pixels->Update();
#else
                    m_pixels->DrawTextScrolling("NOT CONNECTED", RED);
#endif
                    ElapsedTime::Delay(1000);
                    m_done = true;
                }
                break;

            case ASKING: {
                static bool isOn{true};
#if FCOS_FOXIECLOCK
                m_pixels->DrawText(20, m_webVersion, ORANGE);
                m_pixels->Set(LED_OPT_UPDT, BLUE);
                m_pixels->Set(LED_JOY_UP, isOn ? GREEN : BLACK);
#else
                m_pixels->DrawText(0, yPos, m_webVersion, ORANGE);
                m_pixels->DrawChar(14, 0, CHAR_UP_ARROW, isOn ? GREEN : GRAY);
#endif
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
#if FCOS_FOXIECLOCK
                    sprintf(str, "%3d", percentComplete);
                    m_pixels->DrawText(20, str,
                                       percentComplete < 100 ? PURPLE : GREEN);
                    m_pixels->Set(LED_OPT_UPDT,
                                  percentComplete < 100 ? PURPLE : GREEN);
#else
                    sprintf(str, "%3d%%", percentComplete);
                    m_pixels->DrawText(0, yPos, str,
                                       percentComplete < 100 ? PURPLE : GREEN);
#endif
                    m_pixels->Update();
                });
                httpUpdate.onError([&](int error) {
                    char str[10];
                    sprintf(str, "%03d", error);
                    m_pixels->Clear();
#if FCOS_FOXIECLOCK
                    m_pixels->DrawText(20, str, RED);
#else
                    m_pixels->DrawText(5, yPos, str, RED);
#endif
                    m_pixels->Update();
                    ElapsedTime::Delay(5000);
                });
                httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
                httpUpdate.rebootOnUpdate(true);

                m_pixels->Clear();
#if FCOS_FOXIECLOCK
                m_pixels->Set(LED_OPT_UPDT, BLUE);
                m_pixels->DrawText(62, "0", PURPLE);
#else
                m_pixels->DrawText(8, yPos, "0%", PURPLE);
#endif
                m_pixels->Update();

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
