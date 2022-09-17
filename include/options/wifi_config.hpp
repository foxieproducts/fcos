#pragma once
#include <WiFiManager.h>

#include <elapsed_time.hpp>
#include <options/numeric.hpp>

class WiFiConfig : public Numeric {
    enum {
        BOOT_INIT_WAIT_MS = 2000,
    };

    WiFiManager m_wifiMgr;
    ElapsedTime m_wifiLedAnim;
    bool m_isInitialized{false};

    String m_timeZonesHtml;
    WiFiManagerParameter* m_setupHtmlParam{nullptr};

  public:
    // 0 = off, 1 = on, 2 = show config portal
    WiFiConfig() : Numeric("WIFI", std::vector<String>({"0", "1", "2"})) {}

    virtual void Initialize() override {
        if ((*m_settings)["WIFI"] == "0") {
            WiFi.disconnect();
        } else {
            WiFi.begin();
            m_rtc->ForceNTPUpdate();
        }

        PrepareConfigPortalParameters();
    }

    virtual void Activate() override { Numeric::Activate(); }

    virtual void Update() override {
        Numeric::Update();
        if (m_wifiMgr.getConfigPortalActive()) {
            Numeric::Hide();  // hide the arrows while the portal is active
            AllowChangingValues(false);

            static bool isBrightGreen = false;
            m_wifiMgr.process();

            if (m_wifiLedAnim.Ms() > 500) {
                m_wifiLedAnim.Reset();
                isBrightGreen = !isBrightGreen;
            }
            m_pixels->Set(41, isBrightGreen ? GREEN : BLUE);
            m_pixels->Set(40, isBrightGreen ? BLUE : GREEN);
        } else {
            AllowChangingValues(true);
        }
    }

    virtual void Hide() override {
        Numeric::Hide();
        if (m_wifiMgr.getConfigPortalActive()) {
            // in the interest of a better user experience, update
            // the display before hiding the portal, since it takes
            // a moment to shut down
            for (size_t i = 42; i < 62; ++i) {
                m_pixels->Set(i, BLACK);
            }
            m_pixels->DrawChar(20, '3', GREEN);
            m_pixels->DrawChar(8, ':', DARK_PURPLE);
            m_pixels->Update();
            m_wifiMgr.stopConfigPortal();
        }
        if ((*m_settings)["wifi_configured"] != "1") {
            (*m_settings)["WIFI"] = "0";
        } else if ((*m_settings)["wifi_configured"] == "1" &&
                   (*m_settings)["WIFI"] == "2") {
            (*m_settings)["WIFI"] = "1";
        }
    }

    virtual bool ShouldTimeout() override {
        return !m_wifiMgr.getConfigPortalActive();
    }

  private:
    virtual void ChangeSettingToCurrentValue() override {
        Numeric::ChangeSettingToCurrentValue();

        if ((*m_settings)["WIFI"] == "2" ||
            ((*m_settings)["WIFI"] == "1" &&
             (*m_settings)["wifi_configured"] != "1")) {
            (*m_settings)["WIFI"] = "2";
            if ((*m_settings)["wifi_configured"] != "1") {
                m_wifiMgr.resetSettings();
            }
            Activate();
            m_wifiMgr.startConfigPortal("Foxie_WiFiSetup");
        }
        if (WiFi.isConnected() && (*m_settings)["WIFI"] == "0") {
            DPRINT("Disabling WiFi                                  \n");
            WiFi.disconnect();
        } else if (!WiFi.isConnected() && (*m_settings)["WIFI"] == "1") {
            DPRINT("Enabling WiFi                                   \n");
            WiFi.begin();
            m_rtc->ForceNTPUpdate();
        }
    }

    String GetHttpParam(String name) {
        String value;
        if (m_wifiMgr.server->hasArg(name)) {
            value = m_wifiMgr.server->arg(name);
        }
        return value;
    }

    void PrepareConfigPortalParameters() {
        m_wifiMgr.setConfigPortalBlocking(false);
        m_wifiMgr.setDarkMode(true);
        m_wifiMgr.setSaveConfigCallback([&]() {
            (*m_settings)["WIFI"] = "1";
            (*m_settings)["wifi_configured"] = "1";
            Activate();
            Update();

            ElapsedTime successAnim;
            bool isLedOn = true;
            while (successAnim.Ms() < 1000) {
                m_pixels->Set(41, isLedOn ? GREEN : BLACK);
                m_pixels->Set(40, isLedOn ? GREEN : BLACK);
                isLedOn = !isLedOn;
                m_pixels->Update();
                ElapsedTime::Delay(75);
            }
        });
        m_wifiMgr.setSaveParamsCallback([&]() {
            // do stuff with the params (timezone)
            auto names = m_rtc->GetTimezoneNames();
            size_t selected = GetHttpParam("tz_select").toInt();
            selected = selected > 0 && selected < names.size() ? selected : 0;
            (*m_settings)["timezone"] = names[selected];
            (*m_settings)["TIMEZONE"] = selected;
            m_rtc->ForceNTPUpdate();
        });
        m_wifiMgr.setConfigPortalTimeout(60);

        std::vector<const char*> menu = {"wifi", "param", "sep", "info",
                                         "exit"};
        m_wifiMgr.setMenu(menu);

        m_timeZonesHtml = R"(
            <br/>
            <label for='tz_select'>Select Timezone (Note: only affects clock when on WiFi):</label>
            <select name="tz_select" id="tz_select" class="button">
            )";

        auto names = m_rtc->GetTimezoneNames();
        size_t index = 0;
        for (const auto& name : names) {
            m_timeZonesHtml += "<option value='" + String(index) + "'";
            if ((*m_settings)["TIMEZONE"].as<size_t>() == index) {
                m_timeZonesHtml += " selected";
            }
            m_timeZonesHtml += ">" + name + "</option>";
            index++;
        }
        m_timeZonesHtml += "</select>";

        m_setupHtmlParam = new WiFiManagerParameter(m_timeZonesHtml.c_str());
        m_wifiMgr.addParameter(m_setupHtmlParam);
    }
};