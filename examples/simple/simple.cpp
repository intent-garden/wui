// wui.cpp : Defines the entry point for the application.
//

#include <wui/framework/framework.hpp>
#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>
#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>
#include <wui/common/flag_helpers.hpp>
#include <wui/window/window.hpp>
#include <wui/control/button.hpp>
#include <wui/control/input.hpp>
#include <wui/control/menu.hpp>
#include <wui/control/list.hpp>
#include <wui/control/select.hpp>
#include <wui/control/image.hpp>
#include <wui/control/text.hpp>
#include <wui/control/message.hpp>
#include <wui/control/splitter.hpp>
#include <wui/control/progress.hpp>
#include <wui/control/slider.hpp>
#include <wui/control/panel.hpp>

#include <Resource.h>

#include <iostream>

std::shared_ptr<wui::i_theme> MakeRedButtonTheme()
{
    auto redButtonTheme = wui::make_custom_theme();

    redButtonTheme->load_theme(*wui::get_default_theme());

    redButtonTheme->set_color(wui::button::tc, wui::button::tv_calm, wui::make_color(205, 15, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_active, wui::make_color(235, 15, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_border, wui::make_color(200, 215, 200));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_focused_border, wui::make_color(20, 215, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_text, wui::make_color(190, 205, 190));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_disabled, wui::make_color(180, 190, 180));
    //redButtonTheme->set_string(wui::image::tc, wui::image::tv_path, "IMAGES_DARK");

    return redButtonTheme;
}

struct Item
{
    int id;
    bool expanded;
};

struct PluggedWindow : public std::enable_shared_from_this<PluggedWindow>
{
    std::weak_ptr<wui::window> parentWindow;

    std::shared_ptr<wui::window> window;
    std::shared_ptr<wui::text> text1;
    std::shared_ptr<wui::list> list;
    std::shared_ptr<wui::menu> popupMenu;
    std::shared_ptr<wui::panel> panel;
    std::shared_ptr<wui::button> button1, button2, button3;
    std::shared_ptr<wui::splitter> splitter;
    std::shared_ptr<wui::input> input;
    std::shared_ptr<wui::message> messageBox;
    std::shared_ptr<wui::window> dialog;

    bool plugged;

    int splitterPos;

    void Plug()
    {
        auto parentWindow_ = parentWindow.lock();
        if (parentWindow_)
        {
            parentWindow_->add_control(window, { 0, 30, 300, parentWindow_->position().height() });
        }

        plugged = !plugged;
    }

    void Unplug()
    {
        if (parentWindow.lock())
            parentWindow.lock()->remove_control(window);

        Init();
		
        plugged = !plugged;
    }

    void Init()
    {
        window->init("Child window plugged!", { 0 }, 
            static_cast<wui::window_style>(static_cast<uint32_t>(wui::window_style::pinned) | static_cast<uint32_t>(wui::window_style::border_right)),
            [this]() {
                if (parentWindow.lock())
                    parentWindow.lock()->emit_event(5555, 0);
            plugged = false;
        });
    }

    void SplitterChange(int32_t x, int32_t y)
    {
        auto p = window->position(); auto h = p.height(), w = p.width();

        splitterPos = h - y;

        list->set_position({ 10, 50, w - 10, y - 2 });
        panel->set_position({ 0, y + 10, w, h });
        button1->set_position({ 10, y + 15, 30, y + 35 });
        button2->set_position({ 40, y + 15, 60, y + 35 });
        button3->set_position({ 70, y + 15, 90, y + 35 });
        input->set_position({ 100, y + 15, w - 10, h - 10 });

        window->redraw({ 0, 0, p.right, p.bottom }, true);
    }

    PluggedWindow(std::shared_ptr<wui::window> &parentWindow_)
        : parentWindow(parentWindow_),
        window(std::make_shared<wui::window>()),
        text1(std::make_shared<wui::text>("Some text")),
        list(std::make_shared<wui::list>()),
        splitter(std::make_shared<wui::splitter>(wui::splitter_orientation::horizontal, [this](int32_t x, int32_t y) { SplitterChange(x, y); })),
        popupMenu(std::make_shared<wui::menu>()),
        panel(std::make_shared<wui::panel>()),
		button1(std::make_shared<wui::button>("Button 1", [this]() {
            messageBox->show("Lorem Ipsum is simply dummy text of the printing and typesetting industry.\nLorem Ipsum has been the industry's\nstandard dummy text ever since the 1500s, when an unknown printer took\na galley of type and scrambled it to make a type specimen book.",
                "hello world", wui::message_icon::information, wui::message_button::ok, [](wui::message_result) {});
        }, wui::button_view::image, IMG_ACCOUNT, 16)),
        button2(std::make_shared<wui::button>("Button 2", [this]() {
            window->emit_event(310, 200);
        }, wui::button_view::image, IMG_ACCOUNT, 16)),
        button3(std::make_shared<wui::button>("Button 3", [this]() {
            std::shared_ptr<wui::input> input1(std::make_shared<wui::input>());
            input1->set_text("98753");
            dialog->add_control(input1, { 10, 35, 200, 60 });

            std::shared_ptr<wui::input> input2(std::make_shared<wui::input>());
            input2->set_text("99wegdyug");
            dialog->add_control(input2, { 10, 70, 200, 95 });

            std::shared_ptr<wui::select> select1(std::make_shared<wui::select>());

            wui::select_items_t items = { { 1, "123" }, { 2, "456" }, { 3, "789" }, { 4, "101112" }, { 5, "131415" }, { 6, "161718" }, { 7, "192021" }, { 8, "222324" } };

            select1->set_items(items);

            dialog->add_control(select1, { 10, 130, 200, 155 });

            dialog->set_transient_for(window);
            
            dialog->init("Modal dialog", { 50, 50, 350, 350 }, wui::window_style::dialog, []() {});
        }, wui::button_view::image, IMG_ACCOUNT, 16)),
        input(std::make_shared<wui::input>("", wui::input_view::multiline)),
        messageBox(std::make_shared<wui::message>(parentWindow_, true)),
        dialog(std::make_shared<wui::window>()),
        plugged(false),
        splitterPos(50)
    {
        button1->disable_focusing();
        button2->disable_focusing();
        button3->disable_focusing();

        list->set_draw_callback(std::bind(&PluggedWindow::DrawListItem, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

        list->set_item_click_callback([this](wui::list::click_button btn, int32_t item, int32_t x, int32_t y) {
            if (btn == wui::list::click_button::right)
                popupMenu->show_on_point(x, y);
            else
            {
                static int count = list->get_item_count();
                if (item % 2 == 0)
                    list->set_item_count(count + 2);
                else
                    list->set_item_count(count - 2);
            }
        });

        list->update_columns({ { 30, "##" }, { 100, "Name" }, { 100, "Role" } });
        
        list->set_item_height_callback([](int32_t i, int32_t& h) { h = 32 + i * 2; });
        list->set_item_count(20);
        list->select_item(5);

        popupMenu->set_items({
            { 0, wui::menu_item_state::normal, "Bla bla bla", "", nullptr, {}, [](int32_t i) {} },
            { 1, wui::menu_item_state::separator, "Other", "", nullptr, {}, [](int32_t i) {} },
            { 2, wui::menu_item_state::normal, "Another", "", nullptr, {}, [](int32_t i) {} }
            });

        input->set_change_callback([this]() {
            const auto constAdd = 35;
            auto lines = input->get_lines().size();
            auto font = wui::theme_font(wui::input::tc, wui::input::tv_font);

            if (lines < 9)
            {
                auto wp = window->position();
                int32_t inputTop = wp.height() - constAdd - (lines * font.size);
                splitter->set_position({ 0, inputTop + 2, wp.right, inputTop + 8 });
                SplitterChange(0, inputTop);
            }
            else
            {
                auto ip = input->position();
                if (constAdd + lines * font.size > ip.height() && ip.height() < constAdd + 8 * font.size)
                {
                    if (lines > 8) lines = 8;
                    auto wp = window->position();
                    int32_t inputTop = wp.height() - constAdd - (lines * font.size);
                    splitter->set_position({ 0, inputTop + 2, wp.right, inputTop + 8 });
                    SplitterChange(0, inputTop);
                }
            }
            });

        window->add_control(popupMenu, { 0 });

        window->add_control(text1, { 0 });

        window->add_control(list, { 0 });
        window->add_control(splitter, { 0 });

        window->add_control(panel, { 0 });
        window->add_control(button1, { 0 });
        window->add_control(button2, { 0 });
        window->add_control(button3, { 0 });
        window->add_control(input, { 0 });

        window->set_control_callback([this](wui::window_control control, std::string &tooltip_text, bool continue_) {
            if (control != wui::window_control::pin)
            {
                return;
            }

            if (plugged)
            {
                Unplug();
                tooltip_text = wui::locale("window", "pin");
            }
            else
            {
                Plug();
                tooltip_text = wui::locale("window", "unpin");
            }
        });

        window->subscribe([this](const wui::event &e) {
            switch (e.type)
            {
            case wui::event_type::internal:
                if (e.internal_event_.type == wui::internal_event_type::size_changed)
                {
                    int32_t w = e.internal_event_.x, h = e.internal_event_.y;

                    auto wp = window->position();

                    auto y = wp.height() - splitterPos;

                    text1->set_position({ 10, 30, w - 10, 50 });

                    list->set_position({ 10, 50, w - 10, y - 10 });
                    splitter->set_position({ 10, y - 8, w - 10, y - 2 });
                    splitter->set_margins(50, h - 50);

                    panel->set_position({ 0, y, w, h });
                    button1->set_position({ 10, y + 5, 30, y + 25 });
                    button2->set_position({ 40, y + 5, 60, y + 25 });
                    button3->set_position({ 70, y + 5, 90, y + 25 });
                    input->set_position({ 100, y + 5, w - 10, h - 10 });
                }
                else if (e.internal_event_.type == wui::internal_event_type::user_emitted)
                {
                    /*int32_t x = e.internal_event_.x, y = e.internal_event_.y;

                    messageBox->show("user emitted event received, x: " + std::to_string(x) + ", y: " + std::to_string(y),
                        "user emitted event", wui::message_icon::information, wui::message_button::yes_no, [this](wui::message_result result) {
                            if (result == wui::message_result::yes)
                            {
                                dialog->set_transient_for(window);
                                dialog->init("Modal dialog", { 50, 50, 350, 350 }, wui::window_style::dialog, []() {});
                            }
                        });*/
                    list->make_selected_visible();
                }
            break;
            case wui::event_type::system:
                switch (e.system_event_.type)
                {
                    case wui::system_event_type::device_connected:
#ifdef _WIN32
                        OutputDebugStringA("connect device: ");
                        OutputDebugStringA(to_string(e.system_event_.device).data());
                        OutputDebugStringA("\n");
#elif __linux__
                        printf("connect device: %s\n", to_string(e.system_event_.device).data());
#endif
                    break;
                    case wui::system_event_type::device_disconnected:
#ifdef _WIN32
                        OutputDebugStringA("disconnect device: ");
                        OutputDebugStringA(to_string(e.system_event_.device).data());
                        OutputDebugStringA("\n");
#elif __linux__
                        printf("disconnect device: %s\n", to_string(e.system_event_.device).data());
#endif
                    break;
                }
            break;
            }
        }, wui::flags_map<wui::event_type>(2, wui::event_type::internal, wui::event_type::system));

        Plug();
        Init();
    }

    void DrawListItem(wui::graphic &gr, int32_t nItem, wui::rect itemRect, wui::list::item_state state)
    {
        auto border_width = wui::theme_dimension(wui::list::tc, wui::list::tv_border_width);

        if (state == wui::list::item_state::active)
        {
            gr.draw_rect(itemRect, wui::theme_color(wui::list::tc, wui::list::tv_active_item));
        }
        else if (state == wui::list::item_state::selected)
        {
            gr.draw_rect(itemRect, wui::theme_color(wui::list::tc, wui::list::tv_selected_item));
        }

        auto textColor = wui::theme_color(wui::input::tc, wui::input::tv_text);
        auto font = wui::theme_font(wui::list::tc, wui::list::tv_font);
        
        auto textRect = itemRect;
        
        textRect.move(20, (itemRect.height() - font.size) / 2);
        gr.draw_text(textRect, "Item " + std::to_string(nItem), textColor, font);
    }
};

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
    wui::error err;

    wui::set_app_locales({
        { wui::locale_type::eng, "English", "res/en_locale.json", TXT_LOCALE_EN },
        { wui::locale_type::rus, "Русский", "res/ru_locale.json", TXT_LOCALE_RU },
        });

    auto current_locale = wui::get_default_system_locale();
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

    auto current_theme = "dark";
    wui::set_current_app_theme(current_theme);
    wui::set_default_theme_from_name(current_theme, err);
    if (!err.is_ok())
    {
        std::cerr << err.str() << std::endl;
        return -1;
    }

    auto window = std::make_shared<wui::window>();

    auto menuImage1 = std::make_shared<wui::image>(IMG_ACCOUNT);
    auto menuImage2 = std::make_shared<wui::image>(IMG_SETTINGS);

    auto menu = std::make_shared<wui::menu>();
    menu->set_items({
            { 0, wui::menu_item_state::separator, "Bla bla bla", "", menuImage1, {}, [](int32_t i) {} },
            { 1, wui::menu_item_state::normal, "Expand me 1", "", nullptr, {
                    { 11, wui::menu_item_state::normal, "Expanded 1.1", "", nullptr, {}, [](int32_t i) {} },
                    { 12, wui::menu_item_state::normal, "Expanded 1.2", "", nullptr, {
                            { 121, wui::menu_item_state::normal, "Expanded 1.1.1", "", nullptr, {}, [](int32_t i) {} },
                            { 122, wui::menu_item_state::normal, "Expanded 1.1.2", "Shift+Del", menuImage2, {}, [](int32_t i) {} },
                            { 123, wui::menu_item_state::separator, "Expanded 1.1.3", "", nullptr, {}, [](int32_t i) {} },
                        }, [](int32_t i) {} },
                    { 13, wui::menu_item_state::normal, "Expanded 1.3", "", nullptr, {}, [](int32_t i) {} },
                }, [](int32_t i) {} },
            { 2, wui::menu_item_state::separator, "Expand me 2", "Ctrl+Z", nullptr, {
                    { 21, wui::menu_item_state::normal, "Expanded 2.1", "", nullptr, {}, [](int32_t i) {} },
                    { 22, wui::menu_item_state::normal, "Expanded 2.2", "", nullptr, {}, [](int32_t i) {} },
                    { 23, wui::menu_item_state::separator, "Expanded 2.3", "", nullptr, {}, [](int32_t i) {} },
                }, [](int32_t i) {} },
            { 3, wui::menu_item_state::normal, "Exit", "Alt+F4", nullptr, {}, [&window](int32_t i) { window->destroy(); } }
        });

    window->add_control(menu, { 0 });

    auto menuButton = std::make_shared<wui::button>("Settings", []() {}, wui::button_view::image, IMG_SETTINGS, 32, wui::button::tc_tool);
    menuButton->set_callback([&menu, &menuButton]() { menu->show_on_control(menuButton, 5); });
    menuButton->disable_focusing();
    window->add_control(menuButton, { 0 });

    auto horizProgressBar = std::make_shared<wui::progress>(0, 100, 50);
    window->add_control(horizProgressBar, { 450, 100, 650, 125 });

    auto horizSlider = std::make_shared<wui::slider>(0, 100, 50, [&horizProgressBar](int32_t value) { horizProgressBar->set_value(value); }, wui::slider_orientation::horizontal);
    window->add_control(horizSlider, { 450, 140, 650, 165 });

    auto vertProgressBar = std::make_shared<wui::progress>(0, 100, 80, wui::orientation::vertical);
    window->add_control(vertProgressBar, { 700, 30, 725, 125 });

    auto vertSlider = std::make_shared<wui::slider>(0, 100, 80, [&vertProgressBar](int32_t value) { vertProgressBar->set_value(value); }, wui::slider_orientation::vertical);
    window->add_control(vertSlider, { 660, 30, 685, 155 });

    auto accountImage = std::make_shared<wui::image>(IMG_ACCOUNT);
    window->add_control(accountImage, { 350, 100, 414, 164 });

    auto pluggedWindow = std::make_shared<PluggedWindow>(window);
    auto createPluggedButton = std::make_shared<wui::button>("Create plugged window", []() {});
    createPluggedButton->set_callback([&window, &pluggedWindow, &createPluggedButton]() {
        pluggedWindow.reset();
        pluggedWindow = std::make_shared<PluggedWindow>(window);
        createPluggedButton->disable(); });
    createPluggedButton->disable();
    
    window->add_control(createPluggedButton, { 320, 50, 340, 75 });

    //auto text0 = std::make_shared<wui::text>("Lorem Ipsum is simply dummy text of the printing and typesetting industry.\nLorem Ipsum has been the industry's\nstandard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.");
    auto text0 = std::make_shared<wui::text>("Высокий уровень вовлечения представителей целевой аудитории является четким доказательством простого факта: граница обучения кадров создаёт предпосылки для новых предложений. Однозначно, непосредственные участники технического прогресса, превозмогая сложившуюся непростую экономическую ситуацию, превращены в посмешище, хотя само их существование приносит несомненную пользу обществу. А ещё базовые сценарии поведения пользователей, превозмогая сложившуюся непростую экономическую ситуацию, ограничены исключительно образом.");
    window->add_control(text0, { 320, 180, 890, 240 });
    
    auto nameInput = std::make_shared<wui::input>(/*"", wui::input_view::password*/);
    nameInput->set_text("Hello world!");
    //nameInput->set_input_content(wui::input_content::numeric);
    //nameInput->set_symbols_limit(20);
    //nameInput->set_input_view(wui::input_view::readonly);
    window->add_control(nameInput, { 320, 250, 890, 275 });

    auto someSelect = std::make_shared<wui::select>();
    someSelect->set_items({
            { 0, "Item 0" },
            { 1, "Item 1" },
            { 2, "Item 2" },
            { 3, "Item 3" },
            { 4, "Item 4" },
            { 5, "Item 5" }
        });
    window->add_control(someSelect, { 320, 300, 890, 325 });
    /*std::thread t([someSelect, window]() {
        bool has = false;
        while (true)
        {
            if (!has)
            {
                window->add_control(someSelect, { 320, 300, 890, 325 });
            }
            else
            {
                window->remove_control(someSelect);
            }
            has = !has;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    t.detach();*/

    auto memo = std::make_shared<wui::input>("", wui::input_view::multiline);
    memo->set_symbols_limit(-1);
    window->add_control(memo, { 320, 400, 890, 500 });

    auto dialog = std::make_shared<wui::window>();

    auto messageBox = std::make_shared<wui::message>(dialog);

    auto okButton = std::make_shared<wui::button>("OK", [&messageBox, &window, &dialog]()
    {
        auto text1 = std::make_shared<wui::text>("Your name:");
        dialog->add_control(text1, { 10, 35, 200, 55 });

        auto input1 = std::make_shared<wui::input>();
        dialog->add_control(input1, { 10, 60, 200, 85 });

        auto text2 = std::make_shared<wui::text>("Your Phone:");
        dialog->add_control(text2, { 10, 100, 200, 120 });

        auto input2 = std::make_shared<wui::input>();
        dialog->add_control(input2, { 10, 125, 200, 150 });

        auto select1 = std::make_shared<wui::select>();

        auto list1 = std::make_shared<wui::list>();

        wui::select_items_t items = { { 1, "123" }, { 2, "456" }, { 3, "789" }, { 4, "101112" }, { 5, "131415" }, { 6, "161718" }, { 7, "192021" }, { 8, "222324" } };

        select1->set_items(items);

        dialog->add_control(select1, { 10, 160, 200, 185 });

        dialog->add_control(list1, { 10, 245, 200, 400 });

        auto dialogMsgButton = std::make_shared<wui::button>("Test message", [&]() {
                messageBox->show("Test message",
                "Test title", wui::message_icon::information, wui::message_button::ok);
            });
        dialog->add_control(dialogMsgButton, { 10, 410, 100, 435 });

        auto dialogCloseButton = std::make_shared<wui::button>("Close", [&dialog]() { dialog->destroy(); });
        dialog->add_control(dialogCloseButton, { 110, 410, 210, 435 });
        dialog->set_default_push_control(dialogCloseButton);

        dialog->set_transient_for(window);
        dialog->init("Modal dialog", { -1, -1, 350, 550 }, wui::window_style::dialog, [&dialog]() { /*dialog.reset();*/ });
    });

    auto cancelButton = std::make_shared<wui::button>("Cancel", [window]() { window->destroy(); }, wui::button_view::image_right_text, IMG_ACCOUNT, 24, wui::button::tc, MakeRedButtonTheme());

    auto darkThemeButton = std::make_shared<wui::button>("Set the dark theme",
        [&window, &pluggedWindow, &dialog, &cancelButton]() {
        auto current_theme = "dark";
        wui::set_current_app_theme(current_theme);
        wui::error err;
        wui::set_default_theme_from_name(current_theme, err);
        if (!err.is_ok())
        {
            std::cerr << err.str() << std::endl;
            return;
        }

        window->update_theme();
        pluggedWindow->window->update_theme();
        dialog->update_theme(); 
        cancelButton->update_theme(MakeRedButtonTheme());
    });
    window->add_control(darkThemeButton, { 320, 350, 440, 375 });

    darkThemeButton->turn(true);
	
    auto whiteThemeButton = std::make_shared<wui::button>("Set the light theme",
        [&window, &pluggedWindow, &dialog, &cancelButton]() {
        auto current_theme = "light";
        wui::set_current_app_theme(current_theme);
        wui::error err;
        wui::set_default_theme_from_name(current_theme, err);
        if (!err.is_ok())
        {
            std::cerr << err.str() << std::endl;
            return;
        }

        window->update_theme();
        pluggedWindow->window->update_theme();
        dialog->update_theme();
        cancelButton->update_theme(MakeRedButtonTheme());
    });
    window->add_control(whiteThemeButton, { 500, 350, 620, 375 });

    window->add_control(okButton, { 240, 450, 350, 480 });
    window->add_control(cancelButton, { 370, 450, 480, 480 });

    window->set_min_size(100, 100);

    auto vertSplitter = std::make_shared<wui::splitter>(wui::splitter_orientation::vertical, [&](int32_t x, int32_t y) {
        if (pluggedWindow->plugged)
        {
            auto pos = pluggedWindow->window->position();
            pluggedWindow->window->set_position({ 0, pos.top, x, pos.bottom });
        }

        auto pos = createPluggedButton->position();
        createPluggedButton->set_position({ x + 20, pos.top, x + 250, pos.bottom });

        pos = horizProgressBar->position();
        horizProgressBar->set_position({ x + 100, pos.top, pos.right, pos.bottom });

        pos = accountImage->position();
        accountImage->set_position({ x + 20, pos.top, x + 20 + 64, pos.bottom });

        pos = text0->position();
        text0->set_position({ x + 20, pos.top, pos.right, pos.bottom });

        pos = nameInput->position();
        nameInput->set_position({ x + 20, pos.top, x + 200, pos.bottom });

        pos = someSelect->position();
        someSelect->set_position({ x + 20, pos.top, pos.right, pos.bottom });

        pos = darkThemeButton->position();
        darkThemeButton->set_position({ x + 20, pos.top, x + 200, pos.bottom });

        pos = whiteThemeButton->position();
        whiteThemeButton->set_position({ x + 250, pos.top, x + 430, pos.bottom });

        pos = memo->position();
        memo->set_position({ x + 20, pos.top, pos.right, pos.bottom });

        pos = horizProgressBar->position();
        horizProgressBar->set_position({x + 100, pos.top, x + 250, pos.bottom});

        pos = horizSlider->position();
        horizSlider->set_position({ x + 120, pos.top, x + 230, pos.bottom });
            
        pos = vertSlider->position();
        vertSlider->set_position({ x + 280, pos.top, x + 310, pos.bottom });

        pos = vertProgressBar->position();
        vertProgressBar->set_position({ x + 320, pos.top, x + 350, pos.bottom });
        
        auto wp = window->position();
        window->redraw({ 0, 0, wp.right, wp.bottom }, true);
        });
    window->add_control(vertSplitter, { 0 });

    auto sid = window->subscribe([&](const wui::event &e) {
        if (e.internal_event_.type == wui::internal_event_type::size_changed ||
            e.internal_event_.type == wui::internal_event_type::window_expanded)
        {
            int32_t w = e.internal_event_.x, h = e.internal_event_.y;

            if (pluggedWindow->plugged)
            {
                auto pos = pluggedWindow->window->position();
                if (pos.height() != h)
                {
                    pluggedWindow->window->set_position({ 0, 30, pos.width(), h });
                    vertSplitter->set_position({ pos.width(), 30, pos.width() + 5, h });
                }
            }

            vertSplitter->set_margins(130, w - 100);

            menuButton->set_position({ w - 42, 50, w - 10, 82 });
            text0->set_position({ 320, 180, w - 10, 240 });
            nameInput->set_position({ 320, 250, w - 10, 275 });
            someSelect->set_position({ 320, 300, w - 10, 325 });
            memo->set_position({ 320, 400, w - 10, h - 60 });
            okButton->set_position({ w - 250, h - 55, w - 150, h - 20 });
            cancelButton->set_position({ w - 120, h - 55, w - 20, h - 20 });
        }
        else if (e.internal_event_.type == wui::internal_event_type::window_created)
        {
            wui::init_text_measurer(window->get_graphic());
        }
        else if (e.internal_event_.type == wui::internal_event_type::user_emitted)
        {
            if (e.internal_event_.x == 5555)
                createPluggedButton->enable();
        }
    }, wui::event_type::internal);

    window->set_control_callback([&](wui::window_control control, std::string &tooltip_text, bool continue_) {
        if (control == wui::window_control::theme)
        {
            auto theme_name = wui::get_default_theme()->get_name();

            tooltip_text = wui::locale("window", theme_name == "dark" ? "dark_theme" : "light_theme");

            auto current_theme = theme_name == "dark" ? "light" : "dark";
            wui::set_current_app_theme(current_theme);
            wui::error err;
            wui::set_default_theme_from_name(current_theme, err);
            if (!err.is_ok())
            {
                std::cerr << err.str() << std::endl;
            }

            window->update_theme();
            pluggedWindow->window->update_theme();
            dialog->update_theme();
            cancelButton->update_theme(MakeRedButtonTheme());
        }
    });

    window->set_default_push_control(okButton);

    window->init("Hello from WUI!", { -1, -1, 900, 600 },
        wui::flags_map<wui::window_style>(3, wui::window_style::frame, wui::window_style::switch_theme_button, wui::window_style::border_all),
        []() {
            wui::framework::stop();
        });

    window->enable_device_change_handling(true);

    memo->set_text("Многострочный редактор\n\n"
        "Мы вынуждены отталкиваться от того, что дальнейшее развитие различных \n"
        "форм деятельности выявляет срочную потребность вывода текущих активов.\n"
        "Сложно сказать, почему элементы политического процесса набирают популярность\n"
        "среди определенных слоев населения, а значит, должны быть объединены в целые кластеры себе подобных.\n\n"
        "Но акционеры крупнейших компаний лишь добавляют фракционных разногласий и заблокированы в рамках\n"
        "своих собственных рациональных ограничений.");

    memo->scroll_to_end();

    wui::framework::run();

    return 0;
}
