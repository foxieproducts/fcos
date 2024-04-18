#include <hardware.hpp>
#include <light_sensor.hpp>

static void HardwareTest_CC2(std::shared_ptr<Pixels> pixels,
                             std::shared_ptr<Settings> settings,
                             std::shared_ptr<Rtc> rtc,
                             std::shared_ptr<Joystick> joy);

void ShowSerialStatusMessage(std::shared_ptr<Pixels> pixels,
                             std::shared_ptr<Rtc> rtc) {
    static ElapsedTime statusDisplayTimer;
    if (statusDisplayTimer.Ms() >= 50) {
        statusDisplayTimer.Reset();
        TDPRINT(rtc, "Light Sensor:%.1f%% - Uptime:%ds - WiFi:%d \r",
                pixels->GetBrightness() * 100, rtc->Uptime(),
                WiFi.isConnected());
    }
}

void DoHardwareStartupTests(std::shared_ptr<Pixels> pixels,
                            std::shared_ptr<Settings> settings,
                            std::shared_ptr<Rtc> rtc,
                            std::shared_ptr<Joystick> joy) {
#if FCOS_ESP32_C3
    Serial.begin();
#elif FCOS_ESP8266
    Serial.begin(115200);
#endif

#if FCOS_FOXIECLOCK
    (*settings)["TEST"] = 1;
#endif

    if (joy->AreAnyButtonsPressed() == PIN_BTN_UP ||
        (*settings)["TEST"].as<int>() == 0) {
#if FCOS_CARDCLOCK2
        HardwareTest_CC2(pixels, settings, rtc, joy);
#endif
    } else if (joy->AreAnyButtonsPressed() == PIN_BTN_LEFT) {
        // clear settings and restart
        pixels->Clear();
        pixels->DrawChar(8, ':', ORANGE);
        pixels->Show();
        joy->WaitForNoButtonsPressed();
        settings->clear();
        settings->Save();
        rtc->SetClockToZero();
        WiFi.disconnect(true, true);
        ElapsedTime::Delay(50);
        ESP.restart();
    }
}

void HardwareTest_CC2(std::shared_ptr<Pixels> pixels,
                      std::shared_ptr<Settings> settings,
                      std::shared_ptr<Rtc> rtc,
                      std::shared_ptr<Joystick> joy) {
    auto beginUptime = rtc->Uptime();
    pixels->DrawChar(0, 3, CHAR_UP_ARROW, ORANGE);
    pixels->Show();
    joy->WaitForNoButtonsPressed();
    joy->WaitForButton(joy->up);
    pixels->DrawChar(0, 3, CHAR_UP_ARROW, GREEN);

    pixels->DrawChar(4, 3, CHAR_DOWN_ARROW, ORANGE);
    pixels->Show();
    joy->WaitForButton(joy->down);
    pixels->DrawChar(4, 3, CHAR_DOWN_ARROW, GREEN);

    pixels->DrawChar(8, 3, CHAR_RIGHT_ARROW, ORANGE);
    pixels->Show();
    joy->WaitForButton(joy->right);
    pixels->DrawChar(8, 3, CHAR_RIGHT_ARROW, GREEN);

    pixels->DrawChar(12, 3, CHAR_LEFT_ARROW, ORANGE);
    pixels->Show();
    joy->WaitForButton(joy->left);
    pixels->DrawChar(12, 3, CHAR_LEFT_ARROW, GREEN);

    pixels->Clear();
    pixels->DrawText(0, 3, "PRES", ORANGE);
    pixels->Show();
    joy->WaitForButton(joy->press);

    joy->WaitForNoButtonsPressed();
    for (int i = 0; i < TOTAL_ALL_LEDS; ++i) {
        pixels->Set(i, Pixels::ColorWheel(i));
    }
    pixels->Show();
    joy->WaitForButton(joy->press);

    pixels->Clear();
    pixels->Show();
    LightSensor ls;
    ls.SetHwMax(LightSensor::HW_MAX);

    bool found = false;
    for (size_t i = 0; i <= 10; ++i) {
        ls.SetHwMin(LightSensor::HW_MIN + i);
        ls.ResetToCurrentSensorValue();
        if (ls.GetScaled() < 0.001f) {
            found = true;
            DPRINT("Found min:%d\n", LightSensor::HW_MIN + i);
            break;
        }
    }
    if (!found) {
        pixels->Clear();
        pixels->DrawText(0, 3, "ERR1", PURPLE);
        pixels->Show();
        joy->WaitForButton(joy->press);
    } else {
        (*settings)["LS_HW_MIN"] = ls.GetHwMin();
        (*settings)["LS_HW_MAX"] = ls.GetHwMax();
        DPRINT("LS_HW_MIN: %d\n", ls.GetHwMin());
    }

    pixels->Clear();
    if (rtc->Uptime() != beginUptime) {
        pixels->DrawText(0, 3, "ERR2", RED);
        pixels->Show();
    } else {
        pixels->DrawText(1, 3, " OK ", GREEN);  // everything and RTC is good
        pixels->Show();
        (*settings)["TEST"] = 1;  // test success
        (*settings).Save();
        joy->WaitForNoButtonsPressed();
    }
    ElapsedTime::Delay(1000);
}
