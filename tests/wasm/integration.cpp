#include "../common/list_layout.hpp"
// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/framework/framework.hpp>
#include <wui/window/window.hpp>
#include <wui/control/input.hpp>
#include <wui/control/button.hpp>
#include <wui/control/message.hpp>
#include <wui/theme/theme.hpp>
#include <wui/locale/locale.hpp>
#include <wui/system/timer.hpp>
#include <emscripten.h>
#include <memory>
#include <cassert>
#include "../common/input_selection.hpp"
#include "../common/button_states.hpp"
namespace {
std::shared_ptr<wui::window> root;
std::shared_ptr<wui::input> first, second;
std::shared_ptr<wui::message> dialog;
std::shared_ptr<wui::scroll> bar;
int scroll_activated=0;
std::unique_ptr<wui::timer> self_timer;
int clicks=0,ticks=0,emitted=0,closed=0,result=0;
bool veto=true;
std::string snapshot;
}
extern "C" {
EMSCRIPTEN_KEEPALIVE const char *test_text(int which) { snapshot=(which?second:first)->text();return snapshot.c_str(); }
EMSCRIPTEN_KEEPALIVE int test_value(int which) {
    switch(which) { case 0:return clicks;case 1:return ticks;case 2:return emitted;case 3:return first->focused();case 4:return second->focused();case 5:return closed;case 6:return result;case 7:return scroll_activated;case 8:return bar->get_scroll_pos();default:return 0; }
}
EMSCRIPTEN_KEEPALIVE void test_action(int action) {
    switch(action) {
        case 0: root->emit_event(12,34);break;
        case 1: root->expand();break;
        case 2: root->normal();break;
        case 3: root->minimize();break;
        case 4: root->destroy();break;
        case 5: veto=false;root->destroy();break;
        case 6:
            dialog=std::make_shared<wui::message>(root,true);
            dialog->show("Modal keyboard routing", "Dialog",wui::message_icon::information,wui::message_button::ok,[](auto){++result;});break;
        case 9:
            second->set_text("a\n\nb");root->set_focused(second);break;
        case 8:
            first->set_text("я");
            second->set_text("a" + std::string(30, '\n') + "z");
            root->set_focused(second);break;
        case 7:
            dialog=std::make_shared<wui::message>(root,false);
            dialog->show("Separate browser window", "Dialog",wui::message_icon::information,wui::message_button::ok,[](auto){++result;});break;
    }
}
EMSCRIPTEN_KEEPALIVE int test_pixels() {
    wui::graphic source(root->context()), target(root->context());
    assert(source.init({0,0,20,20},wui::make_color(0,0,0)));
    assert(target.init({0,0,40,40},wui::make_color(0,0,0)));
    source.draw_rect({0,0,10,20},wui::make_color(255,0,0));
    source.draw_rect({10,0,20,20},wui::make_color(0,0,255));
    target.draw_graphic({5,7,10,12},source,8,0);
    int id=target.drawable();
    return EM_ASM_INT({
        var ctx=Module.wui.surfaces.get($0);var s=ctx.wuiScale;
        var at=function(x,y){return Array.from(ctx.getImageData(x*s,y*s,1,1).data).join(',');};
        return at(5,8)==='255,0,0,255' && at(9,8)==='0,0,255,255' && at(16,8)==='0,0,0,255' && at(9,20)==='0,0,0,255';
    },id);
}
}
int main()
{
    wui::framework::init();
    assert(wui::set_default_theme_from_file("dark","res/dark.json"));
    assert(wui::set_locale_from_file(wui::locale_type::eng,"English","res/en_locale.json"));
    assert(run_list_layout_tests());
    assert(run_input_selection_tests());
    assert(run_button_state_tests());
    root=std::make_shared<wui::window>();
    first=std::make_shared<wui::input>();second=std::make_shared<wui::input>("",wui::input_view::multiline);
    // Stack-owned callback data verifies framework::run() preserves main's stack.
    int stack_counter=0;
    auto button=std::make_shared<wui::button>("Click",[&](){clicks=++stack_counter;});
    root->add_control(first,{20,60,300,100});root->add_control(second,{20,120,300,180});root->add_control(button,{20,200,140,240});
    bar=std::make_shared<wui::scroll>(1000,0,wui::orientation::vertical,[](auto state,int){if(state==wui::scroll_state::activated) ++scroll_activated;});
    root->add_control(bar,{350,60,364,260});
    root->set_control_callback([](auto control,std::string&,bool& proceed){if(control==wui::window_control::close && veto) proceed=false;});
    root->subscribe([](const auto& event){if(event.internal_event_.type==wui::internal_event_type::user_emitted && event.internal_event_.x==12 && event.internal_event_.y==34) ++emitted;},wui::event_type::internal);
    assert(root->init("WASM integration",{40,40,540,440},wui::window_style::frame|wui::window_style::border_all,[]{++closed;wui::framework::stop();}));
    self_timer=std::make_unique<wui::timer>([]{++ticks;self_timer.reset();});self_timer->start(30);
    wui::framework::run();
    assert(stack_counter==clicks);
    EM_ASM({Module.wuiTestReturned=true;});
}
