#pragma once
// The idea here is to provide a common interface for Arduino
// when unit testing on non-Arduino platforms (such as the host machine).

#if ARDUINO
#include <Arduino.h>
#else

// fake everything for non-Arduino platforms,
// such as unit tests when running natively

#include <chrono>
#include <thread>
#define millis()                                             \
    std::chrono::duration_cast<std::chrono::milliseconds>(   \
        std::chrono::steady_clock::now().time_since_epoch()) \
        .count()
#define delay(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#define digitalRead(pin) \
    0  // TODO: Make some kind of configurable interface for this
#define analogRead(pin) 0  // TODO: and this
#define LOW 0x0
#define HIGH 0x1
#define INPUT 0x01
#define OUTPUT 0x03
#define PULLUP 0x04
#define INPUT_PULLUP 0x05
#define PULLDOWN 0x08
#define INPUT_PULLDOWN 0x09
#define ANALOG 0xC0
#endif
