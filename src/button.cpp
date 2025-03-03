#include <button.hpp>
Button::Button(const int pin) : m_pin(pin) {
#if ARDUINO
    pinMode(pin, INPUT_PULLUP);
#endif
}

void Button::Update() {
    if (!m_enabled) {
        return;
    }

    if (HasPinStateChanged()) {
        if (m_isPressed && MustDelayBeforePress() == false) {
            SendEvent(PRESS);
        } else if (!m_isPressed) {
            SendEvent(RELEASE);
            m_longPressEventSent = false;
        }
    }

    if (ShouldSendDelayedPress()) {
        SendEvent(PRESS);
    }

    if (ShouldSendLongPress()) {
        SendEvent(LONG_PRESS);
        m_longPressEventSent = true;
    }

    if (ShouldRepeat()) {
        SendEvent(REPEAT);
    }
}

bool Button::IsPressed() const {
    return m_enabled && m_isPressed;
}
size_t Button::GetTimeInState() const {
    return m_etInState.Ms();
}

void Button::SetEnabled(const bool enabled) {
    m_enabled = enabled;
    Reset();
}

void Button::Reset() {
    m_etInState.Reset();
    m_isPressed = m_pressEventSent = m_longPressEventSent = m_debouncing = false;
}

void Button::SetDigitalReadFunc(const DigitalReadFunc_t& func) {
    m_digitalReadFunc = func;
}

void Button::SendEvent(const Event_e evt) {
    m_etSinceRepeat.Reset();
    if (config.handlerFunc) {
        config.handlerFunc(evt);
    }
    m_pressEventSent = m_isPressed;
}

bool Button::MustDelayBeforePress() const {
    return config.beforePress > 0;
}
bool Button::ShouldSendDelayedPress() const {
    return m_isPressed && !m_pressEventSent && MustDelayBeforePress() &&
           m_etInState.Ms() >= config.beforePress;
}

bool Button::ShouldRepeat() {
    return !HasPinStateChanged() && m_isPressed && config.canRepeat &&
           m_pressEventSent && m_etInState.Ms() >= config.beforeRepeat &&
           (m_etSinceRepeat.Ms() >= config.repeatRate);
}

bool Button::ShouldSendLongPress() const {
    return m_isPressed && !m_longPressEventSent && m_pressEventSent && 
           m_etInState.Ms() >= config.longPressTime;
}

bool Button::HasPinStateChanged() {
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

bool Button::IsDebouncing() {
    if (!m_debouncing) {
        return false;
    }
    m_debouncing = (m_etInState.Ms() < config.debounceTime);
    return m_debouncing;
}

Joystick::Joystick() {
    m_buttons.push_back(&up);
    m_buttons.push_back(&down);
    m_buttons.push_back(&left);
    m_buttons.push_back(&right);
    m_buttons.push_back(&press);
}

int Joystick::AreAnyButtonsPressed() {
    Update();
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

bool Joystick::WaitForButton(const Button& btn, const int ms) {
    ElapsedTime elapsed;
    while (elapsed.Ms() < ms || ms == -1) {
        Update();
        if (btn.IsPressed()) {
            return false;
        }
        ElapsedTime::Delay(1);
    }
    return true;
}

void Joystick::WaitForNoButtonsPressed() {
    while (AreAnyButtonsPressed() != -1) {
        ElapsedTime::Delay(1);
    }
}

void Joystick::Reset() {
    for (auto& btn : m_buttons) {
        btn->Reset();
    }
}

void Joystick::Update() {
    for (auto& btn : m_buttons) {
        btn->Update();
    }
}
