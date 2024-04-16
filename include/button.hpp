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
    Button(const int pin);

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

    void Update();

    bool IsPressed() const;
    size_t GetTimeInState() const;

    void SetEnabled(const bool enabled);

    void Reset();

    void SetDigitalReadFunc(const DigitalReadFunc_t& func);

  private:
    void SendEvent(const Event_e evt);

    bool MustDelayBeforePress() const;
    bool ShouldSendDelayedPress() const;

    bool ShouldRepeat();

    bool HasPinStateChanged();

    bool IsDebouncing();
};

class Joystick {
  private:
    std::vector<Button*> m_buttons;

  public:
    Button up{PIN_BTN_UP};
    Button down{PIN_BTN_DOWN};
    Button left{PIN_BTN_LEFT};
    Button right{PIN_BTN_RIGHT};
    Button press{PIN_BTN_PRESS};

    Joystick();

    int AreAnyButtonsPressed();
    bool WaitForButton(const Button& btn, const int ms = -1);

    void WaitForNoButtonsPressed();

    void Reset();

    void Update();
};
