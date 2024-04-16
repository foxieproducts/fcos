#include <button.hpp>
#include <memory>
#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>

void ShowSerialStatusMessage(std::shared_ptr<Pixels> pixels,
                             std::shared_ptr<Rtc> rtc);

void DoHardwareStartupTests(std::shared_ptr<Pixels> pixels,
                            std::shared_ptr<Settings> settings,
                            std::shared_ptr<Rtc> rtc,
                            std::shared_ptr<Joystick> joy);
