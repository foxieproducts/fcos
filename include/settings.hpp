#pragma once
#include <ArduinoJson.h>
#include <LittleFS.h>

class Settings : public DynamicJsonDocument {
  private:
    enum {
        MAX_SETTINGS_SIZE = 32768,
    };

    String m_filename;
    bool m_loaded{false};

  public:
    Settings(const String filename = "/config.json");

    bool Load();

    bool Save(bool force = false);
    bool IsLoaded() { return m_loaded; }

  private:
    bool IsSameAsSavedFile();
};