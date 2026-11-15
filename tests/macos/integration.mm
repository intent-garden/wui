#include "../common/list_layout.hpp"
// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#import <Cocoa/Cocoa.h>
#include <wui/framework/framework.hpp>
#include <wui/window/window.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/control/input.hpp>
#include <wui/control/button.hpp>
#include <wui/control/text.hpp>
#include <wui/control/image.hpp>
#include <wui/control/select.hpp>
#include <wui/control/list.hpp>
#include <wui/control/slider.hpp>
#include <wui/system/clipboard_tools.hpp>
#include <wui/system/timer.hpp>
#include <wui/theme/theme.hpp>
#include <wui/locale/locale.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <thread>
#include "../common/input_selection.hpp"
#include "../common/button_states.hpp"

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); std::abort(); } } while (0)

// Preserve every pasteboard type, including non-text data owned by the user.
struct clipboard_snapshot
{
    NSMutableArray<NSPasteboardItem *> *items = [NSMutableArray new];
    clipboard_snapshot() {
        for(NSPasteboardItem *item in NSPasteboard.generalPasteboard.pasteboardItems) {
            NSPasteboardItem *copy=[NSPasteboardItem new];
            for(NSPasteboardType type in item.types) {
                NSData *data=[item dataForType:type]; if(data) [copy setData:data forType:type];
            }
            [items addObject:copy];
        }
    }
    ~clipboard_snapshot() {
        [NSPasteboard.generalPasteboard clearContents];
        if(items.count) [NSPasteboard.generalPasteboard writeObjects:items];
    }
};

static void click(NSView *view, int x, int y)
{
    NSPoint p=[view convertPoint:NSMakePoint(x,y) toView:nil];
    for (NSEventType type : {NSEventTypeLeftMouseDown,NSEventTypeLeftMouseUp}) {
        NSEvent *e=[NSEvent mouseEventWithType:type location:p modifierFlags:0 timestamp:0
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
        if(type==NSEventTypeLeftMouseDown) [view mouseDown:e]; else [view mouseUp:e];
    }
}
static void key(NSView *view, unsigned short code, NSString *text, NSEventModifierFlags modifiers=0)
{
    NSEvent *e=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:modifiers
        timestamp:0 windowNumber:view.window.windowNumber context:nil characters:text
        charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
    [view keyDown:e];
}
static void screenshot(NSView *view)
{
    [view displayIfNeeded];
    NSBitmapImageRep *rep=[view bitmapImageRepForCachingDisplayInRect:view.bounds];
    [view cacheDisplayInRect:view.bounds toBitmapImageRep:rep];
    CHECK(rep.pixelsWide>=static_cast<int>(view.bounds.size.width));
    int bright=0;
    CGFloat scale=rep.pixelsWide/view.bounds.size.width;
    for(int y=98*scale;y<120*scale;++y) for(int x=25*scale;x<150*scale;++x) {
        NSColor *c=[[rep colorAtX:x y:y] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
        if(c.redComponent>0.7 && c.greenComponent>0.7 && c.blueComponent>0.7) ++bright;
    }
    CHECK(bright>20); // input text must be copied from its offscreen graphic, not just stored.
    NSData *png=[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    CHECK([png writeToFile:@"/tmp/wui-macos-integration.png" atomically:YES]);
}
static void graphics_test()
{
    wui::system_context context{};
    context.scale=2;
    wui::graphic source(context), target(context);
    CHECK(source.init({0,0,20,20},wui::make_color(255,0,0)));
    CHECK(target.init({0,0,80,80},wui::make_color(0,0,0)));
    target.draw_graphic({40,45,20,20},source,0,0);
    auto native=static_cast<CGContextRef>(target.drawable());
    CHECK(CGBitmapContextGetWidth(native)==160 && CGBitmapContextGetHeight(native)==160);
    CGImageRef image=CGBitmapContextCreateImage(native);
    NSBitmapImageRep *rep=[[NSBitmapImageRep alloc] initWithCGImage:image];
    CGImageRelease(image);
    // Memory snapshots have the same top-left orientation as the displayed NSView.
    NSColor *red=[[rep colorAtX:90 y:100] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    NSColor *black=[[rep colorAtX:70 y:100] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CHECK(red.redComponent>0.9 && red.blueComponent<0.1);
    CHECK(black.redComponent<0.1);
    target.draw_image(WUI_SOURCE_DIR "/examples/hello_world/res/images/dark/logo.png",{0,0,20,20});
    CHECK(target.get_error().is_ok());
    // Changing backing scale invalidates offscreen buffers used by input/list.
    context.scale=1;
    CHECK(target.max_size().is_null());
    target.release(); CHECK(target.init({0,0,80,80},wui::make_color(0,0,0)));
    CHECK(CGBitmapContextGetWidth(static_cast<CGContextRef>(target.drawable()))==80);
}
int main()
{
    @autoreleasepool {
        wui::framework::init();
        graphics_test();
        clipboard_snapshot clipboard;
        CHECK(wui::set_default_theme_from_file("dark",WUI_SOURCE_DIR "/examples/simple/res/dark.json"));
        CHECK(wui::set_locale_from_file(wui::locale_type::eng,"English",WUI_SOURCE_DIR "/examples/simple/res/en_locale.json"));
        wui::font font{"Helvetica",18,wui::decorations::normal};
        auto before=wui::measure_text("Привет, macOS!",font);
        CHECK(before.width()>0 && before.height()>0);
        CHECK(wui::measure_text("Hello  ",font).width()>wui::measure_text("Hello",font).width());
        CHECK(run_list_layout_tests());
        CHECK(run_input_selection_tests());
        CHECK(run_button_state_tests());
        auto w=std::make_shared<wui::window>();
        auto input=std::make_shared<wui::input>();
        int clicks=0,created=0,resized=0,emitted=0,closed=0,ticks=0;
        auto button=std::make_shared<wui::button>("Click / Enter",[&]{++clicks;});
        auto label=std::make_shared<wui::text>("Привет, macOS! Retina / UTF-8");
        auto picture=std::make_shared<wui::image>(WUI_SOURCE_DIR "/examples/hello_world/res/images/dark/logo.png");
        CHECK(picture->width()>0 && picture->height()>0);
        w->add_control(label,{20,45,460,75});
        w->add_control(input,{20,90,460,125});
        w->add_control(button,{20,145,200,185});
        w->add_control(picture,{280,145,380,245});
        w->set_default_push_control(button);
        w->subscribe([&](const wui::event& ev){
            if(ev.internal_event_.type==wui::internal_event_type::window_created) {
                ++created; CHECK(w->context().valid()); CHECK(w->get_graphic().drawable());
            }
            if(ev.internal_event_.type==wui::internal_event_type::size_changed) ++resized;
            if(ev.internal_event_.type==wui::internal_event_type::user_emitted) {
                CHECK([NSThread isMainThread]); CHECK(ev.internal_event_.x==42 && ev.internal_event_.y==7); ++emitted;
            }
        },wui::event_type::internal);
        CHECK(w->init("WUI macOS integration",{100,100,600,400},wui::window_style::frame,[&]{++closed;wui::framework::stop();}));
        CHECK(created==1 && resized>=1);
        NSView *view=(__bridge NSView *)w->context().native_view;
        CHECK(view.isFlipped);
        wui::timer t([&]{
            CHECK([NSThread isMainThread]);
            if(++ticks==1) {
                click(view,50,160); CHECK(clicks==1);
                click(view,50,105); CHECK(input->focused());
                [(id<NSTextInputClient>)view insertText:@"Привет 👋" replacementRange:NSMakeRange(NSNotFound,0)];
                CHECK(input->text()=="Привет 👋");
                // Command-A / Command-C / Command-V use the existing input's editing paths.
                key(view,0,@"a",NSEventModifierFlagCommand);
                key(view,8,@"c",NSEventModifierFlagCommand);
                CHECK(wui::clipboard_get_text(w->context())=="Привет 👋");
                key(view,9,@"v",NSEventModifierFlagCommand);
                CHECK(input->text()=="Привет 👋");
                key(view,36,@"\r"); CHECK(clicks==2);
                key(view,48,@"\t"); CHECK(button->focused());
                key(view,48,@"\t",NSEventModifierFlagShift); CHECK(input->focused());
                w->set_position({120,120,660,460});
                CHECK(w->position().width()==540 && w->position().height()==340);
                CHECK(w->get_graphic().max_size().width()==540);
                auto child=std::make_shared<wui::window>();
                auto childInput=std::make_shared<wui::input>("Nested");
                child->add_control(childInput,{10,35,170,65});
                w->add_control(child,{20,210,220,315});
                CHECK(child->init("Embedded",{},wui::window_style::dialog));
                CHECK(!child->is_physical_window());
                child->destroy(); CHECK(child->parent().expired());
                // Physical transient must restore its parent and leave measurement usable.
                auto dialog=std::make_shared<wui::window>();
                dialog->set_transient_for(w,false);
                CHECK(dialog->init("Dialog",{0,0,200,100},wui::window_style::dialog));
                CHECK(!w->enabled()); dialog->destroy(); CHECK(w->enabled());
                CHECK(wui::measure_text("Привет, macOS!",font).width()==before.width());
                auto modal=std::make_shared<wui::window>();
                auto modalInput=std::make_shared<wui::input>();
                modal->add_control(modalInput,{10,35,190,65});
                modal->set_transient_for(w,true);
                CHECK(modal->init("Docked dialog",{0,0,210,120},wui::window_style::dialog));
                CHECK(!modal->is_physical_window() && !w->enabled());
                auto mp=modalInput->position(); click(view,mp.left+15,mp.top+15);
                [(id<NSTextInputClient>)view insertText:@"Modal" replacementRange:NSMakeRange(NSNotFound,0)];
                CHECK(modalInput->text()=="Modal");
                modal->destroy(); CHECK(w->enabled());
                w->set_focused(input);
                // IME composition is visible but only committed text reaches the input.
                [(id<NSTextInputClient>)view setMarkedText:@"日本" selectedRange:NSMakeRange(2,0) replacementRange:NSMakeRange(NSNotFound,0)];
                CHECK([(id<NSTextInputClient>)view hasMarkedText]);
                CHECK(input->text()=="Привет 👋");
                [(id<NSTextInputClient>)view insertText:@"日本" replacementRange:NSMakeRange(NSNotFound,0)];
                CHECK(![(id<NSTextInputClient>)view hasMarkedText]);
                CHECK(input->text().find("日本")!=std::string::npos);
                std::thread emitter([&]{
                    // Legacy clients can request redraw from capture/worker threads;
                    // the AppKit backend must marshal view invalidation to main.
                    for (int redraw = 0; redraw < 100; ++redraw) w->redraw({0,0,200,100}, true);
                    w->emit_event(42,7);
                }); emitter.join();
                screenshot(view);
            } else if(ticks==2) {
                NSWindow *native=(__bridge NSWindow *)w->context().native_window;
                w->minimize();
            } else if(ticks==12) {
                NSWindow *native=(__bridge NSWindow *)w->context().native_window;
                CHECK(native.miniaturized);
                w->normal();
            } else if(ticks==22) {
                NSWindow *native=(__bridge NSWindow *)w->context().native_window;
                CHECK(!native.miniaturized);
                auto p=w->position();
                w->expand(); CHECK(w->state()==wui::window_state::maximized);
                w->normal(); CHECK(w->position().width()==p.width());
                bool veto=true;
                w->set_control_callback([&](wui::window_control action,std::string&,bool& proceed){
                    if(action==wui::window_control::close) proceed=!veto;
                });
                w->destroy(); CHECK(w->context().valid() && closed==0);
                w->set_control_callback({});
            } else if(ticks==23) {
                CHECK(emitted==1);
                t.stop();
                w->destroy();
            }
        });
        t.start(60);
        wui::framework::run();
        CHECK(ticks==23 && closed==1 && !w->context().valid());
        CHECK(wui::measure_text("Привет, macOS!",font).width()==before.width());
        // A stopped run loop can be used again, and timers may destroy themselves.
        CHECK(w->init("Reopened",{100,100,400,300},wui::window_style::frame,[&]{wui::framework::stop();}));
        std::unique_ptr<wui::timer> once;
        once=std::make_unique<wui::timer>([&]{once.reset();w->destroy();});
        once->start(30);
        wui::framework::run();
        CHECK(!once && !w->context().valid());
        // Hover animation must stay on the UI thread and tolerate removal in its callback.
        CHECK(w->init("Scrollbar timer",{100,100,400,300},wui::window_style::frame,[&]{wui::framework::stop();}));
        int activated=0;
        std::shared_ptr<wui::scroll> bar;
        bar=std::make_shared<wui::scroll>(1000,0,wui::orientation::vertical,[&](auto state,int) {
            CHECK([NSThread isMainThread]);
            if(state==wui::scroll_state::activated) {
                ++activated;w->remove_control(bar);bar.reset();w->destroy();
            }
        });
        w->add_control(bar,{250,40,264,180});
        wui::event hover{};hover.type=wui::event_type::mouse;hover.mouse_event_.type=wui::mouse_event_type::enter;
        bar->receive_control_events(hover);
        wui::timer timeout([&]{wui::framework::stop();});timeout.start(2000);
        wui::framework::run();timeout.stop();
        CHECK(activated==1 && !bar && !w->context().valid());
        std::puts("PASS: windows, resize, callbacks, drawing, PNG, UTF-8, clipboard, dialogs, timers, restart");
    }
}
