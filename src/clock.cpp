#include <clock.hpp>

void Clock::Activate() {
    LoadSettings();
    SetAnimator(CreateAnimator(m_pixels, m_settings, m_rtc,
                               (AnimatorType_e)m_animMode));
}

void Clock::Update() {
    auto wheelPos = (*m_settings)["COLR"].as<uint8_t>();
    RgbColor color = wheelPos;
    m_currentColor = color;
    m_pixels->Darken();
#if FCOS_FOXIECLOCK
    m_anim->Update();
    DrawClockDigits(m_currentColor);
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    m_anim->Update();
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        m_pixels->ClearRoundLEDs({1, 1, 1});
    }

    int brightestLED = 0;
    if (m_rtc->Millis() >= 666)
        brightestLED = 2;
    else if (m_rtc->Millis() >= 333)
        brightestLED = 1;

    m_pixels->DrawSecondLEDs(m_rtc->Second(), m_anim->digitColors[0],
                             brightestLED);

    m_pixels->DrawMinuteLED(m_rtc->Minute(), m_anim->digitColors[1]);
    m_pixels->DrawHourLED(m_rtc->Hour12(), m_anim->GetColonColor());
    DrawClockDigits(m_currentColor);

#endif

    CheckIfWaitingToSaveSettings();
}

void Clock::SetAnimator(std::shared_ptr<Animator> anim) {
    m_anim = anim;
    m_anim->Start();
    m_anim->SetColor((*m_settings)["COLR"].as<uint8_t>());
}

void Clock::Up(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Up();
        PrepareToSaveSettings();
    }
};

void Clock::Down(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Down();
        PrepareToSaveSettings();
    }
};

bool Clock::Left(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Left();
        // temporary hack for photos
        // m_rtc->SetTime(12, 34, 00);
    }
    return evt != Button::REPEAT;  // when held, move to SetTime display
};

bool Clock::Right(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Right();
    }
    return evt != Button::REPEAT;  // when held, move to ConfigMenu display
};

// toggles between configured MINB=user and temporary MINB=0
void Clock::Press(const Button::Event_e evt) {
    static bool toggledDarkMode = false;
    if (evt == Button::RELEASE) {
        if (toggledDarkMode) {
            // don't want to change anim mode when toggling dark mode
            toggledDarkMode = false;
            return;
        }

        if (++m_animMode >= ANIM_TOTAL) {
            m_animMode = 0;
        }
        SetAnimator(CreateAnimator(m_pixels, m_settings, m_rtc,
                                   (AnimatorType_e)m_animMode));
        (*m_settings)["ANIM"] = m_animMode + 1;
        m_pixels->Clear();
        Joystick joy;
#if FCOS_CARDCLOCK2
        uint8_t pos = 0;
        for (int i = 0; i < 0 + (m_anim->name.length() * 4); i++) {
            m_pixels->Clear();
            m_pixels->DrawText(0 - i, 3, m_anim->name, LIGHT_GRAY);
            m_pixels->Show();
            if (joy.WaitForButton(joy.press, 75)) {
                ElapsedTime::Delay(25);
            }
        }
#else
        m_pixels->DrawText(0, 0, (*m_settings)["ANIM"], LIGHT_GRAY);
        m_pixels->Show();
        joy.WaitForButton(joy.press, 500);
#endif
        joy.WaitForNoButtonsPressed();
        PrepareToSaveSettings();
    } else if (evt == Button::REPEAT) {
        m_pixels->ToggleDarkMode();
        toggledDarkMode = true;
    }
}

void Clock::DrawClockDigits(const RgbColor color) {
    char text[10];
    if ((*m_settings)["24HR"] == "24") {
        sprintf(text, "%02d:%02d", m_rtc->Hour(), m_rtc->Minute());
    } else {
        sprintf(text, "%2d:%02d", m_rtc->Hour12(), m_rtc->Minute());
    }
    int yPos = 0;
#if FCOS_FOXIECLOCK
    m_pixels->DrawChar(0, text[0], m_anim->digitColors[0]);
    m_pixels->DrawChar(20, text[1], m_anim->digitColors[1]);
    //                         [2] is the colon
    m_pixels->DrawChar(42, text[3], m_anim->digitColors[2]);
    m_pixels->DrawChar(62, text[4], m_anim->digitColors[3]);

    if ((m_pixels->GetBrightness() >= 0.05f || (*m_settings)["MINB"] != "0")) {
    }
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
#if FCOS_CARDCLOCK2
    yPos = 3;
#endif
    m_pixels->DrawChar(0, yPos, text[0], m_anim->digitColors[0]);
    m_pixels->DrawChar(4, yPos, text[1], m_anim->digitColors[1]);
    //                         [2] is the colon
    m_pixels->DrawChar(10, yPos, text[3], m_anim->digitColors[2]);
    m_pixels->DrawChar(14, yPos, text[4], m_anim->digitColors[3]);
#endif  // FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        DrawSeparator(8, m_anim->GetColonColor());
    } else {
#if FCOS_FOXIECLOCK
        // m_pixels->DrawChar(8, yPos, ':', m_anim->GetColonColor());
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
        m_pixels->DrawChar(8, yPos, ':', m_anim->GetColonColor());
#endif
    }
}

void Clock::DrawSeparator(const int x, RgbColor color) {
    if (m_rtc->Second() % 2 && !m_blinkerRunning) {
        m_blinkerRunning = true;
        m_blinkerTimer.Reset();
    }
    const auto scale = &Pixels::ScaleBrightness;
    RgbColor bottomColor = m_anim->GetColonColor();
    RgbColor topColor = bottomColor;

    if (m_blinkerRunning) {
        const float progress = m_blinkerTimer.Ms() / 1900.0f;
        float brightness = progress < 0.5f ? progress : 1.0f - progress;
        bottomColor = scale(bottomColor, 0.55f - brightness);
        topColor = scale(topColor, 0.05f + brightness);

        if (m_blinkerTimer.Ms() >= 1900 ||
            (!(m_rtc->Second() % 2) && m_rtc->Millis() > 900)) {
            m_blinkerRunning = false;
        }
    } else {
        bottomColor = scale(bottomColor, 0.55f);
        topColor = scale(topColor, 0.05f);
    }

#if FCOS_FOXIECLOCK
    m_pixels->Set(40, bottomColor);
    m_pixels->Set(41, topColor);
#elif FCOS_CARDCLOCK
    m_pixels->Set(x, 1, bottomColor);
    m_pixels->Set(x, 3, topColor);
#elif FCOS_CARDCLOCK2
    m_pixels->Set(x, 3 + 1, bottomColor);
    m_pixels->Set(x, 3 + 3, topColor);
#endif
}

void Clock::PrepareToSaveSettings() {
    m_shouldSaveSettings = true;
    m_waitingToSaveSettings.Reset();
}

void Clock::CheckIfWaitingToSaveSettings() {
    if (m_shouldSaveSettings && m_waitingToSaveSettings.Ms() >= 2000) {
        // wait until 2 seconds after changing the color to save
        // settings, since the user can quickly change either one and we
        // want to save flash write cycles

        ElapsedTime saveTime;
        m_settings->Save();
        TDPRINT(m_rtc, "Saved settings in %dms                          \n",
                saveTime.Ms());  // usually ~25ms
        m_shouldSaveSettings = false;
    }
}

void Clock::LoadSettings() {
    if (!(*m_settings).containsKey("WLED")) {
        (*m_settings)["WLED"] = "ON";
    }

    if (!(*m_settings).containsKey("ANIM")) {
        (*m_settings)["ANIM"] = m_animMode + 1;
    } else {
        m_animMode = (*m_settings)["ANIM"].as<int>() - 1;
        if (m_animMode >= ANIM_TOTAL) {
            m_animMode = ANIM_NORMAL;
        }
    }
}
