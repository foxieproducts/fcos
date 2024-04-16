#pragma once
#include <button.hpp>
#include <display.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>

class Breakout : public Display {
  private:
    std::shared_ptr<Joystick> m_joy;
    bool m_running{true};

    struct Ball {
        float x, y;
        float dx, dy;
        uint8_t wheelColor;
    };
    Ball m_ball{0, 0, 0.11f, 0.11f, 0};

  public:
    Breakout(std::shared_ptr<Pixels> pixels,
             std::shared_ptr<Settings> settings,
             std::shared_ptr<Rtc> rtc)
        : Display() {
        m_pixels = pixels;
        m_settings = settings;
        m_rtc = rtc;
        m_joy = std::make_shared<Joystick>();
    }

    void Start() {
        while (m_running) {
            Update();
            m_rtc->Update();
            m_joy->Update();
            m_pixels->Update();
            yield();  // allow the ESP platform tasks to run
        }
    }

    virtual void Update() override {
        if (m_joy->left.IsPressed()) {
            m_running = false;
            Serial.println("Exiting...\n");
            return;
        }
        static int darken = 0;
        if (darken++ == 10) {
            darken = 0;
            m_pixels->Darken(1, 0.99f);
        }

        m_pixels->Set(m_ball.x, m_ball.y,
                      Pixels::ColorWheel(m_ball.wheelColor++));
        m_ball.x += m_ball.dx;
        m_ball.y += m_ball.dy;
        if (m_ball.x < 0 || m_ball.x > DISPLAY_WIDTH - 1) {
            // bounce with a bit of spin
            if (m_ball.dx < 0) {
                m_ball.dx = .1f + ((rand() % 10) + 5) / 100.0f;
                m_ball.x = 0;
            } else {
                m_ball.dx = -.1f + -(((rand() % 10) + 5) / 100.0f);
                m_ball.x = DISPLAY_WIDTH - 1;
            }
        }
        if (m_ball.y < 0 || m_ball.y > DISPLAY_HEIGHT - 1) {
            // bounce with a bit of spin
            if (m_ball.dy < 0) {
                m_ball.dy = .1f + ((rand() % 10) + 5) / 100.0f;
                m_ball.y = 0;
            } else {
                m_ball.dy = -.1f + -(((rand() % 10) + 5) / 100.0f);
                m_ball.y = DISPLAY_HEIGHT - 1;
            }
        }
    }
};
