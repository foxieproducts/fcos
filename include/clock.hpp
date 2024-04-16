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

    bool m_blinkerRunning{false};
    ElapsedTime m_blinkerTimer;

    RgbColor m_currentColor{0};
    std::shared_ptr<Animator> m_anim;

  public:
    Clock(std::shared_ptr<Rtc> rtc) : Display(), m_rtc(rtc) {}

    virtual void Activate();
    virtual void Update() override;

    void SetAnimator(std::shared_ptr<Animator> anim);

  protected:
    virtual void Up(const Button::Event_e evt) override;
    virtual void Down(const Button::Event_e evt) override;
    virtual bool Left(const Button::Event_e evt) override;
    virtual bool Right(const Button::Event_e evt) override;
    virtual void Press(const Button::Event_e evt) override;

  private:
    void DrawClockDigits(const RgbColor color);
    void DrawSeparator(const int x, RgbColor color);
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();
};