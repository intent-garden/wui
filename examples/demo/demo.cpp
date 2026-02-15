//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/framework/framework.hpp>

#include <wui/config/config.hpp>

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>

#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>

#include <MainFrame/MainFrame.h>

#include <Resource.h>

#ifdef _WIN32
#include <gdiplus.h>
#endif

#include <iostream>

#ifdef _WIN32
int APIENTRY wWinMain(_In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
#elif __linux__
int main(int argc, char *argv[])
#endif
{
    wui::framework::init();

    auto ok = wui::config::create_config("demo.ini", "Software\\wui\\demo");
    if (!ok)
    {
        std::cerr << wui::config::get_error().str() << std::endl;
        return -1;
    }

    wui::error err;

    wui::set_app_locales({
        { wui::locale_type::eng, "English", "res/en_locale.json", TXT_LOCALE_EN },
        { wui::locale_type::rus, "Русский", "res/ru_locale.json", TXT_LOCALE_RU },
        });

    auto current_locale = static_cast<wui::locale_type>(wui::config::get_int("User", "Locale",
        static_cast<int32_t>(wui::get_default_system_locale())));
    wui::set_current_app_locale(current_locale);
    
    wui::set_locale_from_type(current_locale, err);
    if (!err.is_ok())
    {
        std::cerr << err.str() << std::endl;
        return -1;
    }

    wui::set_app_themes({
        { "dark", "res/dark.json", TXT_DARK_THEME },
        { "light", "res/light.json", TXT_LIGHT_THEME }
        });

    auto current_theme = wui::config::get_string("User", "Theme", "dark");
    wui::set_current_app_theme(current_theme);
    
    wui::set_default_theme_from_name(current_theme, err);
    if (!err.is_ok())
    {
        std::cerr << err.str() << std::endl;
        return -1;
    }

    MainFrame mainFrame;
    mainFrame.Run();

    wui::framework::run();

    return 0;
}
