#include <settings.hpp>

Settings::Settings(const String filename)
    : DynamicJsonDocument(MAX_SETTINGS_SIZE), m_filename(filename) {
#if FCOS_ESP32_C3
    LittleFS.begin(true);
#elif FCOS_ESP8266
    LittleFS.begin();
#endif

    Load();
}

bool Settings::Load() {
    File file = LittleFS.open(m_filename, "r");
    m_loaded = false;
    if (file.size() > MAX_SETTINGS_SIZE) {
        return false;
    }

    DeserializationError err = deserializeJson(*this, file);
    file.close();

    m_loaded = err ? false : true;
    return m_loaded;
}

bool Settings::Save(bool force) {
    if (IsSameAsSavedFile() && !force) {
        return true;
    }

    LittleFS.remove(m_filename);
    File file = LittleFS.open(m_filename, "w+");
    if (!file) {
        return false;
    }
    const bool success = serializeJson(*this, file) > 0;
    file.close();

    return success;
}

bool Settings::IsSameAsSavedFile() {
    File currentFile = LittleFS.open(m_filename, "r");
    if (currentFile.size() <= MAX_SETTINGS_SIZE) {
        DynamicJsonDocument currentDoc(MAX_SETTINGS_SIZE);
        DeserializationError err = deserializeJson(currentDoc, currentFile);
        currentFile.close();

        if (!err && currentDoc == *this) {
            return true;  // file is the same
        }
    }
    return false;  // file is different or non-existent/parsable
}
