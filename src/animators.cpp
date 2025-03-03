#include <animators.hpp>
#include <cmath>  // For atan2, M_PI, sqrt

Animator::Animator() {
    name = "NORMAL";
    digitColors.resize(4, BLACK);
    digitColorEnds.resize(4, BLACK);
    digitBrightness.resize(4, 1.0f);     // Full brightness by default
    digitBrightnessEnds.resize(4, 1.0f); // Full brightness by default
}

void Animator::Update() {
    if (freq > 0 && sinceLastAnimation.Ms() > freq) {
        sinceLastAnimation.Reset();
        if (func) {
            func(*this);
        }
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
    
    // Load brightness settings if they exist
    if (!(*settings).containsKey("BRIGHTNESS")) {
        (*settings)["BRIGHTNESS"] = 100; // Default to 100% brightness
    } else {
        float brightness = (*settings)["BRIGHTNESS"].as<int>() / 100.0f;
        SetAllDigitsBrightness(brightness);
    }
}

void Animator::SetColor(uint8_t colorWheelPos) {
    Start();
    wheelPos = colorWheelPos;
    (*settings)["COLR"] = wheelPos;
    (*settings)["COLR_COLON"] = wheelPos;
    
    // Set both beginning and ending colors to the same value initially
    RgbColor color = Pixels::ColorWheel(wheelPos);
    for (size_t i = 0; i < digitColors.size(); i++) {
        digitColors[i] = color;
        digitColorEnds[i] = color;
    }

    if (func) {
        func(*this);
    }
    sinceLastAnimation.Reset();
}

RgbColor Animator::GetColonColor() {
    return pixels->ColorWheel((*settings)["COLR_COLON"]);
}

RgbColor Animator::GetColonColorEnd() {
    // By default, return the same color as the beginning
    return GetColonColor();
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
        uint8_t tempPos = wheelPos;
        for (size_t i = 0; i < digitColors.size(); i++) {
            digitColors[i] = Pixels::ColorWheel(tempPos);
            // For end colors, use a slightly offset color wheel position
            digitColorEnds[i] = Pixels::ColorWheel(tempPos + 16);
            tempPos += 64;
        }
    };
}

void RainbowRotate1::Start() {
    name = "Rainbow 1";
    freq = 50;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (size_t i = 0; i < digitColors.size(); i++) {
            digitColors[i] = Pixels::ColorWheel(tempPos);
            // For end colors, use a slightly offset color wheel position
            digitColorEnds[i] = Pixels::ColorWheel(tempPos + 16);
            tempPos += 64;
        }
    };
}

void RainbowRotateOpposite::Start() {
    name = "Rainbow Opposite";
    freq = 50;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (size_t i = 0; i < digitColors.size(); i++) {
            digitColors[i] = Pixels::ColorWheel(tempPos);
            // For end colors, use the opposite side of the color wheel (128 is half of 256)
            digitColorEnds[i] = Pixels::ColorWheel(tempPos + 128);
            tempPos += 64;
        }
    };
}

void CandleFlicker::Start() {
    name = "Candle";
    
    // Load settings if they exist
    if (!(*settings).containsKey("CANDLE_FLICKER_FREQ")) {
        (*settings)["CANDLE_FLICKER_FREQ"] = static_cast<int>(flickerFreq);
    } else {
        flickerFreq = static_cast<FlickerFrequency>((*settings)["CANDLE_FLICKER_FREQ"].as<int>());
    }
    
    if (!(*settings).containsKey("CANDLE_FLAME_STYLE")) {
        (*settings)["CANDLE_FLAME_STYLE"] = static_cast<int>(flameStyle);
    } else {
        flameStyle = static_cast<FlameStyle>((*settings)["CANDLE_FLAME_STYLE"].as<int>());
    }
    
    // Initialize per-digit flame parameters
    digitFlames.resize(4); // One for each digit
    
    // Set up the frequency based on flicker speed
    UpdateFlickerParameters();
    
    // Initialize each digit with slightly different parameters for natural look
    for (size_t i = 0; i < digitFlames.size(); i++) {
        // Set different initial durations to stagger the updates
        digitFlames[i].flickerDuration = 200 + (i * 100); // Stagger by 100ms per digit
        
        // Initialize with different parameters for each digit
        switch (flameStyle) {
            case FLAME_NORMAL:
                digitFlames[i].flickerIntensity = 0.2f + (static_cast<float>(rand() % 80) / 100.0f);
                digitFlames[i].colorVariation = -12 + (rand() % 25);
                break;
                
            case FLAME_WINDY:
                digitFlames[i].flickerIntensity = 0.1f + (static_cast<float>(rand() % 90) / 100.0f);
                digitFlames[i].colorVariation = -15 + (rand() % 31);
                break;
                
            case FLAME_DYING:
                if (rand() % 4 == 0) {
                    digitFlames[i].flickerIntensity = 0.05f + (static_cast<float>(rand() % 20) / 100.0f);
                } else {
                    digitFlames[i].flickerIntensity = 0.2f + (static_cast<float>(rand() % 80) / 100.0f);
                }
                digitFlames[i].colorVariation = -15 + (rand() % 31);
                break;
        }
        
        // Initialize previous values to match current values for smooth start
        digitFlames[i].previousIntensity = digitFlames[i].flickerIntensity;
        digitFlames[i].previousColorVar = digitFlames[i].colorVariation;
        
        // Reset the timer
        digitFlames[i].flickerTimer.Reset();
    }
    
    freq = 3; // Update very frequently for smoother transitions
    
    func = [&](Animator& a) {
        // Update each digit independently
        for (size_t i = 0; i < digitFlames.size(); i++) {
            auto& flame = digitFlames[i];
            
            // Check if it's time to generate new flicker parameters
            if (flame.flickerTimer.Ms() >= flame.flickerDuration) {
                // Generate new parameters
                UpdateDigitFlame(flame);
                
                // Reset transition progress
                flame.transitionProgress = 0.0f;
            }
            
            // Update transition progress (0.0 to 1.0) with smoother easing
            // Use a sine-based easing function for more natural transitions
            float rawProgress = static_cast<float>(flame.flickerTimer.Ms()) / 
                               static_cast<float>(flame.flickerDuration);
            
            // Apply easing function for smoother transitions
            // This creates a more natural, organic movement
            flame.transitionProgress = sin(rawProgress * M_PI / 2.0f);
            
            // Base color from the color wheel
            RgbColor baseColor = Pixels::ColorWheel(wheelPos);
            
            // Calculate current color variation by interpolating between previous and current
            int8_t currentColorVar = flame.previousColorVar + 
                (flame.colorVariation - flame.previousColorVar) * flame.transitionProgress;
            
            // Flicker color with interpolated variation
            RgbColor flickerColor = Pixels::ColorWheel(wheelPos + currentColorVar);
            
            // Set the beginning and ending colors
            digitColors[i] = baseColor;
            digitColorEnds[i] = flickerColor;
            
            // Calculate current intensity by interpolating between previous and current
            float currentIntensity = flame.previousIntensity + 
                (flame.flickerIntensity - flame.previousIntensity) * flame.transitionProgress;
            
            // Apply intensity using the brightness system
            SetDigitBrightnessEnd(i, currentIntensity);
        }
        
        // Update the colon color to match (use average of digit 1 and 2 for natural look)
        auto& flame1 = digitFlames[1];
        auto& flame2 = digitFlames[2];
        
        // Calculate interpolated color variations
        int8_t flame1ColorVar = flame1.previousColorVar + 
            (flame1.colorVariation - flame1.previousColorVar) * flame1.transitionProgress;
        int8_t flame2ColorVar = flame2.previousColorVar + 
            (flame2.colorVariation - flame2.previousColorVar) * flame2.transitionProgress;
        
        int8_t colonColorVar = (flame1ColorVar + flame2ColorVar) / 2;
        
        // Calculate interpolated intensities
        float flame1Intensity = flame1.previousIntensity + 
            (flame1.flickerIntensity - flame1.previousIntensity) * flame1.transitionProgress;
        float flame2Intensity = flame2.previousIntensity + 
            (flame2.flickerIntensity - flame2.previousIntensity) * flame2.transitionProgress;
        
        float colonIntensity = (flame1Intensity + flame2Intensity) / 2.0f;
        
        (*settings)["COLR_COLON"] = wheelPos + colonColorVar;
        
        // Set colon brightness
        SetColonBrightnessEnd(colonIntensity);
    };
}

void CandleFlicker::UpdateDigitFlame(DigitFlame& flame) {
    // Store previous values before updating
    flame.previousIntensity = flame.flickerIntensity;
    flame.previousColorVar = flame.colorVariation;
    
    flame.flickerTimer.Reset();
    
    // Generate new flicker parameters based on flame style
    switch (flameStyle) {
        case FLAME_NORMAL:
            // Normal flame has more dramatic variations now
            flame.flickerIntensity = 0.2f + (static_cast<float>(rand() % 80) / 100.0f); // 0.2 to 1.0
            flame.colorVariation = -12 + (rand() % 25); // -12 to +12
            flame.flickerDuration = 200 + (rand() % 300); // 200-500ms for quicker transitions
            break;
            
        case FLAME_WINDY:
            // Windy flame has extreme variations
            flame.flickerIntensity = 0.1f + (static_cast<float>(rand() % 90) / 100.0f); // 0.1 to 1.0
            flame.colorVariation = -15 + (rand() % 31); // -15 to +15
            flame.flickerDuration = 150 + (rand() % 250); // 150-400ms
            break;
            
        case FLAME_DYING:
            // Dying flame frequently dims significantly
            if (rand() % 4 == 0) { // 25% chance of a strong flicker (increased from 12.5%)
                flame.flickerIntensity = 0.05f + (static_cast<float>(rand() % 20) / 100.0f); // 0.05 to 0.25
                flame.flickerDuration = 100 + (rand() % 150); // 100-250ms
            } else {
                flame.flickerIntensity = 0.2f + (static_cast<float>(rand() % 80) / 100.0f); // 0.2 to 1.0
                flame.flickerDuration = 200 + (rand() % 300); // 200-500ms
            }
            flame.colorVariation = -15 + (rand() % 31); // -15 to +15
            break;
    }
}

void CandleFlicker::UpdateFlickerParameters() {
    // Set the base frequency based on flicker speed
    switch (flickerFreq) {
        case FLICKER_SLOW:
            freq = 5; // Slower updates but still smooth
            break;
        case FLICKER_MEDIUM:
            freq = 3; // Medium updates
            break;
        case FLICKER_FAST:
            freq = 2; // Faster updates
            break;
    }
    
    // Reset all flame timers to force an immediate update
    for (auto& flame : digitFlames) {
        flame.flickerTimer.Reset();
        flame.flickerDuration = 0;
    }
}

bool CandleFlicker::Left() {
    // Cycle through flicker frequencies
    flickerFreq = static_cast<FlickerFrequency>((static_cast<int>(flickerFreq) + 1) % FLICKER_TOTAL);
    (*settings)["CANDLE_FLICKER_FREQ"] = static_cast<int>(flickerFreq);
    
    UpdateFlickerParameters();
    return true;
}

bool CandleFlicker::Right() {
    // Cycle through flame styles
    flameStyle = static_cast<FlameStyle>((static_cast<int>(flameStyle) + 1) % FLAME_TOTAL);
    (*settings)["CANDLE_FLAME_STYLE"] = static_cast<int>(flameStyle);
    
    // Reset all flame timers to force an immediate update with the new style
    for (auto& flame : digitFlames) {
        flame.flickerTimer.Reset();
        flame.flickerDuration = 0;
    }
    return true;
}

void RainbowRotate2::Start() {
    name = "Rainbow 2";
    freq = 50;
    func = [&](Animator& a) {
        auto tempPos = wheelPos++;
        (*settings)["COLR_COLON"] = tempPos;
        for (size_t i = 0; i < digitColors.size(); i++) {
            digitColors[i] = Pixels::ColorWheel(tempPos);
            // For end colors, use a slightly offset color wheel position
            digitColorEnds[i] = Pixels::ColorWheel(tempPos + 8);
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
        
        // Load end colors if they exist, otherwise initialize with offset values
        if ((*settings).containsKey("ANIM_INDIVIDUAL_COL0_END")) {
            wheelColorsEnd[0] = (*settings)["ANIM_INDIVIDUAL_COL0_END"].as<uint8_t>();
            wheelColorsEnd[1] = (*settings)["ANIM_INDIVIDUAL_COL1_END"].as<uint8_t>();
            wheelColorsEnd[2] = (*settings)["ANIM_INDIVIDUAL_COLON_END"].as<uint8_t>();
            wheelColorsEnd[3] = (*settings)["ANIM_INDIVIDUAL_COL2_END"].as<uint8_t>();
            wheelColorsEnd[4] = (*settings)["ANIM_INDIVIDUAL_COL3_END"].as<uint8_t>();
        } else {
            // Initialize end colors with a slight offset from beginning colors
            for (int i = 0; i < 5; i++) {
                wheelColorsEnd[i] = wheelColors[i] + 16;
            }
            StoreSettings();
        }
    }

    freq = 50;
    func = [&](Animator& a) {
        digitColors[0] = GetSelectedDigitColor(0);  // col0
        digitColors[1] = GetSelectedDigitColor(1);  // col1
                                                    // colon, wheelPos2
        digitColors[2] = GetSelectedDigitColor(3);  // col2
        digitColors[3] = GetSelectedDigitColor(4);  // col3
        
        digitColorEnds[0] = GetSelectedDigitColorEnd(0);  // col0 end
        digitColorEnds[1] = GetSelectedDigitColorEnd(1);  // col1 end
                                                          // colon end
        digitColorEnds[2] = GetSelectedDigitColorEnd(3);  // col2 end
        digitColorEnds[3] = GetSelectedDigitColorEnd(4);  // col3 end
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

RgbColor IndividualDigitColors::GetSelectedDigitColorEnd(const uint8_t sel) {
    RgbColor color = Pixels::ColorWheel(wheelColorsEnd[sel]);
    if (show && sel == selected &&
        (timeShown.Ms() < 300 ||
         timeShown.Ms() > 600 && timeShown.Ms() < 900)) {
        color = WHITE;
    };

    return color;
}

RgbColor IndividualDigitColors::GetColonColor() {
    RgbColor color = Pixels::ColorWheel(wheelColors[2]);
    if (show && selected == 2 &&
        (timeShown.Ms() < 300 ||
         timeShown.Ms() > 600 && timeShown.Ms() < 900)) {
        color = WHITE;
    }
    return color;
}

RgbColor IndividualDigitColors::GetColonColorEnd() {
    RgbColor color = Pixels::ColorWheel(wheelColorsEnd[2]);
    if (show && selected == 2 &&
        (timeShown.Ms() < 300 ||
         timeShown.Ms() > 600 && timeShown.Ms() < 900)) {
        color = WHITE;
    }
    return color;
}

void IndividualDigitColors::Up() {
    if (selected < 5) {
        wheelColors[selected] -= 2;
        StoreSettings();
    }
}

void IndividualDigitColors::Down() {
    if (selected < 5) {
        wheelColors[selected] += 2;
        StoreSettings();
    }
}

bool IndividualDigitColors::Left() {
    if (selected > 0) {
        selected--;
    } else {
        selected = 4;
    }
    show = true;
    timeShown.Reset();
    return true;
}

bool IndividualDigitColors::Right() {
    if (selected < 4) {
        selected++;
    } else {
        selected = 0;
    }
    show = true;
    timeShown.Reset();
    return true;
}

bool IndividualDigitColors::CanChangeColor() {
    return true;
}

void IndividualDigitColors::StoreSettings() {
    (*settings)["ANIM_INDIVIDUAL_COL0"] = wheelColors[0];
    (*settings)["ANIM_INDIVIDUAL_COL1"] = wheelColors[1];
    (*settings)["ANIM_INDIVIDUAL_COLON"] = wheelColors[2];
    (*settings)["ANIM_INDIVIDUAL_COL2"] = wheelColors[3];
    (*settings)["ANIM_INDIVIDUAL_COL3"] = wheelColors[4];
    
    (*settings)["ANIM_INDIVIDUAL_COL0_END"] = wheelColorsEnd[0];
    (*settings)["ANIM_INDIVIDUAL_COL1_END"] = wheelColorsEnd[1];
    (*settings)["ANIM_INDIVIDUAL_COLON_END"] = wheelColorsEnd[2];
    (*settings)["ANIM_INDIVIDUAL_COL2_END"] = wheelColorsEnd[3];
    (*settings)["ANIM_INDIVIDUAL_COL3_END"] = wheelColorsEnd[4];
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
                } else if (dot.y == DISPLAY_HEIGHT) {
                    dot.xDir = -1;
                    dot.yDir = 0;
                    dot.x = DISPLAY_WIDTH - 2;
                    dot.y = DISPLAY_HEIGHT - 1;
                } else if (dot.x == -1) {
                    dot.xDir = 0;
                    dot.yDir = -1;
                    dot.x = 0;
                    dot.y = DISPLAY_HEIGHT - 2;
                } else if (dot.y == -1) {
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

Starfield::Starfield() : Animator() {
    name = "Warp Speed";
}

void Starfield::Start() {
    stars.resize(7);
    
    // Set animation frequency (update every 15ms for smooth animation)
    freq = 15;
    
    // Load color from settings
    if (!(*settings).containsKey("COLR")) {
        (*settings)["COLR"] = wheelPos;
    } else {
        wheelPos = (*settings)["COLR"].as<uint32_t>();
    }
    
    // Set the digit colors based on the wheel position
    for (auto& d : digitColors) {
        d = Pixels::ColorWheel(wheelPos);
    }
    
    // Set the colon color
    (*settings)["COLR_COLON"] = wheelPos;
    
    // Define the animation function
    func = [&](Animator& a) {
        // Don't clear the display - rely on natural fading from Clock class
        
        // Update and draw each star
        for (auto& star : stars) {
            // Update star position
            star.Update();
            
            // If star is out of bounds, reset it
            if (star.IsOutOfBounds()) {
                star.Reset();
            }
            
            // Draw the star at its current position
            // Use integer casting for the pixel coordinates
            int pixelX = static_cast<int>(star.x);
            int pixelY = static_cast<int>(star.y);
            
            // Only draw if within bounds (should be redundant with IsOutOfBounds check)
            if (pixelX >= 0 && pixelX < DISPLAY_WIDTH && pixelY >= 0 && pixelY < DISPLAY_HEIGHT) {
                // Always use white stars with varying brightness
                pixels->Set(pixelX, pixelY, star.color);
            }
        }
    };
}

SnowfallAnimator::SnowfallAnimator() : Animator() {
    name = "Snowfall";
}

void SnowfallAnimator::Start() {
    // Create snowflakes (reduced by 30% from 20 to 14)
    snowflakes.resize(14);
    
    // Initialize snow piles for accumulation
    snowPiles.resize(DISPLAY_WIDTH);
    for (auto& pile : snowPiles) {
        pile.Reset();
        pile.height = 0; // Start with no snow
    }
    
    // Initialize wind
    windStrength = 0;
    windChangeDuration = 3000 + (rand() % 5000); // 3-8 seconds
    windChangeTimer.Reset();
    
    // Set animation frequency (update every 20ms for smoother animation)
    freq = 20;
    
    // Load snow color from settings or use default
    if (!(*settings).containsKey("SNOW_COLOR")) {
        (*settings)["SNOW_COLOR"] = 0; // Default to white
        snowColorPos = 0;
    } else {
        snowColorPos = (*settings)["SNOW_COLOR"].as<uint32_t>();
    }
    
    // Load snow brightness from settings or use default (white)
    if (!(*settings).containsKey("SNOW_BRIGHTNESS")) {
        (*settings)["SNOW_BRIGHTNESS"] = 2; // Default to brighter white (level 2)
        snowBrightness = 0.66f; // Brighter white
    } else {
        int brightnessLevel = (*settings)["SNOW_BRIGHTNESS"].as<uint32_t>();
        // Ensure level is between 0-2
        brightnessLevel = std::min(2, std::max(0, brightnessLevel));
        // Convert level to brightness value (0.0, 0.33, 0.66)
        snowBrightness = (float)brightnessLevel / 3.0f;
        // Update settings to match our system
        (*settings)["SNOW_BRIGHTNESS"] = brightnessLevel;
    }
    
    // Set the digit colors based on the wheel position
    for (auto& d : digitColors) {
        d = Pixels::ColorWheel(wheelPos);
    }
    
    // Set the colon color
    (*settings)["COLR_COLON"] = wheelPos;
    
    // Create a display buffer to reduce flickering
    displayBuffer.resize(DISPLAY_WIDTH * DISPLAY_HEIGHT, BLACK);
    
    // Initialize snowflakes with random positions to avoid all starting at the top
    for (auto& flake : snowflakes) {
        flake.x = (float)(rand() % DISPLAY_WIDTH);
        flake.y = (float)(rand() % DISPLAY_HEIGHT);
        flake.speed = (float)(rand() % 50) / 100.0f + 0.1f;  // 0.1 to 0.6
        flake.size = (float)(rand() % 60) / 100.0f + 0.4f;  // 0.4 to 1.0
        
        // Calculate snow color based on current settings
        flake.color = CalculateSnowColor(snowBrightness);
    }
    
    // Define the animation function
    func = [&](Animator& a) {
        // Apply a gentle fade to the entire buffer to create trails
        for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
            // Fade existing pixels slightly to create a trail effect
            if (displayBuffer[i].R > 0 || displayBuffer[i].G > 0 || displayBuffer[i].B > 0) {
                // Apply a very slight fade to existing pixels (0.85 for longer trails)
                displayBuffer[i] = Pixels::ScaleBrightness(displayBuffer[i], 0.85f);
                
                // If pixel is too dim, turn it off completely
                if (displayBuffer[i].R < 5 && displayBuffer[i].G < 5 && displayBuffer[i].B < 5) {
                    displayBuffer[i] = BLACK;
                }
            }
        }
        
        // Update wind with smoother transitions
        if (windChangeTimer.Ms() >= windChangeDuration) {
            windChangeTimer.Reset();
            windChangeDuration = 2000 + (rand() % 3000); // 2-5 seconds (more frequent changes)
            
            // Target wind strength - stronger and more varied
            float targetWindStrength = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.8f; // -0.4 to 0.4
            
            // Set the wind strength directly for more noticeable effect
            windStrength = targetWindStrength;
        }
        
        // Update and draw each snowflake
        for (auto& flake : snowflakes) {
            // Update snowflake position with wind effect
            flake.Update(windStrength);
            
            // If snowflake has reached the bottom, add to snow pile and reset
            if (flake.HasReachedBottom()) {
                int pileX = static_cast<int>(flake.x);
                
                // Ensure pileX is within bounds
                if (pileX >= 0 && pileX < DISPLAY_WIDTH) {
                    // Only add to pile if it's not too high (max 3 lines)
                    if (snowPiles[pileX].height < 3) {
                        snowPiles[pileX].height++;
                        snowPiles[pileX].meltTimer.Reset();
                    }
                }
                
                flake.Reset();
                
                // Update the snowflake color based on current settings
                flake.color = CalculateSnowColor(snowBrightness);
            }
            
            // Draw the snowflake at its current position
            int pixelX = static_cast<int>(flake.x);
            int pixelY = static_cast<int>(flake.y);
            
            // Only draw if within bounds
            if (pixelX >= 0 && pixelX < DISPLAY_WIDTH && pixelY >= 0 && pixelY < DISPLAY_HEIGHT) {
                // Add to buffer - just use the snowflake's color (no glow effect)
                displayBuffer[pixelY * DISPLAY_WIDTH + pixelX] = flake.color;
            }
        }
        
        // Draw and update snow piles
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            auto& pile = snowPiles[x];
            
            // Check if pile should melt - faster melting (2500-5000ms instead of 5000-10000ms)
            if (pile.height > 0 && pile.ShouldMelt()) {
                pile.height--;
                pile.meltTimer.Reset();
                pile.meltDelay = 1000 + (rand() % 1000); // 1-2 seconds (much faster melting)
            }
            
            // Draw the snow pile
            for (int h = 0; h < pile.height; h++) {
                int y = DISPLAY_HEIGHT - 1 - h;
                if (y >= 0 && y < DISPLAY_HEIGHT) {
                    // Use slightly different shades based on current settings
                    float brightness = 0.7f + ((float)(rand() % 30) / 100.0f);
                    RgbColor pileColor = CalculateSnowColor(snowBrightness);
                    // Add to buffer
                    displayBuffer[y * DISPLAY_WIDTH + x] = pileColor;
                }
            }
        }
        
        // Draw the buffer to the display
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                pixels->Set(x, y, displayBuffer[y * DISPLAY_WIDTH + x]);
            }
        }
    };
}

// Left button cycles through brightness levels: COLOR -> DIM WHITE -> BRIGHT WHITE
bool SnowfallAnimator::Left() {
    // Get current brightness level (0, 1, or 2)
    int currentLevel = (int)(snowBrightness * 3.0f + 0.5f);
    
    // Cycle to next level
    currentLevel = (currentLevel + 1) % 3;
    
    // Convert level to brightness value
    snowBrightness = (float)currentLevel / 3.0f;
    
    // Store in settings
    (*settings)["SNOW_BRIGHTNESS"] = currentLevel;
    
    // Update all snowflake colors with new brightness
    UpdateSnowflakeColors();
    
    return true;  // We handled the button press
}

// Right button puts snow in COLOR mode and cycles through colors
bool SnowfallAnimator::Right() {
    // Always set to color mode (brightness 0)
    snowBrightness = 0.0f;
    (*settings)["SNOW_BRIGHTNESS"] = 0;
    
    // Cycle through colors
    snowColorPos += 16;  // Larger step for more noticeable changes
    (*settings)["SNOW_COLOR"] = snowColorPos;
    
    // Update all snowflake colors with new color
    UpdateSnowflakeColors();
    
    return true;  // We handled the button press
}

// Helper method to update all snowflake colors based on current settings
void SnowfallAnimator::UpdateSnowflakeColors() {
    // Update all existing snowflakes with the new color/brightness
    for (auto& flake : snowflakes) {
        flake.color = CalculateSnowColor(snowBrightness);
    }
}

// Helper method to calculate snow color based on brightness level
RgbColor SnowfallAnimator::CalculateSnowColor(float brightness) {
    // Get brightness level (0, 1, or 2)
    int level = (int)(brightness * 3.0f + 0.5f);
    level = std::min(2, std::max(0, level));
    
    RgbColor color;
    
    switch (level) {
        case 0: // Level 0: Base color from color wheel
            color = Pixels::ColorWheel(snowColorPos);
            break;
            
        case 1: // Level 1: Dim white (33%)
            color = Pixels::ScaleBrightness(WHITE, 0.5f);
            break;
            
        case 2: // Level 2: Brighter white (66%)
        default:
            color = WHITE;
            break;
    }
    
    return color;
}
#endif

RgbColor Animator::GetAdjustedDigitColor(size_t index) {
    if (index < digitColors.size()) {
        return Pixels::ScaleBrightness(digitColors[index], digitBrightness[index]);
    }
    return BLACK;
}

RgbColor Animator::GetAdjustedDigitColorEnd(size_t index) {
    if (index < digitColorEnds.size()) {
        return Pixels::ScaleBrightness(digitColorEnds[index], digitBrightnessEnds[index]);
    }
    return BLACK;
}

RgbColor Animator::GetAdjustedColonColor() {
    return Pixels::ScaleBrightness(GetColonColor(), colonBrightness);
}

RgbColor Animator::GetAdjustedColonColorEnd() {
    return Pixels::ScaleBrightness(GetColonColorEnd(), colonBrightnessEnd);
}

void Animator::SetDigitBrightness(size_t index, float brightness) {
    if (index < digitBrightness.size()) {
        digitBrightness[index] = std::max(0.0f, std::min(1.0f, brightness));
    }
}

void Animator::SetDigitBrightnessEnd(size_t index, float brightness) {
    if (index < digitBrightnessEnds.size()) {
        digitBrightnessEnds[index] = std::max(0.0f, std::min(1.0f, brightness));
    }
}

void Animator::SetColonBrightness(float brightness) {
    colonBrightness = std::max(0.0f, std::min(1.0f, brightness));
}

void Animator::SetColonBrightnessEnd(float brightness) {
    colonBrightnessEnd = std::max(0.0f, std::min(1.0f, brightness));
}

void Animator::SetAllDigitsBrightness(float brightness) {
    brightness = std::max(0.0f, std::min(1.0f, brightness));
    for (size_t i = 0; i < digitBrightness.size(); i++) {
        digitBrightness[i] = brightness;
        digitBrightnessEnds[i] = brightness;
    }
    colonBrightness = brightness;
    colonBrightnessEnd = brightness;
}

void Animator::BrightnessUp() {
    float currentBrightness = digitBrightness[0]; // Assume all are the same
    SetAllDigitsBrightness(std::min(1.0f, currentBrightness + 0.1f));
    
    // Store in settings
    (*settings)["BRIGHTNESS"] = static_cast<int>(digitBrightness[0] * 100);
}

void Animator::BrightnessDown() {
    float currentBrightness = digitBrightness[0]; // Assume all are the same
    SetAllDigitsBrightness(std::max(0.1f, currentBrightness - 0.1f));
    
    // Store in settings
    (*settings)["BRIGHTNESS"] = static_cast<int>(digitBrightness[0] * 100);
}

