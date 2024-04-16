#pragma once
#include <functional>

#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>

struct Animator {
    std::shared_ptr<Pixels> pixels;
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Rtc> rtc;

    ElapsedTime sinceLastAnimation;
    std::vector<RgbColor> digitColors;
    uint8_t wheelPos{0};
    size_t freq{0};  // ms
    std::function<void(Animator& a)> func;
    String name;

    Animator();

    void Update();

    virtual void Start();

    virtual void SetColor(uint8_t colorWheelPos);

    virtual RgbColor GetColonColor();

    virtual void Up();
    virtual void Down();
    virtual bool Left();
    virtual bool Right();

    virtual bool CanChangeColor();
};

struct RainbowFixed : public Animator {
    virtual void Start() override;
};

struct RainbowRotate1 : public Animator {
    virtual void Start() override;
    virtual void Up() override {}
    virtual void Down() override {}
};

struct RainbowRotate2 : public Animator {
    virtual void Start() override;
    virtual void Up() override {}
    virtual void Down() override {}
};

struct IndividualDigitColors : public Animator {
    size_t selected{4};
    uint8_t wheelColors[5] = {0};  // 5th is for the colon
    ElapsedTime timeShown;
    bool show{false};

    virtual void Start();

    RgbColor GetSelectedDigitColor(const uint8_t sel);

    virtual RgbColor GetColonColor();

    virtual void Up() override;
    virtual void Down() override;
    virtual bool Left() override;
    virtual bool Right() override;
    virtual bool CanChangeColor() override;

  private:
    void StoreSettings();
};

struct RainbowFixedMatrix : public Animator {
    struct MatrixDot {
        int8_t x{-1}, y{DISPLAY_HEIGHT};
        RgbColor color{BLACK};
        ElapsedTime time;
        float period{0};  // ms
    };
    std::vector<MatrixDot> dots;
    bool rainbow{false};

    RainbowFixedMatrix(bool rainbow);

    virtual void Start() override;
};

#if FCOS_CARDCLOCK2
struct HolidayLights : public Animator {
    virtual void Start() override;
    virtual void Up() override {}
    virtual void Down() override {}
};
#endif

enum AnimatorType_e {
    ANIM_NORMAL,
    ANIM_RAINBOW_FIXED,
    ANIM_RAINBOW_ROTATE1,
    ANIM_RAINBOW_ROTATE2,
    ANIM_INDIVIDUAL_DIGITS,
    ANIM_RAINBOW_RAIN,
    ANIM_FIXED_RAIN,
#if FCOS_CARDCLOCK2
    ANIM_HOLIDAY_LIGHTS,
#endif
    ANIM_TOTAL,
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
        case ANIM_RAINBOW_RAIN:
            anim = std::make_shared<RainbowFixedMatrix>(true);
            break;
        case ANIM_FIXED_RAIN:
            anim = std::make_shared<RainbowFixedMatrix>(false);
            break;
#if FCOS_CARDCLOCK2
        case ANIM_HOLIDAY_LIGHTS:
            anim = std::make_shared<HolidayLights>();
            break;
#endif
    }

    anim->pixels = pixels;
    anim->settings = settings;
    anim->rtc = rtc;
    return anim;
}
