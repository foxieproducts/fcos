#pragma once
#include <algorithm>        // for std::min/max
#include <arduino_hal.hpp>  // for analogRead(), etc.
#include <functional>       // for std::function
#include <numeric>          // for std::accumulate
#include <vector>           // for std::vector

class LightSensor {
  public:
    enum {
        CACHE_SIZE = 25,
        HW_MIN = 0,
        HW_MAX = 70,
        FILTER_VAL_MIN = 0,
        FILTER_VAL_MAX = 20,
        FILTER_RANGE = FILTER_VAL_MAX - FILTER_VAL_MIN,
    };
    const float ALLOWED_JITTER = FILTER_VAL_MAX * 0.01f;
    const float ZERO_THRESHOLD = FILTER_VAL_MAX * 0.01f;

    // these values must be adjusted to match the light sensor
    size_t m_hwMin{HW_MIN};
    size_t m_hwMax{HW_MAX};

  private:
    using AnalogReadFunc_t = std::function<uint16_t(const uint8_t pin)>;
    // by default, just uses Arduino's analogRead
    AnalogReadFunc_t m_analogReadFunc{
        [](const uint8_t pin) { return analogRead(pin); }};
    std::vector<float> m_cache;
    size_t m_cachePos{0};
    float m_average{0};

  public:
    LightSensor();

    void Update();

    void SetHwMin(const size_t hwMin) { m_hwMin = hwMin; }
    void SetHwMax(const size_t hwMax) { m_hwMax = hwMax; }
    size_t GetHwMin() const { return m_hwMin; }
    size_t GetHwMax() const { return m_hwMax; }
    float GetAverageValue() const { return m_average; }
    float GetScaled(const size_t places = 3);

    float GetJitterConstrainedAnalogValue();

    float GetMultipleAnalogSamples(const size_t numSamples = 16);

    size_t GetCurrentAnalogValue();

    void ResetToCurrentSensorValue();

    void SetCacheSizeAndCurrentValue(const size_t cacheSize,
                                     const float currentValue);

    void SetAnalogReadFunc(const AnalogReadFunc_t& func);
};
