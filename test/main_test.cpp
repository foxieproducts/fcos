///////////////////////////////////////////////////////////////////////////////
// Example test fixture for the ElapsedTime class -- normally, this would be in
// its own file along with #include <gtest/gtest.h>
//
// Start here: https://google.github.io/googletest/primer.html
///////////////////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>

#include <elapsed_time.hpp>  // the unit of code being tested
///// Test Fixture (Fx), contains SetUp, TearDown, and shared variables ///////
class ElapsedTimeFx : public ::testing::Test {
  protected:
    // shared variable(s), instantiated for each test
    ElapsedTime et;  // sadly, doesn't phone home for #dadjokes

    // virtual void SetUp() {}     // called for each test
    // virtual void TearDown() {}  // called for each test

    // Helper functions for tests to use, to reduce code duplication
    // void DoMoreElaborateThings() { ElaborateThings(); }
};

///// Individual tests (all are member functions of the fixture) //////////////
TEST_F(ElapsedTimeFx, MeasuresRoughly100Milliseconds) {
    EXPECT_LE(et.Ms(), 1);  // at ms resolution, this should still be near 0

    delay(100);
    EXPECT_GE(et.Ms(), 90);
    EXPECT_LE(et.Ms(), 110);
}

TEST_F(ElapsedTimeFx, CanResetToZero) {
    delay(50);
    EXPECT_GT(et.Ms(), 40);

    et.Reset();
    EXPECT_LE(et.Ms(), 1);
}

///////////////////////////////////////////////////////////////////////////////
// Everything below here is required ONLY in test_main.cpp.
///////////////////////////////////////////////////////////////////////////////
//
//
//
// This allows all the unit tests to be run from the host and target
// machines within the VSCode + PlatformIO UI. This chunk of code is
// only necessary within test_main.cpp
//
//
//
//////////////////////////////////////////////////////////////////////////////
#if ARDUINO
#include <Arduino.h>
void setup() {
    Serial.begin();
    ::testing::InitGoogleTest();
    if (RUN_ALL_TESTS())
        ;  // nothing else to do if this fails
}
void loop() {
    delay(1000);
}
#else
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
