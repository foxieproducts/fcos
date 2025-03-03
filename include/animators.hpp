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
    std::vector<RgbColor> digitColors;     // Beginning colors for each digit
    std::vector<RgbColor> digitColorEnds;  // Ending colors for each digit
    std::vector<float> digitBrightness;    // Brightness factors for each digit (0.0-1.0)
    std::vector<float> digitBrightnessEnds; // Brightness factors for ending colors
    float colonBrightness{1.0f};           // Brightness factor for colon
    float colonBrightnessEnd{1.0f};        // Brightness factor for colon end
    uint8_t wheelPos{0};
    size_t freq{0};  // ms
    std::function<void(Animator& a)> func;
    String name;

    Animator();

    void Update();

    virtual void Start();

    virtual void SetColor(uint8_t colorWheelPos);

    virtual RgbColor GetColonColor();
    virtual RgbColor GetColonColorEnd();
    
    // New methods for brightness-adjusted colors
    RgbColor GetAdjustedDigitColor(size_t index);
    RgbColor GetAdjustedDigitColorEnd(size_t index);
    RgbColor GetAdjustedColonColor();
    RgbColor GetAdjustedColonColorEnd();
    
    // Methods to control brightness
    virtual void SetDigitBrightness(size_t index, float brightness);
    virtual void SetDigitBrightnessEnd(size_t index, float brightness);
    virtual void SetColonBrightness(float brightness);
    virtual void SetColonBrightnessEnd(float brightness);
    virtual void SetAllDigitsBrightness(float brightness);
    
    // Optional UI controls for brightness
    virtual void BrightnessUp();
    virtual void BrightnessDown();

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

struct RainbowRotateOpposite : public Animator {
    virtual void Start() override;
    virtual void Up() override {}
    virtual void Down() override {}
};

struct CandleFlicker : public Animator {
    enum FlickerFrequency {
        FLICKER_SLOW = 0,
        FLICKER_MEDIUM,
        FLICKER_FAST,
        FLICKER_TOTAL
    };
    
    enum FlameStyle {
        FLAME_NORMAL = 0,    // Regular candle flame
        FLAME_WINDY,         // Wind-affected flame with more variation
        FLAME_DYING,         // Dying flame with occasional strong flickers
        FLAME_TOTAL
    };
    
    struct DigitFlame {
        ElapsedTime flickerTimer;
        int flickerDuration{0};
        float flickerIntensity{0.0f};
        float previousIntensity{0.0f}; // Store previous intensity for smoother transitions
        int8_t colorVariation{0};
        int8_t previousColorVar{0};    // Store previous color variation
        float transitionProgress{0.0f}; // 0.0 to 1.0 for smooth transitions
    };
    
    FlickerFrequency flickerFreq{FLICKER_MEDIUM};
    FlameStyle flameStyle{FLAME_NORMAL};
    std::vector<DigitFlame> digitFlames;
    
    virtual void Start() override;
    
    virtual bool Left() override;   // Change flicker frequency
    virtual bool Right() override;  // Change flame style
    
    // Helper method to update flicker parameters based on current settings
    void UpdateFlickerParameters();
    
    // Helper method to update a single digit's flame parameters
    void UpdateDigitFlame(DigitFlame& flame);
};

struct RainbowRotate2 : public Animator {
    virtual void Start() override;
    virtual void Up() override {}
    virtual void Down() override {}
};

struct IndividualDigitColors : public Animator {
    size_t selected{4};
    uint8_t wheelColors[5] = {0};      // 5th is for the colon (beginning colors)
    uint8_t wheelColorsEnd[5] = {0};   // 5th is for the colon (ending colors)
    ElapsedTime timeShown;
    bool show{false};

    virtual void Start();

    RgbColor GetSelectedDigitColor(const uint8_t sel);
    RgbColor GetSelectedDigitColorEnd(const uint8_t sel);

    virtual RgbColor GetColonColor();
    virtual RgbColor GetColonColorEnd();

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

struct Starfield : public Animator {
    struct Star {
        float x{0};
        float y{0};
        float dx{0};  // x direction/velocity
        float dy{0};  // y direction/velocity
        float speed{0};
        RgbColor color{WHITE};
        
        Star() {
            Reset();
        }
        
        void Reset() {
            // Start at center of display
            x = DISPLAY_WIDTH / 2.0f;
            y = DISPLAY_HEIGHT / 2.0f;
            
            // Random direction vector (but not zero)
            do {
                dx = ((float)(rand() % 200) / 100.0f) - 1.0f;  // -1.0 to 1.0
                dy = ((float)(rand() % 200) / 100.0f) - 1.0f;  // -1.0 to 1.0
            } while (dx == 0 && dy == 0);
            
            // Normalize the direction vector
            float length = sqrt(dx*dx + dy*dy);
            dx /= length;
            dy /= length;
            
            // Higher initial speed for more streaking
            speed = (float)(rand() % 80) / 100.0f + 0.2f;  // 0.2 to 1.0
            
            // Random brightness for initial position
            float brightness = (float)(rand() % 70) / 100.0f + 0.3f;  // 0.3 to 1.0
            color = Pixels::ScaleBrightness(WHITE, brightness);
        }
        
        void Update() {
            // Move star outward from center
            x += dx * speed;
            y += dy * speed;
            
            // Increase speed more rapidly for longer streaks
            speed *= 1.05f;  // Increased from 1.02f
            
            // Increase brightness as star speeds up
            float brightness = std::min(1.0f, 0.3f + speed);
            color = Pixels::ScaleBrightness(WHITE, brightness);
        }
        
        // Check if star is out of bounds
        bool IsOutOfBounds() {
            return x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT;
        }
    };
    
    std::vector<Star> stars;
    
    Starfield();
    
    virtual void Start() override;
};

struct SnowfallAnimator : public Animator {
    struct Snowflake {
        float x{0};
        float y{0};
        float dx{0};  // x direction/velocity (for wind effect)
        float speed{0};
        float size{1.0f};  // Controls brightness
        RgbColor color{WHITE};
        
        Snowflake() {
            Reset();
        }
        
        void Reset() {
            // Start at random position at the top
            x = (float)(rand() % DISPLAY_WIDTH);
            y = 0;
            
            // Random horizontal drift
            dx = 0;
            
            // Random falling speed
            speed = (float)(rand() % 50) / 100.0f + 0.1f;  // 0.1 to 0.6
            
            // Random size/brightness
            size = (float)(rand() % 60) / 100.0f + 0.4f;  // 0.4 to 1.0
            color = Pixels::ScaleBrightness(WHITE, size);
        }
        
        void Update(float windStrength) {
            // Apply wind effect - make it proportional to the snowflake size
            // Smaller snowflakes are affected more by wind
            dx = windStrength * (1.5f - size); // Smaller flakes get pushed more
            
            // Move snowflake
            x += dx;
            y += speed;
            
            // Wrap around horizontally if blown off-screen
            if (x < 0) {
                x = DISPLAY_WIDTH - 1;
            } else if (x >= DISPLAY_WIDTH) {
                x = 0;
            }
        }
        
        // Check if snowflake has reached the bottom
        bool HasReachedBottom() {
            return y >= DISPLAY_HEIGHT;
        }
    };
    
    // Snow accumulation at the bottom
    struct SnowPile {
        int x{0};
        int height{0};
        ElapsedTime meltTimer;
        int meltDelay{0};  // ms
        
        SnowPile() {
            Reset();
        }
        
        void Reset() {
            x = rand() % DISPLAY_WIDTH;
            height = 1;
            meltDelay = 5000 + (rand() % 5000);  // 5-10 seconds
            meltTimer.Reset();
        }
        
        bool ShouldMelt() {
            return meltTimer.Ms() >= meltDelay;
        }
    };
    
    std::vector<Snowflake> snowflakes;
    std::vector<SnowPile> snowPiles;
    std::vector<RgbColor> displayBuffer;  // Buffer to store current display state
    
    ElapsedTime windChangeTimer;
    int windChangeDuration{0};
    float windStrength{0};
    uint8_t snowColorPos{0};  // Position on the color wheel for snow color
    float snowBrightness{0.5f};  // Brightness of snow (0.3-1.0)
    
    SnowfallAnimator();
    
    virtual void Start() override;
    virtual bool Left() override;   // Adjust snow brightness
    virtual bool Right() override;  // Change snow color
    
    // Helper method to update all snowflake colors based on current settings
    void UpdateSnowflakeColors();
    
    // Helper method to calculate snow color based on brightness and color position
    RgbColor CalculateSnowColor(float brightness);
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
    ANIM_RAINBOW_ROTATE_OPPOSITE,
    ANIM_CANDLE_FLICKER,
#if FCOS_CARDCLOCK2
    ANIM_HOLIDAY_LIGHTS,
    ANIM_STARFIELD,
    ANIM_SNOWFALL,
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
        case ANIM_RAINBOW_ROTATE_OPPOSITE:
            anim = std::make_shared<RainbowRotateOpposite>();
            break;
        case ANIM_CANDLE_FLICKER:
            anim = std::make_shared<CandleFlicker>();
            break;
#if FCOS_CARDCLOCK2
        case ANIM_HOLIDAY_LIGHTS:
            anim = std::make_shared<HolidayLights>();
            break;
        case ANIM_STARFIELD:
            anim = std::make_shared<Starfield>();
            break;
        case ANIM_SNOWFALL:
            anim = std::make_shared<SnowfallAnimator>();
            break;
#endif
    }

    anim->pixels = pixels;
    anim->settings = settings;
    anim->rtc = rtc;
    return anim;
}
