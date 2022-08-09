#pragma once
#include <display.hpp>
#include <vector>

class Numeric : public Display {
  protected:
    size_t m_index{0};
    std::vector<String> m_values;
    bool m_canChangeValue{true};

  public:
    Numeric(const String& name, std::vector<String> values) : m_values(values) {
        m_name = name;
    }

    Numeric(const String& name, const int min, const int max) {
        m_name = name;
        for (int i = min; i <= max; ++i) {
            m_values.push_back(String(i));
        }
    }

    virtual void Activate() override {
        if (m_values.empty()) {
            return;
        }

        m_index = 0;
        // Make sure that the current setting is displayed. It
        // may have been changed via a different method (ANIM, by clicking
        // the center button)
        for (size_t i = 0; i < m_values.size(); ++i) {
            if (m_values[i] == (*m_settings)[m_name].as<String>()) {
                m_index = i;
                break;
            }
        }
    }

    virtual void Update() override {
        if (m_values.empty()) {
            return;
        }

        m_pixels->DrawText(42, m_values[m_index], GREEN);
        m_pixels->DrawChar(
            0, CHAR_UP_ARROW,
            (m_index < (m_values.size() - 1) ? GREEN : EXTRA_DARK_GRAY));
        m_pixels->DrawChar(0, CHAR_DOWN_ARROW,
                           (m_index > 0 ? GREEN : EXTRA_DARK_GRAY));
    }

    virtual void Hide() override {
        m_pixels->DrawChar(0, CHAR_UP_ARROW, BLACK);
        m_pixels->DrawChar(0, CHAR_DOWN_ARROW, BLACK);
    }

    // left/right button presses will exit this display

    virtual void Up(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_values.empty() || !m_canChangeValue) {
                return;
            }

            if ((size_t)m_index < m_values.size() - 1) {
                m_index++;
                ChangeSettingToCurrentValue();
            }
        }
    }
    virtual void Down(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_values.empty() || !m_canChangeValue) {
                return;
            }

            if (m_index > 0) {
                m_index--;
                ChangeSettingToCurrentValue();
            }
        }
    }

    virtual String GetCurrentValue() {
        return m_values.size() ? m_values[m_index] : "";
    }

  protected:
    virtual void ChangeSettingToCurrentValue() {
        if (m_name) {
            (*m_settings)[m_name] = GetCurrentValue();
        }
    }

    void AllowChangingValues(const bool allowed) { m_canChangeValue = allowed; }
};