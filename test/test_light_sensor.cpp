#include <gtest/gtest.h>

#include <light_sensor.hpp>  // the unit of code being tested

///// Test Fixture (Fx), contains SetUp, TearDown, and shared variables ///////
class LightSensorFx : public ::testing::Test {
  protected:
    uint16_t analogVal{0};
    LightSensor ls;

    virtual void SetUp() {
        ls.SetAnalogReadFunc([&](const uint8_t pin) { return analogVal; });
    }

    void UpdateAnalogValue(const uint16_t value) {
        analogVal = value;
        ls.Update();
    }

    // HELPERS
};

///// Individual tests (all are member functions of the fixture) //////////////
TEST_F(LightSensorFx, DoesLightSensorDetectCurrentValAfterReset) {
    UpdateAnalogValue(LightSensor::HW_SENSOR_MAX);
    ls.ResetToCurrentSensorValue();
    EXPECT_GE(ls.GetAverageValue(), LightSensor::FILTER_VAL_MAX);
}

TEST_F(LightSensorFx, CanGetScaledBrightness) {
    ls.SetCacheSizeAndCurrentValue(5, LightSensor::HW_SENSOR_MAX / 2);
    EXPECT_EQ(ls.GetAverageValue(), LightSensor::HW_SENSOR_MAX / 2);

    EXPECT_FLOAT_EQ(ls.GetScaled(), 0.5f);
}
