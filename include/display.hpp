#pragma once
#include <arduino_hal.hpp>
#include <memory>  // for std::shared_ptr
#include <vector>  // for std::vector

#include <button.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>

class DisplayManager;  // forward declaration

// The purpose of this class is to provide easy access to the Joystick,
// LEDs, and light sensor. The DisplayManager component allows easily
// navigating between displays by using the joystick left and right directions.
//
// Each Display can choose to fully override the Up, Down, Left, Right, and
// Press behavior.
// For Left and Right, returning 'true' indicates that the
// Display handled the button, while returning 'false' indicates that
// the DisplayManager should process the button instead, effectively
// changing the Display that the user is seeing.
//
// For example, the FoxieClock display only returns 'false' when the user
// holds the joystick left or right (causing a repeat every 500ms),
// otherwise it returns true (successfully handled).
class Display {
  protected:
    friend class DisplayManager;
    friend class ConfigMenu;
    String m_name;
    bool m_done{false};

    std::shared_ptr<Pixels> m_pixels;
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<Rtc> m_rtc;
    DisplayManager* m_manager{nullptr};  // set by DisplayManager::Add()

  protected:
    virtual ~Display() {}

  public:
    virtual void Initialize() {}  // called exactly once after adding
    virtual void Activate() {}
    virtual void Update() = 0;  // called every loop while active
    virtual void Hide() {}
    virtual void Up(const Button::Event_e evt) {}
    virtual void Down(const Button::Event_e evt) {}
    virtual void Press(const Button::Event_e evt) {}

    virtual bool Left(const Button::Event_e evt) { return false; }
    virtual bool Right(const Button::Event_e evt) { return false; }
    virtual bool ShouldTimeout() { return true; }
    virtual void Timeout() {}

    bool IsDone() const { return m_done; }

    DisplayManager* GetManager() { return m_manager; }
};

class DisplayManager {
  private:
    enum {
        TIMEOUT_MS = 10000,
    };

    std::shared_ptr<Pixels> m_pixels;
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<Rtc> m_rtc;
    std::shared_ptr<Joystick> m_joy;

    std::vector<std::shared_ptr<Display>> m_displays;
    size_t m_activeDisplay{0};
    size_t m_defaultDisplay{0};
    size_t m_lastActiveDisplay{0};
    bool m_isTempDisplay{false};
    bool m_isFirstUpdate{true};

    ElapsedTime m_timeSinceButtonPress;
    ElapsedTime m_sinceLastUpdate;

  public:
    DisplayManager(std::shared_ptr<Pixels> pixels,
                   std::shared_ptr<Settings> settings,
                   std::shared_ptr<Rtc> rtc,
                   std::shared_ptr<Joystick> joy);

    void Add(std::shared_ptr<Display> display);

    void Update();

    std::shared_ptr<Display> GetActive();

    void SetDefaultAndActivateDisplay(const size_t displayNum);

    void ActivateDisplay(const size_t displayNum);

    void ActivateTemporaryDisplay(std::shared_ptr<Display> display);

  private:
    void ConfigureJoystick();

    void ResetTimeSinceButtonPress();
    size_t GetTimeSinceButtonPress();
};
