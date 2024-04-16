#pragma once
#include <Rtc_Pcf8563.h>
#include <Time.h>
#include <Timezone.h>  // https://github.com/JChristensen/Timezone
#if FCOS_ESP32_C3
#include <WiFi.h>
#elif FCOS_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <map>
#include <vector>

#include <elapsed_time.hpp>
#include <settings.hpp>

class Rtc {
  private:
    enum {
        TIMER_FREQUENCY = 1,  // at 1Hz, this results in 1 interrupt per second
        MAX_WAIT_FOR_NTP_MS = 250,
    };

    std::shared_ptr<Settings> m_settings;

    Rtc_Pcf8563 m_rtc;
    bool m_isInitialized{false};

    uint8_t m_hour{0}, m_minute{0}, m_second{0};  // 12:00:00 AM
    size_t m_millisAtInterrupt{0};
    size_t m_uptime{0};

    struct tm m_timeinfo;
    size_t m_uptimeForNextNTPUpdate{5};

    static bool m_receivedInterrupt;  // used by InterruptISR()

  public:
    Rtc(std::shared_ptr<Settings> settings);

    bool IsInitialized();
    void Update();

    uint8_t Hour() { return m_hour; }
    uint8_t Hour12() { return Conv24to12(Hour()); }
    uint8_t Minute() { return m_minute; }
    uint8_t Second() { return m_second; }
    size_t Millis() { return ((size_t)millis() - m_millisAtInterrupt) % 1000; }
    size_t Uptime() { return m_uptime; }

    void SetTime(uint8_t hour, uint8_t minute, uint8_t second);
    static int Conv24to12(int hour);
    void SetClockToZero();
    std::vector<String> GetTimezoneNames();
    int GetTimezoneNumFromName(const String& name);
    void ForceNTPUpdate();

  private:
    void Initialize();
    void GetTimeFromRTC();
    void CheckNTPTime();
    bool GetLocalTime(struct tm* info, uint32_t ms);

    struct NamedTimezone {
        String name;
        Timezone tz;
    };
    std::vector<NamedTimezone> m_timezones;
    void SetupTimezones();

    void AttachInterrupt();
    static inline void IRAM_ATTR InterruptISR();
};
