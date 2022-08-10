#pragma once
// Everything here goes straight out the serial port normally
#include <arduino_hal.hpp>
#if ARDUINO

#define DPRINT(...)                 \
    if (Serial.availableForWrite()) \
        Serial.printf(__VA_ARGS__);

#define TDDPRINT(RTC, ...)                                           \
    DPRINT("[%02d:%02d:%02d.%03d] ", (RTC->Hour()), (RTC->Minute()), \
           (RTC->Second()), (RTC->Millis()))                         \
    DPRINT(__VA_ARGS__)

#define TDPRINT(RTC, ...)                                       \
    DPRINT("[%02d:%02d:%02d] ", (RTC->Hour()), (RTC->Minute()), \
           (RTC->Second()))                                     \
    DPRINT(__VA_ARGS__)

#else
#define DPRINT(...) printf(__VA_ARGS__)
#define TDPRINT(...) printf(__VA_ARGS__)
#define TDDPRINT(...) printf(__VA_ARGS__)
#endif
