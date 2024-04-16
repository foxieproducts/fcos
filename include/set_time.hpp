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
    SetTime(std::shared_ptr<Rtc> rtc);

    virtual void Activate() override;
    virtual void Update() override;
    virtual void Hide() override;
    virtual void Up(const Button::Event_e evt) override;
    virtual void Down(const Button::Event_e evt) override;
    virtual bool Left(const Button::Event_e evt) override;
    virtual bool Right(const Button::Event_e evt) override;
    virtual void Timeout() override;

  private:
    void SetRTCIfTimeChanged();
    void DrawTime();
    void DrawAnalog(const RgbColor color);
};
