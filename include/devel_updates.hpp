#pragma once
#include <ArduinoOTA.h>

#if FCOS_ESP32_C3
#include <ESPmDNS.h>
#include <WiFi.h>
#elif FCOS_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <elapsed_time.hpp>
#include <pixels.hpp>

/**
 * @brief This class handles local OTA updates from esptool. It is used
 *        whenever a local firmware update is initiated over WiFi from VS Code
 *        and allows development without the clock being directly wired via
 *        USB.
 *        The clock's UPDT (Web Update) menu option is different; it updates the
 *        firmware from github.com/foxieproducts/fcos/bin/fc2_<type>.bin,
 *        which is done in options/web_update.hpp
 */
class DevelUpdates {
  private:
    std::shared_ptr<Pixels> m_pixels;
    bool m_isInitialized{false};

  public:
    DevelUpdates(std::shared_ptr<Pixels> pixels) : m_pixels(pixels) {}

  public:
    void Update();

  private:
    void InitializeOTA();

    String GetUniqueMDNSName();
};
