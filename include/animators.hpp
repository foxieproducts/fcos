#pragma once
#include <functional>

#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
enum AnimatorType_e {
    ANIM_NORMAL,
    ANIM_RAINBOW_FIXED,
    ANIM_RAINBOW_ROTATE1,
    ANIM_RAINBOW_ROTATE2,
    ANIM_INDIVIDUAL_DIGITS,
    ANIM_TOTAL,
};

struct Animator {
    std::shared_ptr<Pixels> pixels;
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Rtc> rtc;

    ElapsedTime sinceLastAnimation;
    std::vector<RgbColor> digitColors;
    uint8_t wheelPos{0};
    size_t freq{0};  // ms
    std::function<void(Animator& a)> func;

    Animator() { digitColors.resize(4, BLACK); }

    void Update() {
        if (freq > 0 && sinceLastAnimation.Ms() > freq) {
            sinceLastAnimation.Reset();
            func(*this);
        }
    }

    virtual void Start() {
        if (!(*settings).containsKey("COLR")) {
            (*settings)["COLR"] = wheelPos;
        } else {
            wheelPos = (*settings)["COLR"].as<uint32_t>();
        }

        if (!(*settings).containsKey("COLR_COLON")) {
            (*settings)["COLR_COLON"] = wheelPos;
        }
    }

    virtual void SetColor(uint8_t colorWheelPos) {
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

    virtual RgbColor GetColonColor() {
        return pixels->ColorWheel((*settings)["COLR_COLON"]);
    }

    virtual void Up() {
        if (!CanChangeColor()) {
            return;
        }
        SetColor(wheelPos - 3);
    }
    virtual void Down() {
        if (!CanChangeColor()) {
            return;
        }
        SetColor(wheelPos + 3);
    }
    virtual bool Left() { return false; }
    virtual bool Right() { return false; }

    virtual bool CanChangeColor() { return true; }
};

struct RainbowFixed : public Animator {
    virtual void Start() override {
        freq = 50;
        func = [&](Animator& a) {
            for (auto d = digitColors.rbegin(); d != digitColors.rend(); ++d) {
                *d = Pixels::ColorWheel(wheelPos);
                wheelPos += 64;
            }
        };
    }
};

struct RainbowRotate1 : public Animator {
    virtual void Start() override {
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
    virtual void Up() override {}
    virtual void Down() override {}
};

struct RainbowRotate2 : public Animator {
    virtual void Start() override {
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
    virtual void Up() override {}
    virtual void Down() override {}
};

struct IndividualDigitColors : public Animator {
    size_t selected{4};
    uint8_t wheelColors[5] = {0};  // 5th is for the colon
    ElapsedTime timeShown;
    bool show{false};

    virtual void Start() {
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

    RgbColor GetSelectedDigitColor(const uint8_t sel) {
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

    virtual RgbColor GetColonColor() {
        return GetSelectedDigitColor(2);  // colon
    }

    virtual void Up() override {
        wheelColors[selected] += 3;
        StoreSettings();
    }
    virtual void Down() override {
        wheelColors[selected] -= 3;
        StoreSettings();
    }
    virtual bool Left() override {
        timeShown.Reset();
        show = true;
        if (selected > 0) {
            selected--;
        }
        return true;
    }
    virtual bool Right() override {
        timeShown.Reset();
        show = true;
        if (selected < 4) {
            selected++;
        }
        return true;
    }
    virtual bool CanChangeColor() override { return false; }

  private:
    void StoreSettings() {
        (*settings)["ANIM_INDIVIDUAL_COL0"] = wheelColors[0];
        (*settings)["ANIM_INDIVIDUAL_COL1"] = wheelColors[1];
        (*settings)["ANIM_INDIVIDUAL_COL2"] = wheelColors[2];
        (*settings)["ANIM_INDIVIDUAL_COL3"] = wheelColors[3];
        (*settings)["ANIM_INDIVIDUAL_COLON"] = wheelColors[4];
    }
};

static inline std::shared_ptr<Animator> CreateAnimator(
    std::shared_ptr<Pixels> pixels,
    std::shared_ptr<Settings> settings,
    std::shared_ptr<Rtc> rtc,
    AnimatorType_e type) {
    std::shared_ptr<Animator> anim;
    switch (type) {
        default:
        case ANIM_NORMAL:
            anim = std::make_shared<Animator>();
            break;
        case ANIM_RAINBOW_FIXED:
            anim = std::make_shared<RainbowFixed>();
            break;
        case ANIM_RAINBOW_ROTATE1:
            anim = std::make_shared<RainbowRotate1>();
            break;
        case ANIM_RAINBOW_ROTATE2:
            anim = std::make_shared<RainbowRotate2>();
            break;
        case ANIM_INDIVIDUAL_DIGITS:
            anim = std::make_shared<IndividualDigitColors>();
            break;
    }

    anim->pixels = pixels;
    anim->settings = settings;
    anim->rtc = rtc;
    return anim;
}
