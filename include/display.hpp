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

  public:
    DisplayManager(std::shared_ptr<Pixels> pixels,
                   std::shared_ptr<Settings> settings,
                   std::shared_ptr<Rtc> rtc,
                   std::shared_ptr<Joystick> joy)
        : m_pixels(pixels), m_settings(settings), m_rtc(rtc), m_joy(joy) {
        ConfigureJoystick();
    }

    void Add(std::shared_ptr<Display> display) {
        m_displays.push_back(display);
        m_activeDisplay = m_displays.size() - 1;
        m_displays.back()->m_pixels = m_pixels;
        m_displays.back()->m_settings = m_settings;
        m_displays.back()->m_rtc = m_rtc;
        if (m_displays.back()->m_manager == nullptr) {
            m_displays.back()->m_manager = this;
            m_displays.back()->Initialize();
        }
    }

    void Update() {
        if (m_pixels->Update() || m_isFirstUpdate) {
            if (m_isTempDisplay) {
                // Update the "parent" display of the temp display
                // this is used by ConfigMenu to allow it to composite the
                // ConfigMenu's Display and the currently displayed option (e.g.
                // 24HR mode)
                m_displays[m_lastActiveDisplay]->Update();
            }

            auto& cur = m_displays[m_activeDisplay];
            cur->Update();
            if (cur->IsDone() || (cur->ShouldTimeout() &&
                                  GetTimeSinceButtonPress() >= TIMEOUT_MS &&
                                  m_activeDisplay != m_defaultDisplay)) {
                cur->Timeout();
                ActivateDisplay(m_defaultDisplay);
            }

            if (m_isFirstUpdate) {
                m_pixels->Update(true);
                m_isFirstUpdate = false;
            }
        }
    }

    std::shared_ptr<Display> GetActive() { return m_displays[m_activeDisplay]; }

    void SetDefaultAndActivateDisplay(const size_t displayNum) {
        m_defaultDisplay = displayNum;
        ActivateDisplay(displayNum);
    }

    void ActivateDisplay(const size_t displayNum) {
        if (displayNum < m_displays.size()) {
            m_displays[m_activeDisplay]->Hide();
            m_activeDisplay = displayNum;
            m_displays[m_activeDisplay]->Activate();
            ResetTimeSinceButtonPress();

            if (m_isTempDisplay && m_activeDisplay != m_displays.size() - 1) {
                m_displays.pop_back();
                m_isTempDisplay = false;
            }
        }
    }

    void ActivateTemporaryDisplay(std::shared_ptr<Display> display) {
        m_lastActiveDisplay = m_activeDisplay;
        m_isTempDisplay = true;
        Add(display);
        ActivateDisplay(m_displays.size() - 1);
    }

  private:
    void ConfigureJoystick() {
        m_joy->left.config.repeatRate = 750;
        m_joy->right.config.repeatRate = 750;
        m_joy->press.config.repeatRate = 750;

        m_joy->press.config.handlerFunc = [&](const Button::Event_e evt) {
            m_displays[m_activeDisplay]->Press(evt);
            ResetTimeSinceButtonPress();
        };

        m_joy->up.config.handlerFunc = [&](const Button::Event_e evt) {
            m_displays[m_activeDisplay]->Up(evt);
            ResetTimeSinceButtonPress();
        };

        m_joy->down.config.handlerFunc = [&](const Button::Event_e evt) {
            m_displays[m_activeDisplay]->Down(evt);
            ResetTimeSinceButtonPress();
        };

        m_joy->left.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_displays[m_activeDisplay]->Left(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeDisplay == 0) {
                    return;  // this _could_ wrap around, but it doesn't for now
                }
                ActivateDisplay(m_activeDisplay - 1);
            }
            ResetTimeSinceButtonPress();
        };

        m_joy->right.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_displays[m_activeDisplay]->Right(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeDisplay == m_displays.size() - 1) {
                    return;  // this _could_ wrap around, but it doesn't for now
                }
                ActivateDisplay(m_activeDisplay + 1);
            }
            ResetTimeSinceButtonPress();
        };
    }

    void ResetTimeSinceButtonPress() { m_timeSinceButtonPress.Reset(); }
    size_t GetTimeSinceButtonPress() { return m_timeSinceButtonPress.Ms(); }
};
