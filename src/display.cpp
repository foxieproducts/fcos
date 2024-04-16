#include <display.hpp>

DisplayManager::DisplayManager(std::shared_ptr<Pixels> pixels,
                               std::shared_ptr<Settings> settings,
                               std::shared_ptr<Rtc> rtc,
                               std::shared_ptr<Joystick> joy)
    : m_pixels(pixels), m_settings(settings), m_rtc(rtc), m_joy(joy) {
    ConfigureJoystick();
}

void DisplayManager::Add(std::shared_ptr<Display> display) {
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

void DisplayManager::Update() {
    if (m_sinceLastUpdate.Ms() >= 1000 / FRAMES_PER_SECOND || m_isFirstUpdate) {
        m_sinceLastUpdate.Reset();
        if (m_isTempDisplay) {
            // Update the "parent" display of the temp display
            // this is used by ConfigMenu to allow it to composite the
            // ConfigMenu's Display and the currently displayed option (e.g.
            // 24HR mode)
            m_displays[m_lastActiveDisplay]->Update();
        }

        auto& cur = m_displays[m_activeDisplay];
        cur->Update();
        if (cur->IsDone() ||
            (cur->ShouldTimeout() && GetTimeSinceButtonPress() >= TIMEOUT_MS &&
             m_activeDisplay != m_defaultDisplay)) {
            cur->Timeout();
            cur->m_done = false;
            if (m_isTempDisplay) {
                ActivateDisplay(m_lastActiveDisplay);
            } else {
                ActivateDisplay(m_defaultDisplay);
            }
        }

        m_pixels->Update();
        m_isFirstUpdate = false;
    }
}

std::shared_ptr<Display> DisplayManager::GetActive() {
    return m_displays[m_activeDisplay];
}

void DisplayManager::SetDefaultAndActivateDisplay(const size_t displayNum) {
    m_defaultDisplay = displayNum;
    ActivateDisplay(displayNum);
}

void DisplayManager::ActivateDisplay(const size_t displayNum) {
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

void DisplayManager::ActivateTemporaryDisplay(
    std::shared_ptr<Display> display) {
    m_lastActiveDisplay = m_activeDisplay;
    m_isTempDisplay = true;
    Add(display);
    ActivateDisplay(m_displays.size() - 1);
}

void DisplayManager::ConfigureJoystick() {
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

void DisplayManager::ResetTimeSinceButtonPress() {
    m_timeSinceButtonPress.Reset();
}
size_t DisplayManager::GetTimeSinceButtonPress() {
    return m_timeSinceButtonPress.Ms();
}
