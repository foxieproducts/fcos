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

        HW_SENSOR_MIN = 0,
        HW_SENSOR_MAX = 20,  // past this is "really bright"
        HW_SENSOR_RANGE = HW_SENSOR_MAX - HW_SENSOR_MIN,

        FILTER_VAL_MIN = 0,
        FILTER_VAL_MAX = 20,
        FILTER_RANGE = FILTER_VAL_MAX - FILTER_VAL_MIN,
    };
    const float ALLOWED_JITTER = FILTER_VAL_MAX / 100.0f;
    const float ZERO_THRESHOLD = 0.0f + ALLOWED_JITTER;

  private:
    using AnalogReadFunc_t = std::function<uint16_t(const uint8_t pin)>;
    // by default, just uses Arduino's analogRead
    AnalogReadFunc_t m_analogReadFunc{
        [](const uint8_t pin) { return analogRead(pin); }};
    std::vector<float> m_cache;
    size_t m_cachePos{0};
    float m_average{0};

  public:
    LightSensor() { ResetToCurrentSensorValue(); }

    void Update() {
        m_cache[m_cachePos] = GetJitterConstrainedAnalogValue();
        if (++m_cachePos == m_cache.size()) {
            m_cachePos = 0;
        }
    }

    float GetAverageValue() const { return m_average; }
    float GetScaled(const size_t places = 3) {
        const float truncatedAverage =
            std::round((m_average / FILTER_VAL_MAX) * std::pow(10, places)) /
            std::pow(10, places);
        return truncatedAverage;
    }

    float GetJitterConstrainedAnalogValue() {
        float sample = GetMultipleAnalogSamples();
        sample = std::min(std::max((float)FILTER_VAL_MIN, sample),
                          (float)FILTER_VAL_MAX);

        m_cache[m_cachePos] = sample;
        const float avg =
            std::accumulate(m_cache.begin(), m_cache.end(), 0.0f) /
            m_cache.size();
        const float delta = std::abs(m_average - avg);

        if (sample >= ALLOWED_JITTER &&
            sample <= (FILTER_VAL_MAX - ALLOWED_JITTER) &&
            delta <= ALLOWED_JITTER) {
            sample = avg;
            // m_average doesn't change when inside this window
        } else {
            m_average = avg;
        }

        return sample;
    }

    float GetMultipleAnalogSamples(const size_t numSamples = 16) {
        float avg = 0.0f;
        for (size_t i = 0; i < numSamples; ++i) {
            avg += GetCurrentAnalogValue();
        }

        avg /= numSamples;

        if (avg <= ZERO_THRESHOLD) {
            avg = 0.0f;  // close enough to 0 to call it zero.
        }

        return avg;
    }

    int16_t GetCurrentAnalogValue() {
        uint16_t val = m_analogReadFunc(PIN_LIGHT_SENSOR);
        // scale the hw value to the range of the filter values
        return (((val - HW_SENSOR_MIN) * FILTER_RANGE) / HW_SENSOR_RANGE) +
               FILTER_VAL_MIN;
    }

    void ResetToCurrentSensorValue() {
        m_average = GetMultipleAnalogSamples();
        SetCacheSizeAndCurrentValue(CACHE_SIZE, m_average);
    }

    void SetCacheSizeAndCurrentValue(const size_t cacheSize,
                                     const float currentValue) {
        m_cache.clear();
        m_cache.resize(cacheSize, currentValue);
        m_average = currentValue;
        m_cachePos = 0;
    }

    void SetAnalogReadFunc(const AnalogReadFunc_t& func) {
        m_analogReadFunc = func;
    }
};
