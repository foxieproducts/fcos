#include <gtest/gtest.h>

#include <button.hpp>  // the unit of code being tested

///// Test Fixture (Fx), contains SetUp, TearDown, and shared variables ///////
class ButtonFx : public ::testing::Test {
  protected:
    int pinState{HIGH};
    Button btn{0};  // the pin parameter is actually unused for testing

    virtual void SetUp() {
        btn.SetDigitalReadFunc([&](const int pin) { return pinState; });
        btn.config.beforeRepeat = 50;
        btn.config.repeatRate = 10;
    }
    virtual void TearDown() {}

    // Helper functions for tests to use, to reduce code duplication
    void UpdatePinState(int state) {
        pinState = state;
        btn.Update();
    }
};

///// Individual tests (all are member functions of the fixture) //////////////
TEST_F(ButtonFx, DoesButtonDetectPress) {
    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());
}

TEST_F(ButtonFx, AreBouncesIgnoredOnPress) {
    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());

    // now the button is ignoring changes for a bit
    UpdatePinState(HIGH);
    EXPECT_TRUE(btn.IsPressed());

    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());

    UpdatePinState(HIGH);
    EXPECT_TRUE(btn.IsPressed());

    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());

    UpdatePinState(HIGH);
    EXPECT_TRUE(btn.IsPressed());

    delay(btn.config.debounceTime + 1);
    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());
}

TEST_F(ButtonFx, AreBouncesIgnoredOnRelease) {
    UpdatePinState(LOW);
    EXPECT_TRUE(btn.IsPressed());

    delay(btn.config.debounceTime + 1);
    UpdatePinState(HIGH);
    EXPECT_FALSE(btn.IsPressed());

    // now the button is ignoring changes for a bit
    UpdatePinState(LOW);
    EXPECT_FALSE(btn.IsPressed());

    UpdatePinState(HIGH);
    EXPECT_FALSE(btn.IsPressed());

    UpdatePinState(LOW);
    EXPECT_FALSE(btn.IsPressed());

    delay(btn.config.debounceTime + 1);
    UpdatePinState(HIGH);
    EXPECT_TRUE(!btn.IsPressed());
}

TEST_F(ButtonFx, IsPressEventSent) {
    bool receivedEvent = false;
    btn.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            receivedEvent = true;
        }
    };

    UpdatePinState(LOW);
    EXPECT_TRUE(receivedEvent);
}

TEST_F(ButtonFx, IsDelayBeforePressUsed) {
    bool receivedEvent = false;
    ElapsedTime etBeforePress;
    size_t msBeforePressEvent = 0;
    pinState = LOW;
    btn.config.beforePress = 100;
    btn.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            msBeforePressEvent = etBeforePress.Ms();
            receivedEvent = true;
        }
    };

    btn.Update();
    EXPECT_FALSE(receivedEvent);

    do {
        btn.Update();
        delay(1);
    } while (etBeforePress.Ms() < btn.config.beforePress * 2 && !receivedEvent);

    EXPECT_TRUE(receivedEvent);
    EXPECT_GE(msBeforePressEvent, btn.config.beforePress - 10);
    EXPECT_LE(msBeforePressEvent, btn.config.beforePress + 10);
}

TEST_F(ButtonFx, IsReleaseEventSent) {
    bool receivedEvent = false;
    btn.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::Event_e::RELEASE) {
            receivedEvent = true;
        }
    };

    UpdatePinState(LOW);

    delay(btn.config.debounceTime + 1);
    UpdatePinState(HIGH);
    EXPECT_TRUE(receivedEvent);
}

TEST_F(ButtonFx, AreRepeatEventsSent) {
    size_t repeatEvents = 0;
    btn.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::REPEAT) {
            repeatEvents++;
        }
    };

    UpdatePinState(LOW);

    delay(btn.config.beforeRepeat + btn.config.repeatRate);
    btn.Update();  // causes first REPEAT event
    EXPECT_EQ(1, repeatEvents);

    btn.Update();  // should not yet cause another REPEAT event
    EXPECT_EQ(1, repeatEvents);

    // cause 5 more REPEAT events
    for (size_t c = 0; c < btn.config.repeatRate * 5; c++) {
        delay(1);
        btn.Update();
    }
    EXPECT_GE(repeatEvents, 6);
}