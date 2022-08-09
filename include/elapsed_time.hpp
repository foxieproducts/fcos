#pragma once
#include <stdint.h>         // for size_t
#include <arduino_hal.hpp>  // for millis()

class ElapsedTime {
  private:
    unsigned long m_millis{0xFFFFFFFF};

  public:
    ElapsedTime() { Reset(); }
    void Reset() { m_millis = millis(); }
    size_t Ms() const { return millis() - m_millis; }
    size_t S() const { return Ms() / 1000; }

    static void Delay(const size_t ms) {
        ElapsedTime delayTime;
        do {
            delay(1);
        } while (delayTime.Ms() < ms);
    }
};
