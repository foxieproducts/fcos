#pragma once
#include <stdint.h>

#include <animators.hpp>
#include <button.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <set_time.hpp>
#include <settings.hpp>

class Clock : public Display {
  private:
    std::shared_ptr<Rtc> m_rtc;
    size_t m_animMode{0};
    bool m_shouldSaveSettings{false};

    ElapsedTime m_waitingToSaveSettings;
    ElapsedTime m_sinceStartedConfigMode;
    ElapsedTime m_sinceButtonPressed;
    bool m_showingColorWheel{false};

    bool m_blinkerRunning{false};
    ElapsedTime m_blinkerTimer;

    uint8_t m_colorWheelPos{0};
    RgbColor m_currentColor{0};
    std::shared_ptr<Animator> m_anim;

  public:
    Clock(std::shared_ptr<Rtc> rtc) : m_rtc(rtc) {}

    virtual void Activate() override {
        LoadSettings();
        m_showingColorWheel = false;
        SetAnimator(CreateAnimator((AnimatorType_e)m_animMode));
    }

    virtual void Update() override {
        RgbColor color = Pixels::ColorWheel(m_colorWheelPos);
        m_currentColor = color;
        m_pixels->Darken();
        m_anim->Update();
        if (m_showingColorWheel) {
#if FCOS_FOXIECLOCK
            // m_pixels->DrawColorWheelBetween(m_colorWheelPos, 82, 90);
#endif
        }
        DrawClockDigits(m_currentColor);
        if ((m_pixels->GetBrightness() >= 0.05f ||
             (*m_settings)["MINB"] != "0")) {
            DrawSeparator(8, m_currentColor);
        }

        CheckIfWaitingToSaveSettings();
    }

    void SetAnimator(std::shared_ptr<Animator> anim) {
        m_anim = anim;
        m_anim->pixels = m_pixels;
        m_anim->rtc = m_rtc;
        m_anim->SetColor(m_colorWheelPos);
    }

  protected:
    virtual void Up(const Button::Event_e evt) override {
        if (!m_anim->CanChangeColor()) {
            return;
        }
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_colorWheelPos -= 3;  // uint8_t wrapping is intended
            m_anim->SetColor(m_colorWheelPos);
            (*m_settings)["COLR"] = m_colorWheelPos;

            m_sinceStartedConfigMode.Reset();
            m_showingColorWheel = true;
            PrepareToSaveSettings();
        }
    };

    virtual void Down(const Button::Event_e evt) {
        if (!m_anim->CanChangeColor()) {
            return;
        }
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_colorWheelPos += 3;  // uint8_t wrapping is intended
            m_anim->SetColor(m_colorWheelPos);
            (*m_settings)["COLR"] = m_colorWheelPos;

            m_sinceStartedConfigMode.Reset();
            m_showingColorWheel = true;
            PrepareToSaveSettings();
        }
    };

    virtual bool Left(const Button::Event_e evt) {
        return evt != Button::REPEAT;  // when held, move to SetTime display
    };

    virtual bool Right(const Button::Event_e evt) {
        return evt != Button::REPEAT;  // when held, move to ConfigMenu display
    };

    // toggles between configured MINB=user and temporary MINB=0
    virtual void Press(const Button::Event_e evt) override {
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
            SetAnimator(CreateAnimator((AnimatorType_e)m_animMode));
            (*m_settings)["ANIM"] = m_animMode + 1;
            PrepareToSaveSettings();
        } else if (evt == Button::REPEAT) {
            m_pixels->ToggleDarkMode();
            toggledDarkMode = true;
        }
    }

  private:
    void DrawClockDigits(const RgbColor color) {
        char text[10];
        if ((*m_settings)["24HR"] == "24") {
            sprintf(text, "%02d:%02d", m_rtc->Hour(), m_rtc->Minute());
        } else {
            sprintf(text, "%2d:%02d", m_rtc->Hour12(), m_rtc->Minute());
        }
#if FCOS_FOXIECLOCK
        m_pixels->DrawChar(0, text[0], m_anim->digitColors[0]);
        m_pixels->DrawChar(20, text[1], m_anim->digitColors[1]);
        //                         [2] is the colon
        m_pixels->DrawChar(42, text[3], m_anim->digitColors[2]);
        m_pixels->DrawChar(62, text[4], m_anim->digitColors[3]);
#elif FCOS_CARDCLOCK
        m_pixels->DrawText(10, text, color);
#endif
    }

    void DrawSeparator(const int x, RgbColor color) {
        if (m_rtc->Second() % 2 && !m_blinkerRunning) {
            m_blinkerRunning = true;
            m_blinkerTimer.Reset();
        }
        const uint8_t pos = m_colorWheelPos;
        const auto colorWhl = &Pixels::ColorWheel;
        const auto scale = &Pixels::ScaleBrightness;

        RgbColor bottomColor = colorWhl(pos - 8);  // + 128
        RgbColor topColor = colorWhl(pos - 8);     // + 64

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
#endif
    }

    void PrepareToSaveSettings() {
        m_shouldSaveSettings = true;
        m_waitingToSaveSettings.Reset();
    }

    void CheckIfWaitingToSaveSettings() {
        if (m_shouldSaveSettings && m_waitingToSaveSettings.Ms() >= 2000) {
            // wait until 2 seconds after changing the color to save
            // settings, since the user can quickly change either one and we
            // want to save flash write cycles

            ElapsedTime saveTime;
            m_settings->Save();
            TDPRINT(m_rtc, "Saved settings in %dms                          \n",
                    saveTime.Ms());  // usually ~25ms
            m_shouldSaveSettings = false;
            m_showingColorWheel = false;
        }
    }

    void LoadSettings() {
        if (!(*m_settings).containsKey("WLED")) {
            (*m_settings)["WLED"] = "ON";
        }

        if (!(*m_settings).containsKey("COLR")) {
            (*m_settings)["COLR"] = m_colorWheelPos;
        } else {
            m_colorWheelPos = (*m_settings)["COLR"].as<uint32_t>();
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
};