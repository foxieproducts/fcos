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
    size_t m_secondsUntilNextNTPUpdate{10};
    ElapsedTime m_timeSinceLastNTPUpdate;

    static bool m_receivedInterrupt;  // used by InterruptISR()

  public:
    Rtc(std::shared_ptr<Settings> settings) : m_settings(settings) {
        GetTimeFromRTC();
        SetupTimezones();

        // try to get the RTC initialized for up to 25ms
        ElapsedTime initTime;
        while (!m_isInitialized && initTime.Ms() < 25) {
            Update();
            ElapsedTime::Delay(1);
        }
    }

    bool IsInitialized() { return m_isInitialized; }

    void Update() {
        if (!m_isInitialized) {
            Initialize();
        }

        if (m_receivedInterrupt) {
            // happens once per second
            m_receivedInterrupt = false;
            m_uptime++;
            GetTimeFromRTC();
        }

        CheckNTPTime();
    }

    uint8_t Hour() { return m_hour; }
    uint8_t Hour12() { return Conv24to12(Hour()); }
    uint8_t Minute() { return m_minute; }
    uint8_t Second() { return m_second; }
    size_t Millis() { return ((size_t)millis() - m_millisAtInterrupt) % 1000; }
    size_t Uptime() { return m_uptime; }

    void SetTime(uint8_t hour, uint8_t minute, uint8_t second) {
        m_rtc.setTime(hour, minute, second);
        GetTimeFromRTC();
    }
    static int Conv24to12(int hour) {
        if (hour > 12) {
            hour -= 12;
        } else if (hour == 0) {
            hour = 12;
        }
        return hour;
    }

    void SetClockToZero() {
        m_rtc.zeroClock();
        GetTimeFromRTC();
    }

    std::vector<String> GetTimezoneNames() {
        std::vector<String> names;
        for (const auto& tz : m_timezones) {
            names.push_back(tz.name);
        }
        return names;
    }

    void ForceNTPUpdate() {
        m_timeSinceLastNTPUpdate.Reset();
        m_secondsUntilNextNTPUpdate = 5;
    }

  private:
    void Initialize() {
        // try to set the alarm -- if it succeeds, the RTC is booted and we can
        // setup the 1 second timer
        m_rtc.getDateTime();
        m_rtc.setAlarm(1, 2, 3, 4);
        m_rtc.enableAlarm();
        if (m_rtc.getAlarmMinute() == 1 && m_rtc.getAlarmHour() == 2 &&
            m_rtc.getAlarmDay() == 3 && m_rtc.getAlarmWeekday() == 4) {
            m_rtc.clearAlarm();

            m_rtc.getDateTime();
            if (m_rtc.getTimerValue() != TIMER_FREQUENCY) {
                // Configured at 1Hz, this results in 1 interrupt per second //
                // fresh boot -- no time backup, timer was not enabled
                m_rtc.zeroClock();
            }

            m_rtc.clearStatus();
            m_rtc.clearTimer();
            // interrupt every 1 second
            m_rtc.setTimer(TIMER_FREQUENCY, TMR_1Hz, true);

            m_isInitialized = true;
            AttachInterrupt();

            GetTimeFromRTC();
        }
    }

    void GetTimeFromRTC() {
        unsigned long msAtInterrupt = millis();

        // this is not ideal, but some things mess up the RMT output, which
        // causes a rare flicker. this is a workaround, to cause the rest of
        // the system to wait while the LEDs are being updated.
        ElapsedTime::Delay(10);

        m_rtc.getDateTime();
        if (m_rtc.getHour() < 24) {
            m_hour = m_rtc.getHour();
            m_minute = m_rtc.getMinute();
            if (m_second != m_rtc.getSecond()) {
                m_millisAtInterrupt = msAtInterrupt;
                m_second = m_rtc.getSecond();
            }
        }
    }

    void CheckNTPTime() {
        if (WiFi.isConnected() &&
            m_timeSinceLastNTPUpdate.S() > m_secondsUntilNextNTPUpdate) {
            m_timeSinceLastNTPUpdate.Reset();
            if (GetLocalTime(&m_timeinfo, MAX_WAIT_FOR_NTP_MS)) {
                m_secondsUntilNextNTPUpdate = (189 * 60) + rand() % 120;

                int selectedTimezone = (*m_settings)["TIMEZONE"].as<int>();
                auto local = m_timezones[selectedTimezone].tz.toLocal(
                    mktime(&m_timeinfo));
                m_timeinfo = *localtime(&local);

                SetTime(m_timeinfo.tm_hour, m_timeinfo.tm_min,
                        m_timeinfo.tm_sec);

                TDPRINT(this,
                        "Got NTP time (%02d:%02d:%02d) (TZ: %s) next update in "
                        "~3 hours    \n",
                        m_timeinfo.tm_hour, m_timeinfo.tm_min,
                        m_timeinfo.tm_sec,
                        m_timezones[selectedTimezone].name.c_str());
            } else {
                TDPRINT(this, "Failed to get time, retry in ~120s         \n");
                m_secondsUntilNextNTPUpdate = 110 + +rand() % 20;
            }
        }
    }

    bool GetLocalTime(struct tm* info, uint32_t ms) {
        uint32_t start = millis();
        time_t now;
        while ((millis() - start) <= ms) {
            time(&now);
            localtime_r(&now, info);
            if (info->tm_year > (2016 - 1900)) {
                return true;
            }
            delay(10);
        }
        return false;
    }

    struct NamedTimezone {
        String name;
        Timezone tz;
    };
    std::vector<NamedTimezone> m_timezones;

    void SetupTimezones() {
        TimeChangeRule nzDT = {"NZDT", week_t::First, Sun, Sep, 25, 780};
        TimeChangeRule nzST = {"NZST", week_t::First, Sun, Apr, 3, 720};
        TimeChangeRule aEDT = {"AEDT", week_t::First, Sun, Oct, 2, 660};
        TimeChangeRule aEST = {"AEST", week_t::First, Sun, Apr, 3, 600};
        TimeChangeRule aWDT = {"AWDT", week_t::First, Sun, Oct, 2, 540};
        TimeChangeRule aWST = {"AWST", week_t::First, Sun, Apr, 3, 480};
        TimeChangeRule msk = {"MSK", week_t::Last, Sun, Mar, 1, 180};
        TimeChangeRule CEST = {"CEST", week_t::Last, Sun, Mar, 27, 120};
        TimeChangeRule CET = {"CET ", week_t::Last, Sun, Oct, 30, 60};
        TimeChangeRule BST = {"BST", week_t::Last, Sun, Mar, 1, 60};
        TimeChangeRule GMT = {"GMT", week_t::Last, Sun, Oct, 2, 0};
        TimeChangeRule utcRule = {"UTC", week_t::Last, Sun, Mar, 1, 0};
        TimeChangeRule usEDT = {"EDT", week_t::Second, Sun, Mar, 12, -240};
        TimeChangeRule usEST = {"EST", First, Sun, Nov, 6, -300};
        TimeChangeRule usCDT = {"CDT", week_t::Second, Sun, Mar, 12, -300};
        TimeChangeRule usCST = {"CST", week_t::First, Sun, Nov, 6, -360};
        TimeChangeRule usMDT = {"MDT", week_t::Second, Sun, Mar, 12, -360};
        TimeChangeRule usMST = {"MST", week_t::First, Sun, Nov, 6, -420};
        TimeChangeRule usPDT = {"PDT", week_t::Second, Sun, Mar, 12, -420};
        TimeChangeRule usPST = {"PST", week_t::First, Sun, Nov, 6, -480};
        TimeChangeRule usAKDT = {"AKDT", week_t::Second, Sun, Mar, 12, -480};
        TimeChangeRule usAKST = {"AKST", week_t::First, Sun, Nov, 6, -540};
        TimeChangeRule usHST = {"HST", week_t::First, Sun, Nov, 6, -600};

        m_timezones.push_back(
            {"UTC +12 New Zealand Daylight Time (DST)", {nzST, nzDT}});
        m_timezones.push_back(
            {"UTC +10 Australia Eastern Time (DST)", {aEDT, aEST}});
        m_timezones.push_back(
            {"UTC +8 Australia Western Time (DST)", {aWDT, aWST}});
        m_timezones.push_back({"UTC +3 Moscow Standard Time (no DST)", {msk}});
        m_timezones.push_back(
            {"UTC +1 Central European Time (DST)", {CEST, CET}});
        m_timezones.push_back({"UTC 0 London (DST)", {BST, GMT}});
        m_timezones.push_back({"UTC 0", {utcRule}});
        m_timezones.push_back(
            {"UTC -4 Eastern Daylight Time (DST)", {usEDT, usEST}});
        m_timezones.push_back(
            {"UTC -5 Central Daylight Time (DST)", {usCDT, usCST}});
        m_timezones.push_back(
            {"UTC -6 Mountain Daylight Time (DST)", {usMDT, usMST}});
        m_timezones.push_back({"UTC -6 Arizona (no DST)", {usMST}});
        m_timezones.push_back(
            {"UTC -7 Pacific Daylight Time (DST)", {usPDT, usPST}});
        m_timezones.push_back(
            {"UTC -8 Alaska Daylight Time (DST)", {usAKDT, usAKST}});
        m_timezones.push_back(
            {"UTC -10 Hawaii Standard Time (no DST)", {usHST}});

        if (!(*m_settings).containsKey("TIMEZONE")) {
            (*m_settings)["TIMEZONE"] = 4;  // UTC
        }
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    }

    void AttachInterrupt() {
        pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                        FALLING);
    }
    static inline void IRAM_ATTR InterruptISR() { m_receivedInterrupt = true; }
};
