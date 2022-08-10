#include <arduino_hal.hpp>
#include <memory>

#include <clock.hpp>
#include <config_menu.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <fw_update.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>

bool Rtc::m_receivedInterrupt = false;
void ShowSerialStatusMessage(std::shared_ptr<Pixels> pixels,
                             std::shared_ptr<Rtc> rtc);

void setup() {
#if FCOS_ESP32_C3
    Serial.begin();
#elif FCOS_ESP8266
    Serial.begin(115200);
#endif

    auto settings = std::make_shared<Settings>();
    auto pixels = std::make_shared<Pixels>(settings);
    auto rtc = std::make_shared<Rtc>(settings);
    auto joy = std::make_shared<Joystick>();
    auto fw_update = std::make_shared<FwUpdate>(pixels);

    // TODO: check for a button being held on boot to reset all settings
    // (including wifi)

    // These are the "displays" that are available, but more can be added. The
    // DisplayMgr navigates between them using left/right motions, if the
    // Display allows it (e.g. holding left at the Clock activates SetTime)
    auto displayMgr =
        std::make_shared<DisplayManager>(pixels, settings, rtc, joy);

    displayMgr->Add(std::make_shared<SetTime>(rtc));
    displayMgr->Add(std::make_shared<Clock>(rtc));
    displayMgr->Add(std::make_shared<ConfigMenu>());

    // 0 = SetTime   <=>   1 = Clock   <=>   2 = ConfigMenu
    displayMgr->SetDefaultAndActivateDisplay(1);

    for (;;) {  // forever, instead of loop(), because I avoid globals ;)
        ShowSerialStatusMessage(pixels, rtc);
        rtc->Update();
        joy->Update();
        fw_update->Update();
        displayMgr->Update();
        yield();  // allow the ESP platform tasks to run
    }
}

void ShowSerialStatusMessage(std::shared_ptr<Pixels> pixels,
                             std::shared_ptr<Rtc> rtc) {
    static ElapsedTime statusDisplayTimer;
    if (statusDisplayTimer.Ms() >= 50) {
        statusDisplayTimer.Reset();
        TDPRINT(rtc, "Light Sensor: %.1f%% - Uptime: %ds   \r",
                pixels->GetBrightness() * 100, rtc->Uptime());
    }
}

void loop() {}
