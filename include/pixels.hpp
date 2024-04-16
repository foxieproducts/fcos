#pragma once
#include <NeoPixelBus.h>  // for communication with LEDs
#include <vector>         // for std::vector (primarily for characters/*.inc)

#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <light_sensor.hpp>
#include <settings.hpp>

static RgbColor BLACK(0, 0, 0);
static RgbColor WHITE(255, 255, 255);
static RgbColor GRAY(128, 128, 128);
static RgbColor RED(255, 0, 0);
static RgbColor GREEN(0, 255, 0);
static RgbColor BLUE(0, 0, 255);
static RgbColor YELLOW(255, 255, 0);
static RgbColor CYAN(0, 255, 255);
static RgbColor PURPLE(96, 0, 255);
static RgbColor MAGENTA(255, 0, 255);
static RgbColor DARK_RED(128, 0, 0);
static RgbColor DARK_GREEN(0, 128, 0);
static RgbColor DARK_BLUE(0, 0, 128);
static RgbColor DARK_YELLOW(128, 128, 0);
static RgbColor DARK_CYAN(0, 128, 128);
static RgbColor DARK_MAGENTA(128, 0, 128);
static RgbColor DARK_PURPLE(32, 0, 224);
static RgbColor DARK_GRAY(32, 32, 32);
static RgbColor EXTRA_DARK_GRAY(16, 16, 16);
static RgbColor EARL_GRAY(17, 01, 'D');
static RgbColor ORANGE(255, 128, 0);
static RgbColor DARK_ORANGE(128, 64, 0);
static RgbColor LIGHT_GRAY(224, 224, 224);
static RgbColor LIGHT_YELLOW(224, 224, 0);

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
    CHAR_DISPLAY_HEIGHT = 20,
    OPTION_LEDS = 8 + 2,
    NUM_RINGS = 0,
    SCROLL_DELAY_HORIZONTAL_MS = 8,
    SCROLL_DELAY_VERTICAL_MS = 8,
#elif FCOS_CARDCLOCK
    DISPLAY_WIDTH = 17,
    DISPLAY_HEIGHT = 5,
    CHAR_DISPLAY_HEIGHT = 5,
    OPTION_LEDS = 0,
    NUM_RINGS = 2,
    SCROLL_DELAY_HORIZONTAL_MS = 10,
    SCROLL_DELAY_VERTICAL_MS = 20,
#elif FCOS_CARDCLOCK2
    DISPLAY_WIDTH = 17,
    DISPLAY_HEIGHT = 11,
    CHAR_DISPLAY_HEIGHT = 5,
    OPTION_LEDS = 0,
    NUM_RINGS = 3,
    SCROLL_DELAY_HORIZONTAL_MS = 10,
    SCROLL_DELAY_VERTICAL_MS = 20,
#endif
    RING_SIZE = 12,
    ROUND_LEDS = RING_SIZE * NUM_RINGS,

    CHAR_HEIGHT = 5,
    TOTAL_MATRIX_LEDS = DISPLAY_WIDTH * DISPLAY_HEIGHT,
    FIRST_RING_LED = TOTAL_MATRIX_LEDS,
    TOTAL_ALL_LEDS = TOTAL_MATRIX_LEDS + ROUND_LEDS + OPTION_LEDS,

    SCROLLING_TEXT_MS = 50,
    FRAMES_PER_SECOND = 30,
    LIGHT_SENSOR_UPDATE_MS = 33,

    MIN_DISPLAY_BRIGHTNESS_DEFAULT = 1,
    MAX_DISPLAY_BRIGHTNESS = 9,
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
    // Note: NeoPixelBus' Tx1812 timing seems to cause more glitches with
    // the TC2020, so just use the default WS2812x timing
    // NeoEsp32BitBangWs2812xMethod does not flicker
    // NeoEsp32Rmt0Tx1812Method has flicker
    // NeoEsp32Rmt1Tx1812Method has flicker
    // NeoEsp32Rmt0Ws2812xMethod is _pretty stable_ but all the other
    //                           ones using the RMT behave badly with
    //                           network traffic
    //                           it's almost like there's a special case
    //                           in the ESP core for this kind of use
    //                           on channel 0
    // NeoEsp32Rmt1Ws2812xMethod flickers less often
    NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812xMethod> m_neoPixels;

#elif FCOS_ESP8266
    NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> m_neoPixels;
#endif
    std::shared_ptr<Settings> m_settings;
    LightSensor m_lightSensor;
    float m_currentBrightness{-1};
    float m_adjustedBrightness{-1};

    ElapsedTime m_sinceLastLightSensorUpdate;

    bool m_isPXLmode{false};
    bool m_useDarkMode{false};

  public:
    Pixels(std::shared_ptr<Settings> settings);

    void Update();

    void Show();

    void Clear(const RgbColor color = BLACK,
               const bool includeOptionLEDs = true,
               const bool includeRoundLEDs = false);

    void Darken(const size_t numTimes = 1,
                const float amount = 0.85f,
                const size_t delayMs = 15);

    void Set(const int pos,
             const RgbColor color,
             const bool skipBrightnessScaling = false);
    void Set(const int x,
             const int y,
             const RgbColor color,
             const bool force = false);

    int DrawText(int x, String text, const RgbColor color);

    int DrawText(int x, int y, String text, const RgbColor color);

    int DrawChar(int x, char character, const RgbColor color);

    int DrawChar(int x, int y, char character, const RgbColor color);

    static RgbColor ColorWheel(uint8_t pos);

    float GetBrightness();

    static RgbColor ScaleBrightness(const RgbColor color,
                                    const float brightness);

    void ToggleDarkMode();

    void EnableDarkMode();

    void DisableDarkMode();

    bool IsDarkModeEnabled();

    void DrawColorWheelBetween(uint8_t wheelPos,
                               const size_t x1,
                               const size_t x2);

#if FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    // TODO: Move these functions into a CC-specific version of Pixels

    // if pos == 0 then LED is at the 1:00 position
    void DrawRingLED(const int ringNum,
                     const int pos,
                     const RgbColor color,
                     const bool forceColor = false);

    void ClearRoundLEDs(const RgbColor color = BLACK);

    void DrawColorWheel(const uint8_t bottomPixelWheelPos);

    void DrawTextScrolling(const String& text,
                           const RgbColor color,
                           const size_t delayMs = SCROLLING_TEXT_MS);

    void DrawHourLED(const int hour, const RgbColor color);

    void DrawMinuteLED(const int minute, const RgbColor color);

    void DrawSecondLEDs(const int second,
                        const RgbColor color,
                        const int brightestLED = 0);  // 0,1,2
#endif
    void Move(const int fromCol,
              const int fromRow,
              const int toCol,
              const int toRow);

    void Move(const int from, const int to);

    void MoveHorizontal(const int num);

    void MoveVertical(const int num);

  private:
    void SetLEDBrightnessMultiplierFromSensor();
};
