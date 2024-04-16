#include <animators.hpp>

Animator::Animator() {
    name = "NORMAL";
    digitColors.resize(4, BLACK);
}

void Animator::Update() {
    if (freq > 0 && sinceLastAnimation.Ms() > freq) {
        sinceLastAnimation.Reset();
        func(*this);
    }
}

void Animator::Start() {
    if (!(*settings).containsKey("COLR")) {
        (*settings)["COLR"] = wheelPos;
    } else {
        wheelPos = (*settings)["COLR"].as<uint32_t>();
    }

    if (!(*settings).containsKey("COLR_COLON")) {
        (*settings)["COLR_COLON"] = wheelPos;
    }
}

void Animator::SetColor(uint8_t colorWheelPos) {
    Start();
    wheelPos = colorWheelPos;
    (*settings)["COLR"] = wheelPos;
    (*settings)["COLR_COLON"] = wheelPos;
    for (auto& d : digitColors) {
        d = Pixels::ColorWheel(wheelPos);
    }

    if (func) {
        func(*this);
    }
    sinceLastAnimation.Reset();
}

RgbColor Animator::GetColonColor() {
    return pixels->ColorWheel((*settings)["COLR_COLON"]);
}

void Animator::Up() {
    if (!CanChangeColor()) {
        return;
    }
    SetColor(wheelPos - 2);
}
void Animator::Down() {
    if (!CanChangeColor()) {
        return;
    }
    SetColor(wheelPos + 2);
}
bool Animator::Left() {
    return false;
}
bool Animator::Right() {
    return false;
}

bool Animator::CanChangeColor() {
    return true;
}

void RainbowFixed::Start() {
    name = "Rainbow Fixed";
    freq = 50;
    func = [&](Animator& a) {
        for (auto d = digitColors.rbegin(); d != digitColors.rend(); ++d) {
            *d = Pixels::ColorWheel(wheelPos);
            wheelPos += 64;
        }
    };
}

void RainbowRotate1::Start() {
    name = "Rainbow 1";
    freq = 50;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (auto& d : digitColors) {
            d = Pixels::ColorWheel(tempPos);
            tempPos += 64;
        }
    };
}

void RainbowRotate2::Start() {
    name = "Rainbow 2";
    freq = 50;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (auto& d : digitColors) {
            d = Pixels::ColorWheel(tempPos);
            tempPos += 16;
        }
    };
}

void IndividualDigitColors::Start() {
    name = "Individual (use left/right)";
    if (!(*settings).containsKey("ANIM_INDIVIDUAL_COL0")) {
        StoreSettings();
    } else {
        wheelColors[0] = (*settings)["ANIM_INDIVIDUAL_COL0"].as<uint8_t>();
        wheelColors[1] = (*settings)["ANIM_INDIVIDUAL_COL1"].as<uint8_t>();
        wheelColors[2] = (*settings)["ANIM_INDIVIDUAL_COLON"].as<uint8_t>();
        wheelColors[3] = (*settings)["ANIM_INDIVIDUAL_COL2"].as<uint8_t>();
        wheelColors[4] = (*settings)["ANIM_INDIVIDUAL_COL3"].as<uint8_t>();
    }

    freq = 50;
    func = [&](Animator& a) {
        digitColors[0] = GetSelectedDigitColor(0);  // col0
        digitColors[1] = GetSelectedDigitColor(1);  // col1
                                                    // colon, wheelPos2
        digitColors[2] = GetSelectedDigitColor(3);  // col2
        digitColors[3] = GetSelectedDigitColor(4);  // col3
    };
}

RgbColor IndividualDigitColors::GetSelectedDigitColor(const uint8_t sel) {
    RgbColor color = Pixels::ColorWheel(wheelColors[sel]);
    if (show && sel == selected &&
        (timeShown.Ms() < 300 ||
         timeShown.Ms() > 600 && timeShown.Ms() < 900)) {
        color = WHITE;
    };

    if (timeShown.Ms() > 1000) {
        show = false;
    }

    return color;
}

RgbColor IndividualDigitColors::GetColonColor() {
    return GetSelectedDigitColor(2);  // colon
}

void IndividualDigitColors::Up() {
    wheelColors[selected] += 3;
    StoreSettings();
}
void IndividualDigitColors::Down() {
    wheelColors[selected] -= 3;
    StoreSettings();
}
bool IndividualDigitColors::Left() {
    timeShown.Reset();
    show = true;
    if (selected > 0) {
        selected--;
    }
    return true;
}
bool IndividualDigitColors::Right() {
    timeShown.Reset();
    show = true;
    if (selected < 4) {
        selected++;
    }
    return true;
}
bool IndividualDigitColors::CanChangeColor() {
    return false;
}

void IndividualDigitColors::StoreSettings() {
    (*settings)["ANIM_INDIVIDUAL_COL0"] = wheelColors[0];
    (*settings)["ANIM_INDIVIDUAL_COL1"] = wheelColors[1];
    (*settings)["ANIM_INDIVIDUAL_COLON"] = wheelColors[2];
    (*settings)["ANIM_INDIVIDUAL_COL2"] = wheelColors[3];
    (*settings)["ANIM_INDIVIDUAL_COL3"] = wheelColors[4];
}

RainbowFixedMatrix::RainbowFixedMatrix(bool rainbow)
    : Animator(), rainbow(rainbow) {
    name = rainbow ? "Rainbow Rain" : "Falling Rain";
}

void RainbowFixedMatrix::Start() {
#if FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    dots.resize(8);
#else
    dots.resize(3);
#endif

    freq = 10;
    func = [&](Animator& a) {
        (*settings)["COLR_COLON"] = wheelPos;
        for (auto& d : digitColors) {
            d = Pixels::ColorWheel(wheelPos);
            d.Lighten(40);
        }

        auto animateDot = [&](MatrixDot& dot) {
            if (dot.time.Ms() >= dot.period || dot.period == 0) {
                dot.time.Reset();
#if FCOS_CARDCLOCK || FCOS_CARDCLOCK2
                if (++dot.y == DISPLAY_HEIGHT || dot.period == 0) {
                    dot.x = rand() % DISPLAY_WIDTH;
                    dot.y = dot.period == 0 ? rand() % DISPLAY_HEIGHT : 0;
                    dot.period = 50 + (rand() % 225);
#else
                if (++dot.x == TOTAL_MATRIX_LEDS || dot.period == 0) {
                    dot.x = rand() % TOTAL_MATRIX_LEDS;
                    dot.y = 0;
                    dot.period = 20 + (rand() % 200);
#endif
                    static uint8_t rainbowColor = 0;
                    dot.color = Pixels::ScaleBrightness(
                        Pixels::ColorWheel(rainbow ? rainbowColor : wheelPos),
                        0.7f + ((float)(rand() % 30) / 100.0f));
                    rainbowColor = rand() % 255;
                }
            }
        };
        for (auto& dot : dots) {
            animateDot(dot);
            pixels->Set(dot.x, dot.y, dot.color);
        }
    };
}

#if FCOS_CARDCLOCK2
void HolidayLights::Start() {
    name = "Holiday";
    freq = 10;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (auto& d : digitColors) {
            d = Pixels::ColorWheel(tempPos + 128);
            // tempPos += 64;
        }

        struct PerimeterDot {
            int8_t x{-1}, y{DISPLAY_HEIGHT};
            int8_t xDir{0}, yDir{0};
            RgbColor color{BLACK};
            ElapsedTime time;
            int period{0};  // ms
            PerimeterDot(int8_t x,
                         int8_t y,
                         int8_t xDir,
                         int8_t yDir,
                         RgbColor color)
                : x(x), y(y), xDir(xDir), yDir(yDir), color(color) {}
        };

        static std::vector<PerimeterDot> dots;
        if (dots.empty()) {
            for (int i = 0; i < 20; i++)
                dots.push_back(PerimeterDot(rand() % 15, 0, 1, 0, BLACK));
        }

        auto animateDot = [&](PerimeterDot& dot) {
            if (dot.time.Ms() >= dot.period || dot.period == 0) {
                dot.time.Reset();

                if (dot.period == 0) {
                    dot.period = 25 + rand() % 100;
                    dot.color = Pixels::ColorWheel(rand() % 255);
                };

                dot.x += dot.xDir;
                dot.y += dot.yDir;

                if (dot.x == DISPLAY_WIDTH) {
                    dot.xDir = 0;
                    dot.yDir = 1;
                    dot.x = DISPLAY_WIDTH - 1;
                    dot.y = 1;
                    dot.period = 0;
                }
                if (dot.y == DISPLAY_HEIGHT) {
                    dot.xDir = -1;
                    dot.yDir = 0;
                    dot.x = DISPLAY_WIDTH - 2;
                    dot.y = DISPLAY_HEIGHT - 1;
                }
                if (dot.x == -1) {
                    dot.xDir = 0;
                    dot.yDir = -1;
                    dot.x = 0;
                    dot.y = DISPLAY_HEIGHT - 2;
                    dot.period = 0;
                }
                if (dot.y == -1) {
                    dot.xDir = 1;
                    dot.yDir = 0;
                    dot.x = 1;
                    dot.y = 0;
                }
            }
        };

        for (auto& dot : dots) {
            animateDot(dot);
            pixels->Set(dot.x, dot.y, dot.color);
        }
    };
}
#endif
