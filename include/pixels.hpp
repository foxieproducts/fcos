#pragma once
#include <NeoPixelBus.h>  // for communication with LEDs
#include <vector>         // for std::vector (primarily for characters/*.inc)

#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <light_sensor.hpp>
#include <settings.hpp>

RgbColor BLACK(0, 0, 0);
RgbColor WHITE(255, 255, 255);
RgbColor GRAY(128, 128, 128);
RgbColor RED(255, 0, 0);
RgbColor GREEN(0, 255, 0);
RgbColor BLUE(0, 0, 255);
RgbColor YELLOW(255, 255, 0);
RgbColor CYAN(0, 255, 255);
RgbColor PURPLE(96, 0, 255);
RgbColor MAGENTA(255, 0, 255);
RgbColor DARK_RED(128, 0, 0);
RgbColor DARK_GREEN(0, 128, 0);
RgbColor DARK_BLUE(0, 0, 128);
RgbColor DARK_YELLOW(128, 128, 0);
RgbColor DARK_CYAN(0, 128, 128);
RgbColor DARK_MAGENTA(128, 0, 128);
RgbColor DARK_PURPLE(32, 0, 226);
RgbColor DARK_GRAY(32, 32, 32);
RgbColor EXTRA_DARK_GRAY(16, 16, 16);
RgbColor ORANGE(255, 128, 0);
RgbColor DARK_ORANGE(128, 64, 0);
RgbColor LIGHT_GRAY(224, 224, 224);
RgbColor LIGHT_YELLOW(224, 224, 0);

enum SpecialChars_e {
    CHAR_UP_ARROW = 100,
    CHAR_DOWN_ARROW = 101,
    CHAR_RIGHT_ARROW = 102,
    CHAR_LEFT_ARROW = 103,
};

enum ScrollDirection_e {
    SCROLL_UP = -1,
    SCROLL_DOWN = 1,
    SCROLL_RIGHT = 1,
    SCROLL_LEFT = -1,
};

enum PixelsConfig_e {

#if FCOS_FOXIECLOCK
    DISPLAY_WIDTH = 20 + 20 + 2 /* blinkers */ + 20 + 20,
    DISPLAY_HEIGHT = 1,
    OPTION_LEDS = 8 + 2,
    ROUND_LEDS = 0,
    SCROLL_DELAY_HORIZONTAL_MS = 8,
    SCROLL_DELAY_VERTICAL_MS = 8,
#elif FCOS_CARDCLOCK
    DISPLAY_WIDTH = 17,
    DISPLAY_HEIGHT = 5,
    OPTION_LEDS = 0,
    ROUND_LEDS = 24,
    SCROLL_DELAY_HORIZONTAL_MS = 10,
    SCROLL_DELAY_VERTICAL_MS = 20,
#endif

    TOTAL_LEDS_DISPLAY = DISPLAY_WIDTH * DISPLAY_HEIGHT,
    FIRST_HOUR_LED = TOTAL_LEDS_DISPLAY,
    FIRST_MINUTE_LED = FIRST_HOUR_LED + 12,
    TOTAL_ALL_LEDS = TOTAL_LEDS_DISPLAY + ROUND_LEDS + OPTION_LEDS,

    SCROLLING_TEXT_MS = 50,
    FRAMES_PER_SECOND = 30,
    LIGHT_SENSOR_UPDATE_MS = 33,

    MIN_DISPLAY_BRIGHTNESS_DEFAULT = 0,
    MAX_DISPLAY_BRIGHTNESS_DEFAULT = 9,
};

enum LEDOptionPositions_e {
    LED_OPT_ANIM = 82,  // the 8 LEDs at the right of the FC2 begin at 82
    LED_OPT_MINB = 83,
    LED_OPT_24HR = 84,
    LED_OPT_PXL = 85,
    LED_OPT_BT = 86,
    LED_OPT_WIFI = 87,
    LED_OPT_INFO = 88,
    LED_OPT_UPDT = 89,
    LED_JOY_DOWN = 90,
    LED_JOY_UP = 91,
    LED_UNUSED = 0xFFFF,
};

class Pixels {
  private:
#if FCOS_ESP32_C3
    NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> m_neoPixels;
#elif FCOS_ESP8266
    NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> m_neoPixels;
#endif
    std::shared_ptr<Settings> m_settings;
    LightSensor m_lightSensor;
    float m_currentBrightness{-1};
    float m_adjustedBrightness{-1};

    ElapsedTime m_sinceLastShow;
    ElapsedTime m_sinceLastLightSensorUpdate;

    bool m_isPXLmode{false};
    bool m_useDarkMode{false};

  public:
    Pixels(std::shared_ptr<Settings> settings)
        : m_neoPixels(TOTAL_ALL_LEDS, PIN_LEDS), m_settings(settings) {
#if FCOS_ESP8266
        // TODO: check whether this is needed
        pinMode(PIN_LEDS, OUTPUT);
#endif
        m_neoPixels.Begin();

        if (!(*settings).containsKey("MINB")) {
            (*settings)["MINB"] = String(MIN_DISPLAY_BRIGHTNESS_DEFAULT);
        }
        if (!(*settings).containsKey("MAXB")) {
            (*settings)["MAXB"] = String(MAX_DISPLAY_BRIGHTNESS_DEFAULT);
        }
        m_isPXLmode = ((*m_settings)["PXL"] == "1");

        SetLEDBrightnessMultiplierFromSensor();
    }

    bool Update(const bool force = false) {
        if (m_sinceLastLightSensorUpdate.Ms() >= LIGHT_SENSOR_UPDATE_MS ||
            force) {
            m_sinceLastLightSensorUpdate.Reset();
            SetLEDBrightnessMultiplierFromSensor();
        }

        if (m_sinceLastShow.Ms() >= 1000 / FRAMES_PER_SECOND || force) {
            m_sinceLastShow.Reset();
            Show();
            m_isPXLmode = ((*m_settings)["PXL"] == "1");
            do {
                delay(1);
            } while (!CanShow());
            return true;
        }
        return false;
    }

    void Show() {
        m_neoPixels.Show();
    }

    bool CanShow() {
        return m_neoPixels.CanShow();
    }

    void Clear(const RgbColor color = BLACK,
               const bool includeOptionLEDs = true,
               const bool includeRoundLEDs = false) {
        const size_t numToClear = TOTAL_LEDS_DISPLAY +
                                  (includeOptionLEDs ? OPTION_LEDS : 0) +
                                  (includeRoundLEDs ? ROUND_LEDS : 0);
        for (size_t i = 0; i < numToClear; ++i) {
            m_neoPixels.SetPixelColor(i, color);
        }
    }

    // This is my favorite function in the whole codebase. Here's why:
    // This one weird trick (lol), done repeatedly, gives us nice pretty
    // fading transitions everywhere with almost no coding required and doesn't
    // use enough CPU to be a problem.
    void Darken(const size_t numTimes = 1,
                const float amount = 0.77f,
                const size_t delayMs = 15) {
        const size_t numToClear = TOTAL_LEDS_DISPLAY + OPTION_LEDS + ROUND_LEDS;
        for (size_t t = 0; t < numTimes; ++t) {
            for (size_t i = 0; i < numToClear; ++i) {
                RgbColor color = m_neoPixels.GetPixelColor(i);
                if (color == BLACK) {
                    continue;
                }
                m_neoPixels.SetPixelColor(i, ScaleBrightness(color, amount));
            }

            if (numTimes > 1) {
                ElapsedTime::Delay(delayMs);
            }
        }
    }

    void Set(const int pos,
             const RgbColor color,
             const bool skipBrightnessScaling = false) {
        if (skipBrightnessScaling || color == BLACK) {
            m_neoPixels.SetPixelColor(pos, color);
        } else {
            float adjustedBrightness = m_adjustedBrightness;
            if (!m_isPXLmode && !m_useDarkMode) {
                // this code increases the brightness of the LEDs
                // for the 3-9 digits so that they shine brighter
                // through the acrylics in front of them
                float multiplier = 0.0004f;
                if (GetBrightness() >= 0.04f) {
                    multiplier += GetBrightness() * 0.1f;
                }
                if (pos < 14) {  // digit 1
                    adjustedBrightness += (14 - pos) * multiplier;
                } else if (pos >= 20 && pos < 34) {  // digit 2
                    adjustedBrightness += (34 - pos) * multiplier;
                } else if (pos >= 42 && pos < 56) {  // digit 3
                    adjustedBrightness += (56 - pos) * multiplier;
                } else if (pos >= 62 && pos < 76) {  // digit 4
                    adjustedBrightness += (76 - pos) * multiplier;
                }
                if (adjustedBrightness > 1.0f) {
                    m_neoPixels.SetPixelColor(pos, color);
                    return;
                }
            }
            RgbColor scaledColor = ScaleBrightness(color, adjustedBrightness);
            m_neoPixels.SetPixelColor(pos, scaledColor);
        }
    }
    void Set(const int x,
             const int y,
             const RgbColor color,
             const bool force = false) {
        Set(y * DISPLAY_WIDTH + x, color, force);
    }

    int DrawText(int x, String text, RgbColor color) {
        text.toUpperCase();
        int textWidth = 0;

        for (auto character : text) {
            const int charWidth = DrawChar(x, character, color);
            textWidth += charWidth;
            x += charWidth;
        }
        return textWidth;
    }

    int DrawChar(int x, char character, const RgbColor color) {
        std::vector<uint8_t> charData;
        // clang-format off
        // --------------- 
        // characters are implemented in characters.inc as a list of if/else statements
        // ---------------
#ifdef FCOS_CARDCLOCK
#include "characters/cc.inc"
        // show a ? for unknown characters
        if (charData.empty()) {
            charData = {
                1, 1, 0,
                0, 0, 1,
                0, 1, 0,
                0, 0, 0,
                0, 1, 0,
            };
        }

        const int charWidth = charData.size() / DISPLAY_HEIGHT;
        const uint8_t* data = &charData[0];
        for (int row = 0; row < DISPLAY_HEIGHT; ++row) {
            for (int column = x; column < x + charWidth; ++column) {
                if (column >= 0 && column < DISPLAY_WIDTH && *data) {
                    Set(column, row, color);
                }
                data++;
            }
        }

        return charWidth + 1;
#elif FCOS_FOXIECLOCK
        // hax to ignore the blinker LEDs in the middle
        if (x == 40 || x == 41) { x = 42; }
        if (x == 60 || x == 61) { x = 62; }

        if (m_isPXLmode) {
#include "characters/fc2-pxl.inc"
        } else {
#include "characters/fc2-edgelit.inc"
        }

        // show a ? for unknown characters
        if (charData.empty()) {
            charData = {
                1, 1, 1, 1, 
                1, 1, 1, 1, 
                1, 1, 1, 1, 
                1, 1, 1, 1, 
                1, 1, 1, 1,
            };
        }
        // ---------------
        // clang-format on

        const uint8_t* data = &charData[0];
        const size_t charWidth = charData.size();
        for (size_t pos = 0; pos < charWidth; ++pos) {
            if (*data) {
                Set(x + pos, color);
                // in edge-lit mode in the darkness, only use 1 LED per
                // digit
                if (!m_isPXLmode && m_currentBrightness == 0.0f &&
                    m_useDarkMode) {
                    break;
                }
            }
            data++;
        }

        return charWidth;
#endif
    }

    static RgbColor ColorWheel(uint8_t pos) {
        return HslColor(pos / 255.0f, 1.0f, 0.5);
    }

    float GetBrightness() {
        return m_currentBrightness;
    }

    static RgbColor ScaleBrightness(const RgbColor color,
                                    const float brightness) {
        return color.LinearBlend(BLACK, color, brightness);
    }

    void ToggleDarkMode() {
        m_useDarkMode = !m_useDarkMode;
    }

    void EnableDarkMode() {
        m_useDarkMode = true;
    }

    void DisableDarkMode() {
        m_useDarkMode = false;
    }

    bool IsDarkModeEnabled() {
        return m_useDarkMode;
    }

    void DrawColorWheelBetween(uint8_t wheelPos,
                               const size_t x1,
                               const size_t x2) {
        const size_t steps = x2 - x1;
        for (size_t i = 0; i < steps; ++i) {
            RgbColor color = ColorWheel(wheelPos);
            wheelPos -= 255 / steps;
            Set(x1 + i, color);
        }
    }

#if FCOS_CARDCLOCK
    // TODO: Move these functions into a CC-specific version of Pixels
    void ClearRoundLEDs(const RgbColor color = BLACK) {
        for (size_t i = FIRST_HOUR_LED; i < TOTAL_ALL_LEDS; ++i) {
            m_neoPixels.SetPixelColor(i, color);
        }
    }

    void DrawColorWheel(const uint8_t bottomPixelWheelPos) {
        uint8_t wheelPos = bottomPixelWheelPos - 128;
        for (size_t i = 0; i < 12; ++i) {
            RgbColor color = ColorWheel(wheelPos);
            wheelPos += 255 / 12;
            DrawInsideRingPixel(i == 0 ? 11 : i - 1, color);
            DrawOutsideRingPixel(i, color);
        }
    }

    // if pos == 0 then LED is at the 1:00 position
    void DrawInsideRingPixel(const int pos,
                             const RgbColor color,
                             const bool forceColor = false) {
        m_neoPixels.SetPixelColor(FIRST_HOUR_LED + pos, color, forceColor);
    }

    // if pos == 0 then the LED is in the 12:00 position
    void DrawOutsideRingPixel(const int pos,
                              const RgbColor color,
                              const bool forceColor = false) {
        m_neoPixels.SetPixelColor(FIRST_MINUTE_LED + pos, color, forceColor);
    }

    void DrawTextCentered(const String& text, const RgbColor color) {
        int x = 9 - (text.length() *
                     2);  // chars are usually 3 pixels wide with a 1 pixel
                          // space. this is not completely accurate for a
                          // few narrow characters, such as the colon
        DrawText(x, text, color);
    }

    void DrawTextScrolling(const String& text,
                           const RgbColor color,
                           const size_t delayMs = SCROLLING_TEXT_MS) {
        const auto length = DrawText(0, text, color);

        ElapsedTime waitToScroll;
        for (int i = DISPLAY_WIDTH; i > -length;) {
            Clear();
            DrawText(i, text, color);
            // pressing a button will speed up a long blocking scrolling
            // message
            waitToScroll.Reset();
            size_t adjustedDelay = delayMs;
            while (waitToScroll.Ms() < adjustedDelay) {
                Update(true);
                adjustedDelay = Button::AreAnyButtonsPressed() != -1
                                    ? delayMs / 3
                                    : delayMs;
                yield();
            }
            --i;
        }
    }

    void DrawMinuteLED(const int minute, const RgbColor color) {
        // represent 60 seconds using 12 LEDs, 60 / 12 = 5
        DrawOutsideRingPixel(minute / 5, color);
    }
    void DrawHourLED(const int hour, const RgbColor color) {
        DrawInsideRingPixel(hour - 1, color);
    }

    void DrawSecondLEDs(const int second,
                        const RgbColor color,
                        const bool forceColor = false) {
        DrawInsideRingPixel(second < 5 ? 11 : (second / 5) - 1, color,
                            forceColor);
        DrawOutsideRingPixel(second / 5, color, forceColor);
    }
#endif

    void ScrollHorizontal(const int numColumns,
                          const int direction,
                          const size_t delayMs = SCROLL_DELAY_HORIZONTAL_MS) {
        for (int i = 0; i < numColumns; ++i) {
            MoveHorizontal(direction);
            Show();  // don't wait for FPS update
            ElapsedTime::Delay(delayMs);
        }
    }

    void ScrollVertical(const int numRows,
                        const int direction,
                        const size_t delayMs = SCROLL_DELAY_VERTICAL_MS) {
        for (int i = 0; i < numRows; ++i) {
            MoveVertical(direction);
            Show();  // don't wait for FPS update
            ElapsedTime::Delay(delayMs);
        }
    }

  private:
    void Move(const int fromCol,
              const int fromRow,
              const int toCol,
              const int toRow) {
        RgbColor color =
            m_neoPixels.GetPixelColor(fromRow * DISPLAY_WIDTH + fromCol);

        if (toCol >= 0 && toCol < DISPLAY_WIDTH && toRow >= 0 &&
            toRow < DISPLAY_HEIGHT) {
            Set(toCol, toRow, color, true);  // force color
            Set(fromCol, fromRow, BLACK);
        }
    }

    void Move(const int from, const int to) {
        RgbColor color = m_neoPixels.GetPixelColor(from);
        Set(to, color);
        Set(from, BLACK);
    }

    void MoveHorizontal(const int num) {
        for (int row = 0; row < DISPLAY_HEIGHT; ++row) {
            if (num < 0) {
                for (int column = 1; column < DISPLAY_WIDTH; ++column) {
                    Move(column, row, column + num, row);
                }
            } else {
                for (int column = DISPLAY_WIDTH - 2; column >= 0; --column) {
                    Move(column, row, column + num, row);
                }
            }
        }
    }

    void MoveVertical(const int num) {
        if (num < 0) {
            for (int row = 1; row < DISPLAY_HEIGHT; ++row) {
                for (int column = 0; column < DISPLAY_WIDTH; ++column) {
                    Move(column, row, column, row + num);
                }
            }
        } else {
            for (int row = DISPLAY_HEIGHT - 2; row >= 0; --row) {
                for (int column = 0; column < DISPLAY_WIDTH; ++column) {
                    Move(column, row, column, row + num);
                }
            }
        }
    }
    void SetLEDBrightnessMultiplierFromSensor() {
        m_lightSensor.Update();
        m_currentBrightness = m_lightSensor.GetScaled();

        // TODO: Apply MAXB
        int configuredMinBrightness = (*m_settings)["MINB"].as<int>();
        if (m_useDarkMode) {
            configuredMinBrightness = 0;
        }
        // const int maxBrightness =
        // (*m_settings)["MAXB"].as<int>();

        const float pixelMinBrightness = 0.0045f;
        m_adjustedBrightness = pixelMinBrightness +
                               (0.04f * configuredMinBrightness) +
                               (m_currentBrightness * 0.9f);
        if (m_adjustedBrightness > 0.9f) {
            m_adjustedBrightness = 0.9f;
        }

        if (m_isPXLmode) {
            m_adjustedBrightness = pixelMinBrightness +
                                   (0.002f * configuredMinBrightness) +
                                   (m_adjustedBrightness * 0.1f);
        }
    }
};
