#include <light_sensor.hpp>

LightSensor::LightSensor() {
    ResetToCurrentSensorValue();
}

void LightSensor::Update() {
    m_cache[m_cachePos] = GetJitterConstrainedAnalogValue();
    if (++m_cachePos == m_cache.size()) {
        m_cachePos = 0;
    }
}

float LightSensor::GetScaled(const size_t places) {
    const float truncatedAverage =
        std::round((m_average / FILTER_VAL_MAX) * std::pow(10, places)) /
        std::pow(10, places);
    return truncatedAverage;
}

float LightSensor::GetJitterConstrainedAnalogValue() {
    float sample = GetMultipleAnalogSamples();
    sample = std::min(std::max((float)FILTER_VAL_MIN, sample),
                      (float)FILTER_VAL_MAX);

    m_cache[m_cachePos] = sample;
    const float avg =
        std::accumulate(m_cache.begin(), m_cache.end(), 0.0f) / m_cache.size();
    const float delta = std::abs(m_average - avg);

    if (sample > 0.0f && delta <= ALLOWED_JITTER &&
        sample < FILTER_VAL_MAX - ALLOWED_JITTER) {
        sample = avg;
        //  m_average doesn't change when inside this window
    } else {
        m_average = avg;
    }

    return sample;
}

float LightSensor::GetMultipleAnalogSamples(const size_t numSamples) {
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

size_t LightSensor::GetCurrentAnalogValue() {
    size_t val = m_analogReadFunc(PIN_LIGHT_SENSOR);
    // scale the hw value to the range of the filter values
    val = std::min(std::max(m_hwMin, val), m_hwMax);
    return (((val - m_hwMin) * FILTER_RANGE) / (m_hwMax - m_hwMin)) +
           FILTER_VAL_MIN;
}
void LightSensor::ResetToCurrentSensorValue() {
    m_average = GetMultipleAnalogSamples();
    SetCacheSizeAndCurrentValue(CACHE_SIZE, m_average);
}

void LightSensor::SetCacheSizeAndCurrentValue(const size_t cacheSize,
                                              const float currentValue) {
    m_cache.clear();
    m_cache.resize(cacheSize, currentValue);
    m_average = currentValue;
    m_cachePos = 0;
}

void LightSensor::SetAnalogReadFunc(const AnalogReadFunc_t& func) {
    m_analogReadFunc = func;
}

