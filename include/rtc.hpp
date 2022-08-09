#pragma once
#include <Rtc_Pcf8563.h>
#include <Time.h>
#include <Timezone.h>  // https://github.com/JChristensen/Timezone
#include <WiFi.h>
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
    size_t m_secondsUntilNextNTPUpdate{0};
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

    void ForceNTPUpdate() { m_secondsUntilNextNTPUpdate = 0; }

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
        ElapsedTime::Delay(5);

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
            if (getLocalTime(&m_timeinfo, MAX_WAIT_FOR_NTP_MS)) {
                m_secondsUntilNextNTPUpdate = (89 * 60) + rand() % 120;

                int selectedTimezone = (*m_settings)["TIMEZONE"].as<int>();
                auto local = m_timezones[selectedTimezone].tz.toLocal(
                    mktime(&m_timeinfo));
                m_timeinfo = *localtime(&local);

                SetTime(m_timeinfo.tm_hour, m_timeinfo.tm_min,
                        m_timeinfo.tm_sec);

                TDPRINT(
                    this,
                    "Got NTP time (%02d:%02d:%02d) (TZ: %s) next update in %d "
                    "seconds\n",
                    m_timeinfo.tm_hour, m_timeinfo.tm_min, m_timeinfo.tm_sec,
                    m_timezones[selectedTimezone].name.c_str(),
                    m_secondsUntilNextNTPUpdate);
            } else {
                TDPRINT(this, "Failed to get time, retry in 120s         \n");
                m_secondsUntilNextNTPUpdate = 120;
            }
        }
    }

    struct NamedTimezone {
        String name;
        Timezone tz;
    };
    std::vector<NamedTimezone> m_timezones;
    TimeChangeRule aEDT = {"AEDT", week_t::First, Sun, Oct, 2, 660};
    TimeChangeRule aEST = {"AEST", week_t::First, Sun, Apr, 3, 600};
    TimeChangeRule msk = {"MSK", week_t::Last, Sun, Mar, 1, 180};
    TimeChangeRule CEST = {"CEST", week_t::Last, Sun, Mar, 2, 120};
    TimeChangeRule CET = {"CET ", week_t::Last, Sun, Oct, 3, 60};
    TimeChangeRule BST = {"BST", week_t::Last, Sun, Mar, 1, 60};
    TimeChangeRule GMT = {"GMT", week_t::Last, Sun, Oct, 2, 0};
    TimeChangeRule utcRule = {"UTC", week_t::Last, Sun, Mar, 1, 0};
    TimeChangeRule usEDT = {"EDT", week_t::Second, Sun, Mar, 2, -240};
    TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};
    TimeChangeRule usCDT = {"CDT", week_t::Second, Sun, Mar, 2, -300};
    TimeChangeRule usCST = {"CST", week_t::First, Sun, Nov, 2, -360};
    TimeChangeRule usMDT = {"MDT", week_t::Second, Sun, Mar, 2, -360};
    TimeChangeRule usMST = {"MST", week_t::First, Sun, Nov, 2, -420};
    TimeChangeRule usPDT = {"PDT", week_t::Second, Sun, Mar, 2, -420};
    TimeChangeRule usPST = {"PST", week_t::First, Sun, Nov, 2, -480};
    void SetupTimezones() {
        m_timezones.push_back({"Australia Eastern Time Zone", {aEDT, aEST}});
        m_timezones.push_back({"Moscow Standard Time", {msk}});
        m_timezones.push_back({"Central European Time", {CEST, CET}});
        m_timezones.push_back({"London", {BST, GMT}});
        m_timezones.push_back({"UTC", {utcRule}});
        m_timezones.push_back({"Eastern Daylight Time", {usEDT, usEST}});
        m_timezones.push_back({"Central Daylight Time", {usCDT, usCST}});
        m_timezones.push_back({"Mountain Daylight Time", {usMDT, usMST}});
        m_timezones.push_back({"Arizona (no DST)", {usMST}});
        m_timezones.push_back({"Pacific Daylight Time", {usPDT, usPST}});

        if (!(*m_settings).containsKey("TIMEZONE")) {
            (*m_settings)["TIMEZONE"] = 4;  // UTC
        }
    }

    void AttachInterrupt() {
        pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                        FALLING);
    }
    static inline void IRAM_ATTR InterruptISR() { m_receivedInterrupt = true; }
};
