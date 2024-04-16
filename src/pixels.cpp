#include <pixels.hpp>

Pixels::Pixels(std::shared_ptr<Settings> settings)
    : m_neoPixels(TOTAL_ALL_LEDS, PIN_LEDS), m_settings(settings) {
#if FCOS_ESP8266
    // TODO: check whether this is needed
    pinMode(PIN_LEDS, OUTPUT);
#endif
    m_neoPixels.Begin();

    if (!(*settings).containsKey("MINB")) {
        (*settings)["MINB"] = String(MIN_DISPLAY_BRIGHTNESS_DEFAULT);
    }

#if FCOS_FOXIECLOCK
    m_isPXLmode = ((*m_settings)["PXL"] == "1");
#else
    m_isPXLmode = true;
#endif

    if (!(*settings).containsKey("LS_HW_MIN")) {
        (*settings)["LS_HW_MIN"] = m_lightSensor.GetHwMin();
    }
    if (!(*settings).containsKey("LS_HW_MAX")) {
        (*settings)["LS_HW_MAX"] = m_lightSensor.GetHwMax();
    }
    m_lightSensor.SetHwMin((*m_settings)["LS_HW_MIN"].as<int>());
    m_lightSensor.SetHwMax((*m_settings)["LS_HW_MAX"].as<int>());
    m_lightSensor.ResetToCurrentSensorValue();

    SetLEDBrightnessMultiplierFromSensor();
}

void Pixels::Update() {
    if (m_sinceLastLightSensorUpdate.Ms() >= LIGHT_SENSOR_UPDATE_MS) {
        m_sinceLastLightSensorUpdate.Reset();
        SetLEDBrightnessMultiplierFromSensor();
    }

#if FCOS_FOXIECLOCK
    m_isPXLmode = ((*m_settings)["PXL"] == "1");
#endif
    Show();
}

void Pixels::Show() {
    m_neoPixels.Show();
}

void Pixels::Clear(const RgbColor color,
                   const bool includeOptionLEDs,
                   const bool includeRoundLEDs) {
    const size_t numToClear = TOTAL_MATRIX_LEDS +
                              (includeOptionLEDs ? OPTION_LEDS : 0) +
                              (includeRoundLEDs ? ROUND_LEDS : 0);
    for (size_t i = 0; i < numToClear; ++i) {
        m_neoPixels.SetPixelColor(i, color);
    }
}

void Pixels::Darken(const size_t numTimes,
                    const float amount,
                    const size_t delayMs) {
    const size_t numToClear = TOTAL_MATRIX_LEDS + OPTION_LEDS + ROUND_LEDS;
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
            Show();
        }
    }
}

void Pixels::Set(const int pos,
                 const RgbColor color,
                 const bool skipBrightnessScaling) {
    if (skipBrightnessScaling || color == BLACK) {
        m_neoPixels.SetPixelColor(pos, color);
    } else {
        float adjustedBrightness = m_adjustedBrightness;
        if (!m_isPXLmode && (!m_useDarkMode || GetBrightness() >= 0.04f)) {
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

            if (adjustedBrightness > 0.9f) {
                adjustedBrightness = 0.9f;
            }
        }
        RgbColor scaledColor = ScaleBrightness(color, adjustedBrightness);
        m_neoPixels.SetPixelColor(pos, scaledColor);
    }
}
void Pixels::Set(const int x,
                 const int y,
                 const RgbColor color,
                 const bool force) {
    if (x >= 0 && x <= DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        Set(y * DISPLAY_WIDTH + x, color, force);
    }
}

int Pixels::DrawText(int x, String text, const RgbColor color) {
    return DrawText(x, 0, text, color);
}

int Pixels::DrawText(int x, int y, String text, const RgbColor color) {
    text.toUpperCase();
    int textWidth = 0;

    for (auto character : text) {
        const int charWidth = DrawChar(x, y, character, color);
        textWidth += charWidth;
        x += charWidth;
    }
    return textWidth;
}

int Pixels::DrawChar(int x, char character, const RgbColor color) {
    return DrawChar(x, 0, character, color);
}

int Pixels::DrawChar(int x, int y, char character, const RgbColor color) {
    std::vector<uint8_t> charData;
    // clang-format off
        // --------------- 
        // characters are implemented in characters.inc as a list of if/else statements
        // ---------------
#if FCOS_CARDCLOCK || FCOS_CARDCLOCK2
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

        const int charWidth = charData.size() / CHAR_HEIGHT;
        const uint8_t* data = &charData[0];
        for (int row = 0; row < CHAR_HEIGHT; ++row) {
            for (int column = x; column < x + charWidth; ++column) {
                if (column >= 0 && column < DISPLAY_WIDTH && *data) {
                    Set(column, y + row, color);
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
            if (!m_isPXLmode && m_currentBrightness == 0.0f && m_useDarkMode) {
                break;
            }
        }
        data++;
    }

    return charWidth;
#endif
}

RgbColor Pixels::ColorWheel(uint8_t pos) {
    return HslColor(pos / 255.0f, 1.0f, 0.5);
}

float Pixels::GetBrightness() {
    return m_currentBrightness;
}

RgbColor Pixels::ScaleBrightness(const RgbColor color, const float brightness) {
    return color.LinearBlend(BLACK, color, brightness);
}

void Pixels::ToggleDarkMode() {
    m_useDarkMode = !m_useDarkMode;
}

void Pixels::EnableDarkMode() {
    m_useDarkMode = true;
}

void Pixels::DisableDarkMode() {
    m_useDarkMode = false;
}

bool Pixels::IsDarkModeEnabled() {
    return m_useDarkMode;
}

void Pixels::DrawColorWheelBetween(uint8_t wheelPos,
                                   const size_t x1,
                                   const size_t x2) {
    const size_t steps = x2 - x1;
    for (size_t i = 0; i < steps; ++i) {
        RgbColor color = ColorWheel(wheelPos);
        wheelPos -= 255 / steps;
        Set(x1 + i, color);
    }
}

#if FCOS_CARDCLOCK || FCOS_CARDCLOCK2
// TODO: Move these functions into a CC-specific version of Pixels

// if pos == 0 then LED is at the 1:00 position
void Pixels::DrawRingLED(const int ringNum,
                         const int pos,
                         const RgbColor color,
                         const bool forceColor) {
    Set(FIRST_RING_LED + (ringNum * RING_SIZE) + pos, color, forceColor);
}

void Pixels::ClearRoundLEDs(const RgbColor color) {
    for (size_t i = FIRST_RING_LED; i < TOTAL_ALL_LEDS; ++i) {
        Set(i, color, true);
    }
}

void Pixels::DrawColorWheel(const uint8_t bottomPixelWheelPos) {
    uint8_t wheelPos = bottomPixelWheelPos - 128;
    for (size_t i = 0; i < 12; ++i) {
        RgbColor color = ColorWheel(wheelPos);
        wheelPos += 255 / 12;
        DrawRingLED(0, i, color);
        DrawRingLED(1, i, color);
#if FCOS_CARDCLOCK2
        DrawRingLED(2, i, color);
#endif
    }
}

void Pixels::DrawTextScrolling(const String& text,
                               const RgbColor color,
                               const size_t delayMs) {
    const auto length = DrawText(0, text, color);

    ElapsedTime waitToScroll;
    for (int i = DISPLAY_WIDTH; i > -length;) {
        Clear();
        int yPos = 0;
#if FCOS_CARDCLOCK2
        yPos = 3;
#endif

        DrawText(i, yPos, text, color);
        // pressing a button will speed up a long blocking scrolling
        // message
        waitToScroll.Reset();
        size_t adjustedDelay = delayMs;
        while (waitToScroll.Ms() < adjustedDelay) {
            Update();
            // adjustedDelay = Button::AreAnyButtonsPressed() != -1
            //                     ? delayMs / 3
            //                     : delayMs;
            yield();
        }
        --i;
    }
}

void Pixels::DrawHourLED(const int hour, const RgbColor color) {
    DrawRingLED(0, hour - 1, color);
}

void Pixels::DrawMinuteLED(const int minute, const RgbColor color) {
    // represent 60 seconds using 12 LEDs, 60 / 12 = 5
    // 0 seconds is at the 12th LED, so subtract 1
    int minuteLED = minute / 5;
    if (minuteLED == 0) {
        minuteLED = 11;
    } else {
        minuteLED -= 1;
    }
    DrawRingLED(0, minuteLED, BLACK);
    DrawRingLED(1, minuteLED, color);
#if FCOS_CARDCLOCK2
    DrawRingLED(2, minuteLED, color);
#endif
}

void Pixels::DrawSecondLEDs(const int second,
                            const RgbColor color,
                            const int brightestLED) {
    int secondLED = second / 5;
    if (secondLED == 0) {
        secondLED = 11;
    } else {
        secondLED -= 1;
    }
#if FCOS_CARDCLOCK
    if (brightestLED == 0) {
        DrawRingLED(1, secondLED, color);
        DrawRingLED(0, secondLED, ScaleBrightness(color, 0.25f));
    } else {  // 1 and higher
        DrawRingLED(0, secondLED, ScaleBrightness(color, 0.25f));
        DrawRingLED(1, secondLED, color);
    }
#elif FCOS_CARDCLOCK2
    if (brightestLED == 0) {
        DrawRingLED(0, secondLED, color);
        DrawRingLED(1, secondLED, ScaleBrightness(color, 0.4f));
        DrawRingLED(2, secondLED, ScaleBrightness(color, 0.7f));
    } else if (brightestLED == 1) {
        DrawRingLED(0, secondLED, ScaleBrightness(color, 0.7f));
        DrawRingLED(1, secondLED, color);
        DrawRingLED(2, secondLED, ScaleBrightness(color, 0.4f));
    } else {  // 2 and higher
        DrawRingLED(0, secondLED, ScaleBrightness(color, 0.4f));
        DrawRingLED(1, secondLED, ScaleBrightness(color, 0.7f));
        DrawRingLED(2, secondLED, color);
    }
#endif
}
#endif

void Pixels::Move(const int fromCol,
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

void Pixels::Move(const int from, const int to) {
    RgbColor color = m_neoPixels.GetPixelColor(from);
    Set(to, color);
    Set(from, BLACK);
}

void Pixels::MoveHorizontal(const int num) {
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

void Pixels::MoveVertical(const int num) {
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
void Pixels::SetLEDBrightnessMultiplierFromSensor() {
    // TEMPORARY:
    m_lightSensor.SetHwMin((*m_settings)["LS_HW_MIN"].as<int>());
    m_lightSensor.SetHwMax((*m_settings)["LS_HW_MAX"].as<int>());

    m_lightSensor.Update();
    m_currentBrightness = m_lightSensor.GetScaled();

    int configuredMinBrightness = (*m_settings)["MINB"].as<int>();
    float pixelMinBrightness = 0.008f;
    if (m_useDarkMode) {
        pixelMinBrightness = 0.0045f;
        configuredMinBrightness = 0;
    }

    if (m_isPXLmode) {
        m_adjustedBrightness = pixelMinBrightness +
                               (0.004f * configuredMinBrightness) +
                               (m_currentBrightness * 0.08f);
    } else {
        m_adjustedBrightness = pixelMinBrightness +
                               (0.04f * configuredMinBrightness) +
                               (m_currentBrightness * 0.9f);
        if (m_adjustedBrightness > 0.9f) {
            m_adjustedBrightness = 0.9f;
        }
    }
}
