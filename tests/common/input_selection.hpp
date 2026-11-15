// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#pragma once
#include <wui/control/input.hpp>
#include <wui/system/clipboard_tools.hpp>
#include <wui/theme/theme.hpp>
#include <cstdio>
#include <stdexcept>
#ifdef __APPLE__
#include "../../src/macos/window_mac.hpp"
using selection_backend = wui::macos_window_backend;
#elif __EMSCRIPTEN__
#include "../../src/wasm/window_wasm.hpp"
using selection_backend = wui::wasm_window_backend;
#endif

// Exercise the shared input through the same window event routing as the OS
// backends, without inspecting or modifying the input's private state.
inline bool run_input_selection_tests()
{
    using namespace wui;
    auto w=std::make_shared<window>();
    auto field=std::make_shared<input>();
    auto other=std::make_shared<input>();
    w->add_control(field,{20,60,460,200});w->add_control(other,{20,220,460,260});
    if(!w->init("Input selection tests",{20,20,520,320},window_style::frame)) return false;
    int failures=0, cases=0, changes=0;
    field->set_change_callback([&]{++changes;});
    auto reset=[&](std::string_view text,input_view view=input_view::singleline,int limit=10000) {
        field->set_input_view(view);field->set_input_content(input_content::text);field->set_symbols_limit(limit);field->set_text(text);w->set_focused(field);changes=0;
    };
    auto key=[&](uint8_t code,bool shift=false,keyboard_event_type type=keyboard_event_type::down) {
        keyboard_event e{};e.type=type;e.key[0]=code;e.key_size=1;e.modifier=shift?vk_lshift:0;
        selection_backend::keyboard(*w,e);
    };
    auto typed=[&](std::string_view value) {
        for(size_t i=0;i<value.size();) {
            unsigned char c=value[i];size_t n=c<0x80?1:c<0xe0?2:c<0xf0?3:4;
            keyboard_event e{};e.type=keyboard_event_type::key;e.key_size=n;
            value.substr(i,n).copy(e.key,n);selection_backend::keyboard(*w,e);i+=n;
        }
    };
    auto shortcut=[&](char c){key(c&31,false,keyboard_event_type::key);};
    auto copied=[&](){clipboard_put("<no selection>",w->context());shortcut('c');return clipboard_get_text(w->context());};
    auto paste=[&](std::string_view text){clipboard_put(text,w->context());shortcut('v');};
    auto mouse=[&](mouse_event_type type,int x,int y){selection_backend::mouse(*w,{type,x,y,0});};
    auto x_at=[&](std::string_view prefix){return 25+measure_text(prefix,theme_font(input::tc,input::tv_font)).right;};
    auto equal=[](const auto& actual,const auto& expected){if(actual!=expected) throw std::runtime_error("unexpected result");};
    auto test=[&](const char *name,auto body) {
        ++cases;try {body();} catch(const std::exception& e){++failures;std::fprintf(stderr,"FAIL input: %s (%s)\n",name,e.what());}
    };
    test("Shift extension survives releasing Shift", [&] {
        reset("abcdef");
        key(vk_right);
        key(vk_right);
        key(vk_right,true);
        key(vk_lshift,false,keyboard_event_type::up);
        key(vk_right,true);
        equal(copied(),std::string("cd"));
    });
    test("Mouse movement does not extend keyboard selection", [&] {
        reset("abcdef");
        key(vk_right,true);
        key(vk_right,true);
        selection_backend::mouse(*w,{mouse_event_type::move,200,100,0});
        selection_backend::mouse(*w,{mouse_event_type::move,220,100,0});
        equal(copied(),std::string("ab"));
    });
    test("Left collapses selection to start", [&] {
        reset("abcdef");
        key(vk_right);
        key(vk_right,true);
        key(vk_right,true);
        key(vk_left);
        typed("X");
        equal(field->text(),std::string("aXbcdef"));
    });
    test("Right collapses reverse selection to end", [&] {
        reset("abcdef");
        key(vk_end);
        key(vk_left,true);
        key(vk_left,true);
        key(vk_right);
        typed("X");
        equal(field->text(),std::string("abcdefX"));
    });
    test("Reverse multiline selection copies boundaries", [&] {
        reset("ab\n\ncd",input_view::multiline);
        key(vk_down);
        key(vk_down);
        key(vk_right);
        key(vk_up,true);
        key(vk_up,true);
        equal(copied(),std::string("b\n\nc"));
    });
    test("Shift PageDown selects across lines", [&] {
        reset("one\ntwo\nthree",input_view::multiline);
        key(vk_page_down,true);
        equal(copied(),std::string("one\ntwo\n"));
    });
    test("Vertical navigation keeps preferred column", [&] {
        reset("abcdef\nx\nabcdef",input_view::multiline);
        for(int i=0;i<5;++i)key(vk_right);
        key(vk_down);
        key(vk_down);
        typed("X");
        equal(field->text(),std::string("abcdef\nx\nabcdeXf"));
    });
    test("Enter replaces multiline selection", [&] {
        reset("one\ntwo\nthree",input_view::multiline);
        shortcut('a');
        key(vk_return);
        equal(field->text(),std::string("\n"));
    });
    test("Delete joins after a Unicode line", [&] {
        reset("я\nZ",input_view::multiline);
        key(vk_end);
        key(vk_del);
        equal(field->text(),std::string("яZ"));
    });
    test("Backspace joins Unicode lines", [&] {
        reset("я\n🦊",input_view::multiline);
        key(vk_down);
        key(vk_back);
        equal(field->text(),std::string("я🦊"));
    });
    test("Readonly protects selected deletion", [&] {
        reset("read only",input_view::readonly);
        shortcut('a');
        key(vk_back);
        key(vk_del);
        equal(field->text(),std::string("read only"));
        equal(changes,0);
    });
    test("Typing replaces selection at the limit once", [&] {
        reset("abcde",input_view::singleline,5);
        shortcut('a');
        typed("X");
        equal(field->text(),std::string("X"));
        equal(changes,1);
    });
    test("Unlimited input accepts typing", [&] {
        reset("",input_view::singleline,-1);
        typed("Привет 🦊");
        equal(field->text(),std::string("Привет 🦊"));
    });
    test("Limit counts Unicode symbols", [&] {
        reset("",input_view::singleline,2);
        typed("я🦊Z");
        equal(field->text(),std::string("я🦊"));
    });
    test("Rejected paste preserves selection and text", [&] {
        reset("abcde",input_view::singleline,5);
        shortcut('a');
        paste("too long");
        equal(field->text(),std::string("abcde"));
        equal(copied(),std::string("abcde"));
        equal(changes,0);
    });
    test("Paste keeps trailing newlines and normalizes CRLF", [&] {
        reset("tail",input_view::multiline);
        paste("A\r\n\r\n");
        equal(field->text(),std::string("A\n\ntail"));
    });
    test("Single-line paste cannot create hidden lines", [&] {
        reset("",input_view::singleline);
        paste("a\r\nb\nc");
        equal(field->text(),std::string("a b c"));
    });
    test("Pasted invalid numeric data preserves selection", [&] {
        reset("123");
        field->set_input_content(input_content::integer);
        shortcut('a');
        paste("x");
        equal(field->text(),std::string("123"));
        equal(changes,0);
    });
    test("set_text respects string_view bounds and trailing newline", [&] {
        reset("");
        std::string text="ab\nTAIL";
        field->set_input_view(input_view::multiline);
        field->set_text(std::string_view(text.data(),3));
        equal(field->text(),std::string("ab\n"));
    });
    test("Focus loss clears the entire multiline selection", [&] {
        reset("one\ntwo\nthree",input_view::multiline);
        shortcut('a');
        w->set_focused(other);
        w->set_focused(field);
        typed("X");
        equal(field->text(),std::string("one\ntwo\nthreeX"));
    });
    test("Cut refreshes content and emits one change", [&] {
        reset("ab\ncd",input_view::multiline);
        key(vk_right);
        key(vk_down,true);
        shortcut('x');
        equal(field->text(),std::string("ad"));
        equal(changes,1);
    });
    test("Replacement callback observes final text only", [&] {
        reset("abc");
        shortcut('a');
        std::string seen;
        field->set_change_callback([&]{seen=field->text();++changes;});
        typed("X");
        field->set_change_callback([&]{++changes;});
        equal(seen,std::string("X"));
        equal(changes,1);
    });
    test("Delete at end does not emit change", [&] {
        reset("я");
        key(vk_end);
        key(vk_del);
        equal(changes,0);
    });
    test("Mouse drag selects Unicode by character", [&] {
        reset("я🦊abc");
        mouse(mouse_event_type::move,x_at("я"),100);
        mouse(mouse_event_type::left_down,x_at("я"),100);
        mouse(mouse_event_type::move,x_at("я🦊a"),100);
        mouse(mouse_event_type::left_up,x_at("я🦊a"),100);
        equal(copied(),std::string("🦊a"));
    });
    test("Reverse mouse drag selects the same range", [&] {
        reset("я🦊abc");
        mouse(mouse_event_type::left_down,x_at("я🦊a"),100);
        mouse(mouse_event_type::move,x_at("я"),100);
        mouse(mouse_event_type::left_up,x_at("я"),100);
        equal(copied(),std::string("🦊a"));
    });
    test("Dragging above input clamps to first row", [&] {
        reset("abc\ndef\nghi",input_view::multiline);
        int h=theme_font(input::tc,input::tv_font).size;
        mouse(mouse_event_type::left_down,x_at("a"),60+2*h+h/2);
        mouse(mouse_event_type::move,25,35);
        mouse(mouse_event_type::left_up,25,35);
        equal(copied(),std::string("abc\ndef\ng"));
    });
    test("Dragging below input clamps to last row", [&] {
        reset("abc\ndef",input_view::multiline);
        mouse(mouse_event_type::left_down,25,65);
        mouse(mouse_event_type::move,450,210);
        mouse(mouse_event_type::left_up,450,210);
        equal(copied(),std::string("abc\ndef"));
    });
    test("Mouse release ends dragging outside input", [&] {
        reset("abcdef");
        mouse(mouse_event_type::left_down,25,100);
        mouse(mouse_event_type::move,480,100);
        mouse(mouse_event_type::left_up,480,100);
        mouse(mouse_event_type::move,26,100);
        equal(copied(),std::string("abcdef"));
    });
    test("Double click selects Unicode word with valid endpoint", [&] {
        reset("Привет мир");
        mouse(mouse_event_type::left_double,x_at("Привет ми"),100);
        equal(copied(),std::string("мир"));
        typed("X");
        equal(field->text(),std::string("Привет X"));
    });
    test("Double click on empty line is harmless", [&] {
        reset("a\n\nb",input_view::multiline);
        int h=theme_font(input::tc,input::tv_font).size;
        mouse(mouse_event_type::left_double,25,60+h+h/2);
        typed("X");
        equal(field->text(),std::string("a\nX\nb"));
    });
    test("Selecting only a newline joins the lines on delete", [&] {
        reset("я\n🦊",input_view::multiline);
        key(vk_end);
        key(vk_right,true);
        equal(copied(),std::string("\n"));
        key(vk_del);
        equal(field->text(),std::string("я🦊"));
    });
    test("Enter at limit preserves selected text", [&] {
        reset("abc",input_view::multiline,3);
        key(vk_end);
        key(vk_return);
        equal(field->text(),std::string("abc"));
        equal(changes,0);
    });
    test("Password cut neither copies nor removes text", [&] {
        reset("секрет",input_view::password);
        shortcut('a');
        clipboard_put("safe",w->context());
        shortcut('x');
        equal(field->text(),std::string("секрет"));
        equal(clipboard_get_text(w->context()),std::string("safe"));
    });
    test("Double click on right half of final letter selects word", [&] {
        reset("word next");
        mouse(mouse_event_type::left_double,x_at("word")-1,100);
        equal(copied(),std::string("word"));
    });
    auto menu_action = [&](int item) {
        mouse(mouse_event_type::right_up, 100, 100);
        key(vk_page_up);
        for (int i = 0; i < item; ++i) key(vk_down);
        key(vk_return);
    };
    test("Context menu copies the original multiline selection", [&] {
        reset("one\ntwo", input_view::multiline);
        shortcut('a');
        clipboard_put("sentinel", w->context());
        menu_action(1);
        equal(clipboard_get_text(w->context()), std::string("one\ntwo"));
    });
    test("Context menu cuts the selected range", [&] {
        reset("abcd");
        key(vk_right);
        key(vk_right, true);
        menu_action(0);
        equal(field->text(), std::string("acd"));
        equal(clipboard_get_text(w->context()), std::string("b"));
        equal(changes, 1);
    });
    test("Context menu paste replaces the selected range", [&] {
        reset("one\ntwo", input_view::multiline);
        shortcut('a');
        clipboard_put("X", w->context());
        menu_action(2);
        equal(field->text(), std::string("X"));
        equal(changes, 1);
    });
    test("set_caret_position places the caret for the next keystroke", [&] {
        reset("abcdef");
        field->set_caret_position(3);
        typed("X");
        equal(field->text(),std::string("abcXdef"));
    });
    test("set_caret_position counts characters, not bytes", [&] {
        reset("я🦊abc");
        field->set_caret_position(2);
        typed("X");
        equal(field->text(),std::string("я🦊Xabc"));
    });
    test("set_caret_position past the end clamps to the end", [&] {
        reset("abc");
        field->set_caret_position(99);
        typed("X");
        equal(field->text(),std::string("abcX"));
    });
    test("caret_position counts newlines across multiline text", [&] {
        reset("one\ntwo",input_view::multiline);
        field->set_caret_position(5);
        equal(field->caret_position(),size_t{5});
        typed("X");
        equal(field->text(),std::string("one\ntXwo"));
    });
    test("set_caret_position drops the selection", [&] {
        reset("abcdef\nx",input_view::multiline);
        shortcut('a');
        field->set_caret_position(3);
        typed("X");
        equal(field->text(),std::string("abcXdef\nx"));
    });
    field->set_change_callback({});w->destroy();
    std::printf("Input selection: %d/%d passed\n",cases-failures,cases);
    return failures==0;
}
