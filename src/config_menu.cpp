#include <config_menu.hpp>

void ConfigMenu::Initialize() {
    AddMenuItems();
}

void ConfigMenu::Activate() {
    m_subDisplayActive = false;
}

void ConfigMenu::Update() {
    m_pixels->Darken();

#if FCOS_FOXIECLOCK
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (i == m_selected) {
            if (!m_subDisplayActive && m_items[i].animator &&
                m_timeSinceAnimation.Ms() >= m_items[i].animFreq) {
                m_timeSinceAnimation.Reset();
                m_items[i].animator(m_items[i]);
            }
            if (m_items[i].ledNum != LED_UNUSED) {
                m_pixels->Set(m_items[i].ledNum, m_items[i].color);
            }

            const size_t showSelected = m_selected + 1;
            m_pixels->DrawText(0 + (showSelected < 10 ? 20 : 0),
                               String(showSelected),
                               m_subDisplayActive ? WHITE : m_items[i].color);
            m_pixels->DrawChar(8, ':',
                               m_subDisplayActive ? PURPLE : DARK_PURPLE);
        } else {
            if (m_items[i].ledNum != LED_UNUSED) {
                // an unselected item with a config LED will be used as
                // general lighting for settings that are not selected
                m_pixels->Set(m_items[i].ledNum, EXTRA_DARK_GRAY);
            }
        }
    }
#elif FCOS_CARDCLOCK2
    if (!m_subDisplayActive) {
        String name = m_items[m_selected].display->m_name;
        m_pixels->DrawText(0, 3, name, LIGHT_GRAY);
        m_pixels->DrawChar(14, 0, CHAR_UP_ARROW, GREEN);
        m_pixels->DrawChar(14, 6, CHAR_DOWN_ARROW, GREEN);
    }
#endif
}

void ConfigMenu::Hide() {
    m_settings->Save();
}

void ConfigMenu::Up(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_selected++;
        if (m_selected >= m_items.size()) {
            m_selected = 0;
        }
#if FCOS_CARDCLOCK2
        for (size_t i = 0; i < 8; ++i) {
            m_pixels->MoveVertical(1);
            String name = m_items[m_selected].display->m_name;
            m_pixels->DrawText(0, -4 + i, name, LIGHT_GRAY);
            m_pixels->Show();
            ElapsedTime::Delay(SCROLL_DELAY_VERTICAL_MS);
        }
#endif
    }
}

void ConfigMenu::Down(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        if (m_selected == 0) {
            m_selected = m_items.size();
        }
        m_selected--;
#if FCOS_CARDCLOCK2
        for (size_t i = 0; i < 8; ++i) {
            m_pixels->MoveVertical(-1);
            String name = m_items[m_selected].display->m_name;
            m_pixels->DrawText(0, 10 - i, name, LIGHT_GRAY);
            m_pixels->Show();
            ElapsedTime::Delay(SCROLL_DELAY_VERTICAL_MS);
        }
#endif
    }
}

// holding left exits config menu
bool ConfigMenu::Left(const Button::Event_e evt) {
    if (evt == Button::LONG_PRESS) {
        m_settings->Save();
        return false;
    }
    return true;
}

// pressing right activates the selected item's Display
bool ConfigMenu::Right(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        if (m_items[m_selected].display) {
            GetManager()->ActivateTemporaryDisplay(m_items[m_selected].display);
            m_subDisplayActive = true;
        }
    }
    return true;
}

void ConfigMenu::Timeout() {}

void ConfigMenu::Add(const Item&& item) {
    m_items.push_back(item);
    if (item.display) {
        item.display->m_pixels = m_pixels;
        item.display->m_settings = m_settings;
        item.display->m_rtc = m_rtc;
        item.display->m_manager = m_manager;
        item.display->Initialize();
    }
}

void ConfigMenu::AddMenuItems() {
    Add({std::make_shared<Numeric>("ANIM", 1, ANIM_TOTAL), LED_OPT_ANIM, WHITE,
         [](Item& item) {
             // ANIM is the 8th option LED. ANIM-8, get it? #dadjokes
             static uint8_t wheelPos = 128;
             item.animFreq = 10;
             item.color = Pixels::ColorWheel(wheelPos++);
         }});

    Add({std::make_shared<Numeric>("MINB", 0, MAX_DISPLAY_BRIGHTNESS),
         LED_OPT_MINB, GRAY, [](Item& item) {
             item.color = item.color == LIGHT_GRAY ? WHITE : LIGHT_GRAY;
             item.animFreq = 500;
         }});

    std::vector<String> options = {"12", "24"};
    Add({std::make_shared<Numeric>("24HR", options), LED_OPT_24HR, YELLOW,
         [](Item& item) {
             item.color = item.color == YELLOW ? CYAN : YELLOW;
             item.animFreq = 500;
         }});
#if FCOS_FOXIECLOCK
    Add({std::make_shared<Numeric>("PXL", 0, 1), LED_OPT_PXL, PURPLE,
         [](Item& item) {
             item.color = item.color == PURPLE ? ORANGE : PURPLE;
             item.animFreq = 500;
         }});

    Add({/*"BT",*/ nullptr, LED_OPT_BT, BLUE, [](Item& item) {
             item.color = item.color == BLUE ? DARK_BLUE : BLUE;
             item.animFreq = 500;
         }});
#endif

    m_wifiConfig = std::make_shared<WiFiConfig>();
    Add({m_wifiConfig, LED_OPT_WIFI, GREEN, [](Item& item) {
             item.color = item.color == GREEN ? DARK_GREEN : GREEN;
             item.animFreq = 500;
         }});

    Add({std::make_shared<InfoDisplay>(), LED_OPT_INFO, ORANGE, [](Item& item) {
             item.color = item.color == ORANGE ? DARK_ORANGE : ORANGE;
             item.animFreq = 500;
         }});

    Add({std::make_shared<WebUpdate>(), LED_OPT_UPDT, BLUE, [](Item& item) {
             item.color = item.color == BLUE ? CYAN : BLUE;
             item.animFreq = 500;
         }});

    // flip the order of the list so that 0 == UPDT ... 7 == ANIM, 8+ =
    // custom options
    std::reverse(m_items.begin(), m_items.end());
    m_selected = m_items.size() - 1;
}
