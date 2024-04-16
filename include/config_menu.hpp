#pragma once
#include <vector>

#include <animators.hpp>
#include <button.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>

#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>

#include <options/info_display.hpp>
#include <options/numeric.hpp>
#include <options/web_update.hpp>
#include <options/wifi_config.hpp>

class ConfigMenu : public Display {
  private:
    enum {
        ANIM_DEFAULT_FREQ = 50,
    };

    struct Item {
        std::shared_ptr<Display> display;
        size_t ledNum;
        RgbColor color;
        std::function<void(Item& item)> animator;
        size_t animFreq;
    };
    std::vector<Item> m_items;
    size_t m_selected{0};
    bool m_subDisplayActive{false};

    ElapsedTime m_timeSinceAnimation;

    std::shared_ptr<WiFiConfig> m_wifiConfig;

  public:
    ConfigMenu() {}

    virtual void Initialize();
    virtual void Activate() override;
    virtual void Update() override;
    void Hide();

    virtual void Up(const Button::Event_e evt) override;
    virtual void Down(const Button::Event_e evt) override;
    virtual bool Left(const Button::Event_e evt) override;
    virtual bool Right(const Button::Event_e evt) override;
    void Timeout();

    void Add(const Item&& item);

  private:
    void AddMenuItems();
};
