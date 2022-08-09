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
    // ANIM_RAINBOW_GENTLE?
    ANIM_TOTAL,
};

struct Animator {
    std::shared_ptr<Pixels> pixels;
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

    virtual void Start() {}

    virtual void SetColor(uint8_t colorWheelPos) {
        wheelPos = colorWheelPos;
        for (auto& d : digitColors) {
            d = Pixels::ColorWheel(wheelPos);
        }
        Start();

        if (func) {
            func(*this);
        }
        sinceLastAnimation.Reset();
    }

    virtual bool CanChangeColor() { return true; }
};

struct RainbowFixed : public Animator {
    virtual void Start() {
        for (auto d = digitColors.rbegin(); d != digitColors.rend(); ++d) {
            *d = Pixels::ColorWheel(wheelPos);
            wheelPos += 64;
        }
    }
};

struct RainbowRotate1 : public Animator {
    virtual void Start() override {
        freq = 50;
        func = [&](Animator& a) {
            auto tempPos = wheelPos++;
            for (auto& d : digitColors) {
                d = Pixels::ColorWheel(tempPos);
                tempPos += 64;
            }
        };
    }
    virtual bool CanChangeColor() override { return false; }
};

struct RainbowRotate2 : public Animator {
    virtual void Start() {
        freq = 50;
        func = [&](Animator& a) {
            auto tempPos = wheelPos++;
            for (auto& d : digitColors) {
                d = Pixels::ColorWheel(tempPos);
                tempPos += 32;
            }
        };
    }
    virtual bool CanChangeColor() override { return false; }
};

static inline std::shared_ptr<Animator> CreateAnimator(AnimatorType_e type) {
    switch (type) {
        default:
        case ANIM_NORMAL:
            return std::make_shared<Animator>();
        case ANIM_RAINBOW_FIXED:
            return std::make_shared<RainbowFixed>();
        case ANIM_RAINBOW_ROTATE1:
            return std::make_shared<RainbowRotate1>();
        case ANIM_RAINBOW_ROTATE2:
            return std::make_shared<RainbowRotate2>();
    }
}
