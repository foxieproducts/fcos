#pragma once
#include <arduino_hal.hpp>
#include <elapsed_time.hpp>
#include <functional>  // for std::function
#include <vector>      // for std::vector

class Button {
  public:
    enum Event_e {
        PRESS,
        REPEAT,
        RELEASE,
    };

  private:
    using HandlerFunc_t = std::function<void(const Event_e evt)>;
    using DigitalReadFunc_t = std::function<int(const uint8_t pin)>;
    // by default, just use Arduino's digitalRead
    DigitalReadFunc_t m_digitalReadFunc{
        [](const uint8_t pin) { return digitalRead(pin); }};

    const int m_pin;
    bool m_enabled{true};
    bool m_debouncing{false};
    bool m_isPressed{false};
    bool m_pressEventSent{false};

    ElapsedTime m_etInState;
    ElapsedTime m_etSinceRepeat;

  public:
    Button(const int pin) : m_pin(pin) {
#if ARDUINO
        pinMode(pin, INPUT_PULLUP);
#endif
    }

    // public, for easy external configuration. Use with care.
    struct Config {
        bool canRepeat{true};
        // configurable delays (milliseconds)
        size_t repeatRate{50};
        size_t beforeRepeat{400};
        size_t beforePress{0};
        size_t debounceTime{10};
        HandlerFunc_t handlerFunc;
    } config;

    void Update() {
        if (!m_enabled) {
            return;
        }

        if (HasPinStateChanged()) {
            if (m_isPressed && MustDelayBeforePress() == false) {
                SendEvent(PRESS);
            } else if (!m_isPressed) {
                SendEvent(RELEASE);
            }
        }

        if (ShouldSendDelayedPress()) {
            SendEvent(PRESS);
        }

        if (ShouldRepeat()) {
            SendEvent(REPEAT);
        }
    }

    bool IsPressed() const {
        return m_enabled && m_isPressed;
    }
    size_t GetTimeInState() const {
        return m_etInState.Ms();
    }

    void SetEnabled(const bool enabled) {
        m_enabled = enabled;
        Reset();
    }

    void Reset() {
        m_etInState.Reset();
        m_isPressed = m_pressEventSent = m_debouncing = false;
    }

    void SetDigitalReadFunc(const DigitalReadFunc_t& func) {
        m_digitalReadFunc = func;
    }

  private:
    void SendEvent(const Event_e evt) {
        m_etSinceRepeat.Reset();
        if (config.handlerFunc) {
            config.handlerFunc(evt);
        }
        m_pressEventSent = m_isPressed;
    }

    bool MustDelayBeforePress() const {
        return config.beforePress > 0;
    }
    bool ShouldSendDelayedPress() const {
        return m_isPressed && !m_pressEventSent && MustDelayBeforePress() &&
               m_etInState.Ms() >= config.beforePress;
    }

    bool ShouldRepeat() {
        return !HasPinStateChanged() && m_isPressed && config.canRepeat &&
               m_pressEventSent && m_etInState.Ms() >= config.beforeRepeat &&
               (m_etSinceRepeat.Ms() >= config.repeatRate);
    }

    bool HasPinStateChanged() {
        if (IsDebouncing()) {
            return false;
        }

        const bool isPressed = m_digitalReadFunc(m_pin) == LOW;
        if (isPressed != m_isPressed) {
            m_debouncing = true;  // no soup bounce for you!
            m_isPressed = isPressed;
            m_etInState.Reset();
            return true;
        }
        return false;
    }

    bool IsDebouncing() {
        if (!m_debouncing) {
            return false;
        }
        m_debouncing = (m_etInState.Ms() < config.debounceTime);
        return m_debouncing;
    }
};

#if FCOS_FOXIECLOCK
class Joystick {
  private:
    std::vector<Button*> m_buttons;

  public:
    Button up{PIN_BTN_UP};
    Button down{PIN_BTN_DOWN};
    Button left{PIN_BTN_LEFT};
    Button right{PIN_BTN_RIGHT};
    Button press{PIN_BTN_PRESS};

    Joystick() {
        m_buttons.push_back(&up);
        m_buttons.push_back(&down);
        m_buttons.push_back(&left);
        m_buttons.push_back(&right);
        m_buttons.push_back(&press);
    }

    int AreAnyButtonsPressed() {
        if (up.IsPressed()) {
            return PIN_BTN_UP;
        } else if (down.IsPressed()) {
            return PIN_BTN_DOWN;
        } else if (left.IsPressed()) {
            return PIN_BTN_LEFT;
        } else if (right.IsPressed()) {
            return PIN_BTN_RIGHT;
        } else if (press.IsPressed()) {
            return PIN_BTN_PRESS;
        }
        return -1;
    }

    void WaitForNoButtonsPressed() {
        while (AreAnyButtonsPressed() == -1) {
            Update();
            ElapsedTime::Delay(1);
        }
    }

    void Reset() {
        for (auto& btn : m_buttons) {
            btn->Reset();
        }
    }

    void Update() {
        for (auto& btn : m_buttons) {
            btn->Update();
        }
    }
};
#endif
