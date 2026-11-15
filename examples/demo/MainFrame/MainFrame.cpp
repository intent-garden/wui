// Copyright (c) 2021-2026 Intent Garden Org
// Distributed under the Boost Software License, Version 1.0.
#include <MainFrame/MainFrame.h>
#include <Resource.h>
#include <wui/framework/framework.hpp>
#include <wui/common/flag_helpers.hpp>
#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>
#include <wui/config/config.hpp>
#include <wui/control/input.hpp>
#include <wui/control/image.hpp>
#include <wui/control/list.hpp>
#include <wui/control/select.hpp>
#include <wui/control/menu.hpp>
#include <wui/control/panel.hpp>
#include <wui/control/slider.hpp>
#include <wui/control/progress.hpp>
#include <wui/control/splitter.hpp>
#include <wui/control/scroll.hpp>
#include <wui/control/tooltip.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>

using namespace wui;

std::shared_ptr<text> MainFrame::Label(const std::string& value, rect r, const char* theme)
{
    return Add(std::make_shared<text>(value, hori_alignment::left, vert_alignment::center, theme), r);
}
std::shared_ptr<button> MainFrame::Button(const std::string& value, rect r, std::function<void()> callback, button_view view)
{
    return Add(std::make_shared<button>(value, std::move(callback), view, view == button_view::text ? "button" : "plain_button"), r);
}
void MainFrame::Card(rect r) { Add(std::make_shared<panel>(), r); }
void MainFrame::Heading(const char* title, const char* subtitle)
{
    Label(title, {24, 86, 776, 124}, "h1_text");
    Label(subtitle, {24, 128, 776, 155}, "muted_text");
}
void MainFrame::Notify(const std::string& value) { status->set_text(value); }

MainFrame::MainFrame()
{
    const char* names[] = {"Overview", "Windows", "Buttons", "Inputs", "Lists", "Menus", "Layout"};
    for (int i = 0; i < 7; ++i)
        tabs.push_back(Button(names[i], {24+i*108, 40, 126+i*108, 74}, [this,i] { ShowPage(i); }, button_view::sheet));
    status = Label("Ready / Explore the tabs. Every control is interactive.", {24, 564, 776, 590}, "muted_text");
    buildingPage = 0; Overview();
    buildingPage = 1; Windows();
    buildingPage = 2; Buttons();
    buildingPage = 3; Inputs();
    buildingPage = 4; Lists();
    buildingPage = 5; Menus();
    buildingPage = 6; Layouts();
    this->window->subscribe([this](const event& ev) {
        if (ev.internal_event_.type == internal_event_type::size_changed ||
            ev.internal_event_.type == internal_event_type::window_expanded ||
            ev.internal_event_.type == internal_event_type::window_normalized) Layout();
    }, event_type::internal);
}
MainFrame::~MainFrame()
{
    if (child) child->destroy();
    message.reset();
    for (auto& item : items) this->window->remove_control(item.control);
}
void MainFrame::Run()
{
    this->window->set_min_size(800, 600);
    this->window->set_control_callback([this](window_control control, std::string&, bool&) {
        if (control != window_control::theme) return;
        auto next = get_next_app_theme();
        error err;
        if (set_default_theme_from_name(next, err)) {
            config::set_string("User", "Theme", next);
            this->window->update_theme();
            if (child) child->update_theme();
            Notify("Theme / " + next);
        }
    });
    ShowPage(0);
    this->window->init("wui / Showcase", {-1, -1, 800, 600},
        flags_map<window_style>(3, window_style::frame, window_style::switch_theme_button, window_style::border_all),
        [] { framework::stop(); });
    Layout();
}
void MainFrame::ShowPage(int next)
{
    page = next;
    for (auto& item : items) {
        if (item.page >= 0 && item.page != page && !item.control->parent().expired())
            this->window->remove_control(item.control);
    }
    for (auto& item : items) {
        if (item.page == page && item.control->parent().expired()) {
            this->window->add_control(item.control, item.bounds);
            item.control->update_theme();
        }
    }
    // Popups are explicitly opened by their activation controls, never by navigation.
    for (auto& item : items)
        if (std::dynamic_pointer_cast<menu>(item.control) || std::dynamic_pointer_cast<tooltip>(item.control)) item.control->hide();
    for (int i = 0; i < 7; ++i) tabs[i]->turn(i == page);
    Notify("Ready / Changes stay with each tab. Use Tab to move between controls.");
    Layout();
}
void MainFrame::Layout()
{
    const int w = std::max(800, this->window->position().width());
    const int h = std::max(600, this->window->position().height());
    for (auto& item : items) {
        auto r = item.bounds;
        // Expand the canvas while retaining comfortable font and control sizes.
        r.left = 24 + (r.left-24)*(w-48)/752;
        r.right = 24 + (r.right-24)*(w-48)/752;
        if (item.control == status) { r.top = h-36; r.bottom = h-10; }
        item.control->set_position(r);
    }
    if (layoutDetail) layoutDetail();
    this->window->redraw({0, 0, w, h}, true);
}

void MainFrame::Overview()
{
    Heading("Small library. Real interfaces.", "C++17 / Windows / Linux / macOS / WebAssembly");
    Card({24, 178, 484, 540}); Card({500, 178, 776, 540});
    Label("SIGNAL LAB", {44, 190, 440, 218}, "accent_text");
    Label("Shape the waveform", {44, 223, 460, 253}, "section_text");
    auto amplitude = std::make_shared<int>(60);
    auto mode = std::make_shared<int>(0);
    // The drawing callback uses its own panel bounds, so the graph follows resizing.
    auto chart = std::make_shared<panel>([this, amplitude, mode](graphic& gr) {
        const auto w = std::max(800, this->window->position().width());
        const int left = 24 + 20*(w-48)/752, right = 24 + 436*(w-48)/752;
        auto grid = theme_color("input", "border");
        for (int y = 280; y <= 384; y += 26) gr.draw_line({left, y, right, y}, grid);
        const auto accent = theme_color("accent_text", "color");
        int prev = 332;
        for (int x = left; x < right; ++x) {
            const double phase = (x-left)*18.85/std::max(1,right-left);
            const double wave = *mode == 1 ? (std::sin(phase) >= 0 ? 1 : -1) : std::sin(phase);
            const int y = 332 - int(wave * *amplitude * .48);
            if (x > left) gr.draw_line({x-1,prev,x,y}, accent, 2);
            prev = y;
        }
    });
    Add(chart, {44, 267, 460, 400});
    auto value = Label("Amplitude / 60%", {44, 410, 270, 438});
    auto meter = Add(std::make_shared<progress>(0,100,60), {44, 495, 460, 509});
    Add(std::make_shared<slider>(0,100,60,[this,amplitude,value,meter,chart](int n) {
        *amplitude=n; value->set_text("Amplitude / " + std::to_string(n) + "%");
        meter->set_value(n); this->window->redraw(chart->position());
    }), {44, 450, 460, 478});
    Label("TRY IT", {520, 190, 756, 218}, "accent_text");
    Label("A shared C++ UI", {520, 223, 756, 253}, "section_text");
    Label("Move the slider. Pick a signal.\nSwitch the theme in the title bar.\n\nThese are WUI controls,\nincluding the custom graph.", {520, 269, 756, 378});
    auto choice = Add(std::make_shared<wui::select>(), {520, 398, 756, 432});
    choice->set_items({{0,"Sine wave"},{1,"Square wave"}}); choice->select_item_number(0);
    choice->set_change_callback([this,mode,chart](int, int64_t id) { *mode=int(id); this->window->redraw(chart->position()); });
    Button("Explore input controls", {520, 450, 756, 486}, [this] { ShowPage(3); });
    Label("Built with WUI controls.", {520, 500, 756, 524}, "muted_text");
}

void MainFrame::Dialog(message_icon icon, message_button buttons)
{
    message = std::make_unique<wui::message>(window);
    message->show("This dialog is drawn by WUI.\nChoose an action to update the status bar.", "Showcase / Dialog", icon, buttons,
        [this](message_result result) {
            const char* names[] = {"Closed", "OK", "Cancel", "Yes", "No", "Abort", "Retry", "Ignore", "Try", "Continue"};
            Notify(std::string("Dialog / ") + names[static_cast<int>(result)]);
        });
}
void MainFrame::Windows()
{
    Heading("Windows with a little personality", "Move, resize, switch themes and open a real child window.");
    Card({24,178,390,540}); Card({406,178,776,540});
    Label("DIALOGS", {44,194,360,222}, "accent_text");
    Label("An answer, not just an alert", {44,234,370,266}, "section_text");
    Button("Information / OK", {44,292,370,330}, [this] { Dialog(message_icon::information,message_button::ok); });
    Button("Question / Yes, No, Cancel", {44,348,370,386}, [this] { Dialog(message_icon::question,message_button::yes_no_cancel); });
    Button("Warning / Retry, Cancel", {44,404,370,442}, [this] { Dialog(message_icon::alert,message_button::retry_cancel); });
    Button("Error / Abort, Retry, Ignore", {44,460,370,498}, [this] { Dialog(message_icon::stop,message_button::abort_retry_ignore); });
    Label("CHILD WINDOW", {426,194,756,222}, "accent_text");
    Label("A separate workspace", {426,234,756,266}, "section_text");
    Label("The same window API on desktop\nand in the browser.\n\nOpen the scratchpad, edit its text,\nthen move or close it.", {426,284,756,398});
    Button("Open scratchpad", {426,422,756,460}, [this] {
        if (child) child->destroy();
        child = std::make_shared<wui::window>();
        child->set_transient_for(window, false);
        child->add_control(std::make_shared<input>("A small workspace of your own.\n\nHello, world! Привет, мир!", input_view::multiline), {20,50,400,220});
        child->set_min_size(420,250);
        child->init("Scratchpad", {-1,-1,420,250}, flags_map<window_style>(2,window_style::frame,window_style::border_all), [this] { Notify("Window / Scratchpad closed"); });
        Notify("Window / Scratchpad opened");
    });
    Label("Tray icons belong to desktop apps;\nthe browser has no system tray.", {426,478,756,523}, "muted_text");
}
void MainFrame::Buttons()
{
    Heading("One button, nine personalities", "Text, images, links, toggles, checkboxes, radio buttons and tabs.");
    Card({24,178,484,540}); Card({500,178,776,540});
    Label("ACTIONS & IMAGES", {44,194,460,222}, "accent_text");
    auto clicks = std::make_shared<int>(0);
    auto count = Label("0 clicks", {520,234,756,274}, "h1_text");
    Button("Count a click", {44,242,234,280}, [clicks,count] { count->set_text(std::to_string(++*clicks)+" clicks"); });
    auto disabled = Button("Disabled", {254,242,460,280}, [] {}); disabled->disable();
    Add(std::make_shared<button>("Logo", [this] { Notify("Button / Image only"); }, button_view::image, IMG_LOGO, 32), {44,306,94,356});
    Add(std::make_shared<button>("Image + label", [this] { Notify("Button / Image with text"); }, button_view::image_right_text, IMG_LOGO, 32), {110,306,300,356});
    Add(std::make_shared<button>("Below", [this] { Notify("Button / Label below image"); }, button_view::image_bottom_text, IMG_LOGO, 32), {320,306,460,396});
    Button("An anchor action", {44,380,270,414}, [this] { Notify("Anchor / Links can invoke any callback"); }, button_view::anchor);
    auto first = Button("Preview", {44,458,234,494}, [] {}, button_view::sheet);
    auto second = Button("Source", {254,458,460,494}, [] {}, button_view::sheet);
    first->turn(true);
    // Weak captures prevent controls retaining themselves through callbacks.
    std::weak_ptr<button> a=first,b=second;
    first->set_callback([this,a,b] { a.lock()->turn(true); b.lock()->turn(false); Notify("Tab / Preview selected"); });
    second->set_callback([this,a,b] { a.lock()->turn(false); b.lock()->turn(true); Notify("Tab / Source selected"); });
    Label("STATE & FEEDBACK", {520,194,756,222}, "accent_text");
    auto toggle = Button("Enable action", {520,302,756,336}, [] {}, button_view::switcher);
    toggle->set_callback([toggle=std::weak_ptr<button>(toggle),disabled] { if(toggle.lock()->turned()) disabled->enable(); else disabled->disable(); });
    disabled->set_callback([this] { Notify("Button / Previously disabled action activated"); });
    auto check = Button("Remember choice", {520,350,756,384}, [] {}, button_view::checkbox);
    check->set_callback([this,check=std::weak_ptr<button>(check)] { Notify(check.lock()->turned() ? "Checkbox / Checked" : "Checkbox / Unchecked"); });
    auto radio1 = Button("Compact", {520,400,756,434}, [] {}, button_view::radio);
    auto radio2 = Button("Comfortable", {520,446,756,480}, [] {}, button_view::radio);
    radio1->turn(true);
    a=radio1; b=radio2;
    radio1->set_callback([this,a,b] { a.lock()->turn(true); b.lock()->turn(false); Notify("Density / Compact"); });
    radio2->set_callback([this,a,b] { a.lock()->turn(false); b.lock()->turn(true); Notify("Density / Comfortable"); });
    Label("Try the keyboard, too: Tab + Space.", {520,484,756,515}, "muted_text");
}
void MainFrame::Inputs()
{
    Heading("Text editing, beyond hello world", "Selection, Unicode, passwords, content filters and a multiline scratchpad.");
    Card({24,178,390,540}); Card({406,178,776,540});
    Label("PROFILE", {44,192,370,220}, "accent_text");
    Label("Display name / single line", {44,224,370,244}, "muted_text");
    auto name = Add(std::make_shared<input>("Ada Lovelace"), {44,248,370,280});
    name->set_change_callback([this,name=std::weak_ptr<input>(name)] { Notify("Name / " + name.lock()->text()); });
    Label("Password", {44,288,220,308}, "muted_text");
    auto password = Add(std::make_shared<input>("wui-password",input_view::password), {44,312,246,344});
    auto reveal = Button("Show", {254,312,370,344}, [] {}, button_view::switcher);
    reveal->set_callback([password,reveal=std::weak_ptr<button>(reveal)] { password->set_input_view(reveal.lock()->turned()?input_view::singleline:input_view::password); });
    Label("Integer", {44,352,190,372}, "muted_text");
    Label("Hexadecimal", {210,352,370,372}, "muted_text");
    Add(std::make_shared<input>("42",input_view::singleline,input_content::integer,8), {44,376,194,408});
    Add(std::make_shared<input>("FACADE",input_view::singleline,input_content::hexadecimal,8), {210,376,370,408});
    Label("Numeric", {44,416,194,436}, "muted_text");
    Label("Host / port", {210,416,370,436}, "muted_text");
    Add(std::make_shared<input>("3.14159",input_view::singleline,input_content::numeric,12), {44,440,194,472});
    Add(std::make_shared<input>("127.0.0.1:8080",input_view::singleline,input_content::hostport,64), {210,440,370,472});
    Add(std::make_shared<input>("Read-only / select and copy me",input_view::readonly), {44,488,370,522});
    Label("SCRATCHPAD", {426,192,756,220}, "accent_text");
    auto editor = Add(std::make_shared<input>("Hello, world!\nПривет, мир!\n\nTry selecting across an empty line.\nShift + arrows extend a selection.\nPaste several lines here.\n\nA long line also scrolls horizontally: 0123456789 abcdefghijklmnopqrstuvwxyz\n\nLine 10\nLine 11\nLine 12\nLine 13\nLine 14", input_view::multiline), {426,236,756,452});
    auto stats = Label("14 lines / editable", {426,462,756,486}, "muted_text");
    editor->set_change_callback([editor=std::weak_ptr<input>(editor),stats] { stats->set_text(std::to_string(editor.lock()->get_lines().size())+" lines / editable"); });
    Button("Clear", {426,498,580,530}, [editor] { editor->set_text(""); });
    Button("Insert sample", {596,498,756,530}, [editor] { editor->set_text("Hello, WUI!\n\nUnicode: Привет · café · 日本語\nSelect, copy, paste, repeat."); });
}
void MainFrame::Lists()
{
    Heading("A list with something to do", "Filter sample projects, select a row and browse with the keyboard or scrollbar.");
    auto query = Add(std::make_shared<input>(), {24,176,482,212});
    auto category = Add(std::make_shared<wui::select>(), {500,176,776,212});
    category->set_items({{0,"All projects"},{1,"Desktop"},{2,"Web"}}); category->select_item_number(0);
    auto rows = std::make_shared<std::vector<int>>();
    auto table = Add(std::make_shared<list>(), {24,230,776,486});
    table->update_columns({{360,"Project"},{210,"Target"},{150,"Status"}});
    table->set_item_height_callback([](int, int& height) { height=38; });
    auto title = [](int id) { const char* names[]={"Signal Lab", "Video Monitor", "Neural Editor", "Team Chat", "Media Library", "Query Studio"}; return std::string(names[id%6])+" / "+std::to_string(id+1); };
    table->set_draw_callback([rows,title](graphic& gr,int row,rect r,list::item_state state) {
        if(row<0 || row>=int(rows->size())) return;
        if(state!=list::item_state::normal) gr.draw_rect(r,theme_color("list",state==list::item_state::selected?"selected_item":"active_item"));
        int id=(*rows)[row]; auto f=theme_font("list","font"); auto c=theme_color("text","color");
        gr.draw_text({r.left+12,r.top+10,r.left+350,r.bottom},title(id),c,f);
        gr.draw_text({r.left+372,r.top+10,r.left+558,r.bottom},id%2?"Web":"Desktop",c,f);
        gr.draw_text({r.left+582,r.top+10,r.right,r.bottom},id%3?"Ready":"Draft",theme_color("accent_text","color"),f);
    });
    auto summary = Label("", {24,498,550,532}, "muted_text");
    auto weakQuery=std::weak_ptr<input>(query); auto weakCategory=std::weak_ptr<wui::select>(category);
    auto refresh = [rows,table,weakQuery,weakCategory,summary,title] {
        auto query=weakQuery.lock(); auto category=weakCategory.lock();
        if(!query || !category) return;
        auto needle=query->text();
        std::transform(needle.begin(),needle.end(),needle.begin(),[](unsigned char c){return char(std::tolower(c));});
        rows->clear();
        for(int i=0;i<120;++i) {
            auto name=title(i); std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
            auto kind=category->selected_item().id;
            if(name.find(needle)!=std::string::npos && (kind==0 || (kind==1?i%2==0:i%2==1))) rows->push_back(i);
        }
        table->select_item(-1); table->set_item_count(int(rows->size())); table->scroll_to_start();
        summary->set_text(std::to_string(rows->size())+" sample projects / Type above to filter");
    };
    query->set_change_callback(refresh); category->set_change_callback([refresh](int,int64_t){refresh();}); refresh();
    table->set_item_change_callback([this,rows,title](int row) { if(row>=0 && row<int(rows->size())) Notify("Selected / "+title((*rows)[row])); });
    table->set_item_activate_callback([this,rows,title](int row) { if(row>=0 && row<int(rows->size())) Notify("Opened sample / "+title((*rows)[row])); });
    Button("Reset filters", {596,498,776,532}, [query,category,refresh] { query->set_text(""); category->select_item_number(0); refresh(); });
}
void MainFrame::Menus()
{
    Heading("Small interactions matter", "Nested menus, disabled actions, separators and tooltips.");
    Card({24,178,484,540}); Card({500,178,776,540});
    Label("PROJECT ACTIONS", {44,194,460,222}, "accent_text");
    Label("Open a menu and choose an action.\nExpand Export to see nested items.", {44,240,460,298});
    auto popup=Add(std::make_shared<menu>(), {0});
    auto action=[this](const char* name) { return [this,name](int) { Notify(std::string("Menu / ")+name); }; };
    popup->set_items({
        {1,menu_item_state::normal,"New project","",nullptr,{},action("New project")},
        {2,menu_item_state::normal,"Open recent","",nullptr,{},action("Open recent")},
        {10,menu_item_state::separator,"","",nullptr,{},{}},
        {11,menu_item_state::normal,"Export","",nullptr,{
            {3,menu_item_state::normal,"PNG image","",nullptr,{},action("PNG image")},
            {4,menu_item_state::normal,"CSV table","",nullptr,{},action("CSV table")}},{}},
        {12,menu_item_state::disabled,"Publish (unavailable)","",nullptr,{},{}}
    });
    auto trigger=Button("Project menu", {44,324,460,364}, [] {});
    trigger->set_callback([popup=std::weak_ptr<menu>(popup),trigger=std::weak_ptr<button>(trigger)] { popup.lock()->show_on_control(trigger.lock(),6); });
    Label("Menu actions demonstrate callbacks.\nNo files are created by this sample.", {44,460,460,514}, "muted_text");
    Label("TOOLTIPS", {520,194,756,222}, "accent_text");
    Add(std::make_shared<image>(IMG_LOGO), {520,248,584,312});
    Label("Image control\nTheme-aware resource", {598,250,756,310});
    auto tip=Add(std::make_shared<tooltip>("A WUI tooltip, anchored to a control."), {0});
    auto help=Button("Show tooltip", {520,344,756,384}, [] {});
    help->set_callback([tip,help=std::weak_ptr<button>(help)] { tip->show_on_control(*help.lock(),6); });
    Button("Hide tooltip", {520,448,756,484}, [tip] { tip->hide(); });
}
void MainFrame::Layouts()
{
    Heading("Make room for your tools", "Drag the divider. Panels, splitters, sliders, progress and scroll controls.");
    auto left=Add(std::make_shared<panel>(),{24,178,300,430});
    auto right=Add(std::make_shared<input>("Inspector\n\nDrag the divider to resize this area.\n\nPanels can draw custom graphics.\nSplitters report their new position.\nThe application decides the layout.",input_view::multiline),{316,178,776,430});
    auto text=Label("PROJECT\n\n  src/\n    main.cpp\n    view.cpp\n  res/\n    theme.json",{44,194,280,414});
    auto split=Add(std::make_shared<splitter>(splitter_orientation::vertical,[](int,int){}),{304,178,312,430});
    auto ratio=std::make_shared<double>(.38);
    auto arrange=[this,left,right,text,weakSplit=std::weak_ptr<splitter>(split),ratio] {
        auto split=weakSplit.lock(); if(!split) return;
        int w=std::max(800,this->window->position().width()); int x=24+int((w-48)* *ratio);
        left->set_position({24,178,x-6,430}); text->set_position({44,194,x-20,414});
        split->set_position({x-4,178,x+4,430}); split->set_margins(200,w-240);
        right->set_position({x+10,178,w-24,430});
    };
    layoutDetail=arrange;
    split->set_callback([this,ratio,arrange](int x,int) {
        *ratio=double(x+4-24)/(std::max(800,this->window->position().width())-48); arrange();
        this->window->redraw({0,178,this->window->position().width(),430},true);
    });
    auto level=Add(std::make_shared<progress>(-100,100,25),{426,462,756,482});
    auto label=Label("Balance / +25",{426,498,756,530},"muted_text");
    auto balance=Add(std::make_shared<slider>(-100,100,25,[level,label](int n){level->set_value(n); label->set_text("Balance / "+std::to_string(n));}),{44,456,390,488});
    balance->set_centered_mode(true);
    Add(std::make_shared<wui::scroll>(1000,100,orientation::horizontal,[this](scroll_state,int n){Notify("Scroll / "+std::to_string(n));}),{44,514,390,528});
}
