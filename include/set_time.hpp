#pragma once

#include <button.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>

class SetTime : public Display {
  private:
    enum Mode_e {
        SET_SECOND,
        SET_MINUTE,
        SET_HOUR,
    };

    std::shared_ptr<Rtc> m_rtc;
    Mode_e m_mode{SET_MINUTE};
    int m_hour, m_minute, m_second;
    bool m_timeChanged{false};
    bool m_secondsChanged{false};

  public:
    SetTime(std::shared_ptr<Rtc> rtc) : m_rtc(rtc) {}

    virtual void Activate() override {
        m_mode = SET_HOUR;
        m_timeChanged = false;
        m_secondsChanged = false;
    }

    virtual void Update() override {
        m_pixels->Darken();

        if (!m_timeChanged) {
            m_hour = m_rtc->Hour();
            m_minute = m_rtc->Minute();
        }

        if (!m_secondsChanged) {
            m_second = m_rtc->Second();
        }

        DrawTime();
    }

    virtual void Hide() override {
        if (m_mode == SET_SECOND) {
            m_mode = SET_HOUR;
            DrawTime();
        }
        SetRTCIfTimeChanged();
    }

    virtual void Up(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_timeChanged = true;
            switch (m_mode) {
                case SET_HOUR:
                    if (++m_hour > 23) {
                        m_hour = 0;
                    }
                    break;
                case SET_MINUTE:
                    if (++m_minute > 59) {
                        m_minute = 0;
                    }
                    break;
                case SET_SECOND:
                    m_secondsChanged = true;
                    if (++m_second > 59) {
                        m_second = 0;
                    }
                    break;
            }
        }
    }

    virtual void Down(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_timeChanged = true;
            switch (m_mode) {
                case SET_HOUR:
                    if (m_hour-- == 0) {
                        m_hour = 23;
                    }
                    break;
                case SET_MINUTE:
                    if (m_minute-- == 0) {
                        m_minute = 59;
                    }
                    break;
                case SET_SECOND:
                    m_secondsChanged = true;
                    if (m_second-- == 0) {
                        m_second = 59;
                    }
                    break;
            }
        }
    }

    virtual bool Left(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_mode == SET_HOUR) {
                if (evt == Button::REPEAT) {
                    return false;  // move to next Display to the left
                }
            } else if (m_mode == SET_MINUTE) {
                m_mode = SET_HOUR;
            } else if (m_mode == SET_SECOND) {
                m_mode = SET_MINUTE;
            }
        }
        return true;
    }

    virtual bool Right(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_mode == SET_HOUR) {
                m_mode = SET_MINUTE;
            } else if (m_mode == SET_MINUTE) {
                m_mode = SET_SECOND;
            } else if (m_mode == SET_SECOND) {
                if (evt == Button::REPEAT) {
                    return false;  // move to next Display to the right
                }
            }
        }
        return true;
    }

    virtual void Timeout() override {}

  private:
    void SetRTCIfTimeChanged() {
        if (m_timeChanged) {
            m_rtc->SetTime(m_hour, m_minute,
                           m_secondsChanged ? m_second : m_rtc->Second());
        }
    }

    void DrawTime() {
        RgbColor selectedCol = PURPLE, unselectedCol = LIGHT_GRAY;
        if (m_pixels->GetBrightness() <= 0.01f &&
            ((*m_settings)["MINB"] == "0" || m_pixels->IsDarkModeEnabled())) {
            selectedCol = BLUE;
        } else if (m_rtc->Millis() < 500) {
            selectedCol = DARK_PURPLE;
        }

        char text[10];
        if (m_mode == SET_SECOND) {
            sprintf(text, "%02d", m_minute);
            m_pixels->DrawText(
                0, text, m_mode == SET_MINUTE ? selectedCol : unselectedCol);

            sprintf(text, "%02d", m_second);
            m_pixels->DrawText(
                42, text, m_mode == SET_SECOND ? selectedCol : unselectedCol);
        } else {
            if ((*m_settings)["24HR"] == "24") {
                sprintf(text, "%02d", m_hour);
            } else {
                sprintf(text, "%2d", m_rtc->Conv24to12(m_hour));
            }
            m_pixels->DrawText(
                0, text, m_mode == SET_HOUR ? selectedCol : unselectedCol);

            sprintf(text, "%02d", m_minute);
            m_pixels->DrawText(
                42, text, m_mode == SET_MINUTE ? selectedCol : unselectedCol);
        }

        m_pixels->DrawChar(8, ':', DARK_PURPLE);

        DrawAnalog(selectedCol);
    }

    void DrawAnalog(const RgbColor color) {
#if FCOS_CARDCLOCK
        m_pixels->ClearRoundLEDs(DARK_GRAY);
        m_pixels->DrawSecondLEDs(m_second,
                                 m_mode == SET_SECOND ? color : WHITE);
        m_pixels->DrawHourLED(m_rtc->Conv24to12(m_hour),
                              m_mode == SET_HOUR ? color : WHITE);
        m_pixels->DrawMinuteLED(m_minute, m_mode == SET_MINUTE ? color : WHITE);
#endif
    }
};
