// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#import <Cocoa/Cocoa.h>
#include "window_mac.hpp"
#include "framework_mac.hpp"
#include <wui/theme/theme.hpp>
#include <wui/framework/framework.hpp>
#include <wui/common/flag_helpers.hpp>
#include <wui/control/button.hpp>
#include <wui/control/input.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

using bridge = wui::macos_window_backend;

@interface WUIWindow : NSWindow
{
@public
    std::weak_ptr<wui::window> owner;
}
@end
@implementation WUIWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
- (void)performClose:(id)sender { if (auto w=owner.lock()) w->destroy(); }
@end

@interface WUIView : NSView <NSWindowDelegate, NSTextInputClient>
{
@public
    std::weak_ptr<wui::window> owner;
    NSTrackingArea *tracking;
    NSMutableAttributedString *marked;
    NSRange markedSelection;
    NSPoint dragOrigin;
    NSRect originalFrame;
    NSUInteger resizeEdges;
    double wheelRemainder;
}
@end

static uint8_t key_code(unsigned short code)
{
    using namespace wui;
    switch(code) {
        case 48:return vk_tab; case 36:return vk_return; case 76:return vk_return;
        case 53:return vk_esc; case 51:return vk_back; case 117:return vk_del;
        case 123:return vk_left; case 124:return vk_right; case 125:return vk_down; case 126:return vk_up;
        case 115:return vk_home; case 119:return vk_end; case 116:return vk_page_up; case 121:return vk_page_down;
        case 56:return vk_lshift; case 60:return vk_rshift;
        default:return 0xFF;
    }
}
static uint8_t modifier(NSEvent *event)
{
    if(event.modifierFlags & NSEventModifierFlagShift) return wui::vk_lshift;
    if(event.modifierFlags & (NSEventModifierFlagCommand|NSEventModifierFlagControl)) return wui::vk_lcontrol;
    if(event.modifierFlags & NSEventModifierFlagOption) return wui::vk_alt;
    return 0;
}
@implementation WUIView
- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if(tracking) [self removeTrackingArea:tracking];
    tracking=[[NSTrackingArea alloc] initWithRect:NSZeroRect options:NSTrackingMouseEnteredAndExited
        |NSTrackingMouseMoved|NSTrackingActiveInKeyWindow|NSTrackingInVisibleRect owner:self userInfo:nil];
    [self addTrackingArea:tracking];
}
- (void)drawRect:(NSRect)dirty
{
    if(auto w=owner.lock()) bridge::paint(*w,
        {static_cast<int32_t>(std::floor(dirty.origin.x)),static_cast<int32_t>(std::floor(dirty.origin.y)),
         static_cast<int32_t>(std::ceil(NSMaxX(dirty))),static_cast<int32_t>(std::ceil(NSMaxY(dirty)))},
        NSGraphicsContext.currentContext.CGContext);
    if(self.hasMarkedText) {
        if(auto w=owner.lock()) {
            auto p=bridge::composition_rect(*w);
            [NSColor.textBackgroundColor setFill]; NSRectFill(NSMakeRect(p.left,p.top,p.width(),p.height()));
            [marked.string drawAtPoint:NSMakePoint(p.left,p.top) withAttributes:@{
                NSFontAttributeName:[NSFont systemFontOfSize:16], NSForegroundColorAttributeName:NSColor.textColor,
                NSUnderlineStyleAttributeName:@(NSUnderlineStyleSingle)}];
        }
    }
}
- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    if(auto w=owner.lock()) bridge::resized(*w);
}
- (void)sendMouse:(NSEvent *)event type:(wui::mouse_event_type)type delta:(int32_t)delta
{
    if(auto w=owner.lock()) {
        NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
        bridge::mouse(*w,{type,static_cast<int32_t>(p.x),static_cast<int32_t>(p.y),delta});
    }
}
- (void)mouseDown:(NSEvent *)event
{
    auto w=owner.lock(); if(!w || !bridge::accepts_input(*w)) return;
    [self.window makeFirstResponder:self];
    NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
    resizeEdges=0;
    if(bridge::resizable(*w)) {
        if(p.x<5) resizeEdges|=1;
        if(p.x>self.bounds.size.width-5) resizeEdges|=2;
        if(p.y<5) resizeEdges|=4;
        if(p.y>self.bounds.size.height-5) resizeEdges|=8;
    }
    if(resizeEdges) {
        dragOrigin=NSEvent.mouseLocation; originalFrame=self.window.frame; return;
    }
    if(p.y<30 && bridge::movable(*w) && !bridge::hit_control(*w,p.x,p.y)) {
        if(event.clickCount==2 && bridge::resizable(*w)) {
            if(w->state()==wui::window_state::maximized) w->normal(); else w->expand();
        } else [self.window performWindowDragWithEvent:event];
        return;
    }
    [self sendMouse:event type:event.clickCount==2 ? wui::mouse_event_type::left_double : wui::mouse_event_type::left_down delta:0];
}
- (void)mouseDragged:(NSEvent *)event
{
    if(resizeEdges) {
        NSPoint p=NSEvent.mouseLocation;
        CGFloat dx=p.x-dragOrigin.x,dy=p.y-dragOrigin.y;
        NSRect frame=originalFrame;
        if(resizeEdges&1) { frame.origin.x+=dx; frame.size.width-=dx; }
        if(resizeEdges&2) frame.size.width+=dx;
        if(resizeEdges&4) frame.size.height+=dy;
        if(resizeEdges&8) { frame.origin.y+=dy; frame.size.height-=dy; }
        NSSize min=self.window.minSize;
        if(frame.size.width<min.width) { if(resizeEdges&1) frame.origin.x=NSMaxX(originalFrame)-min.width; frame.size.width=min.width; }
        if(frame.size.height<min.height) { if(resizeEdges&8) frame.origin.y=NSMaxY(originalFrame)-min.height; frame.size.height=min.height; }
        [self.window setFrame:frame display:YES];
    } else [self sendMouse:event type:wui::mouse_event_type::move delta:0];
}
- (void)mouseUp:(NSEvent *)event { if(resizeEdges) resizeEdges=0; else [self sendMouse:event type:wui::mouse_event_type::left_up delta:0]; }
- (void)mouseMoved:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::move delta:0]; }
- (void)mouseEntered:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::enter delta:0]; }
- (void)mouseExited:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::leave delta:0]; }
- (void)rightMouseDown:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::right_down delta:0]; }
- (void)rightMouseUp:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::right_up delta:0]; }
- (void)rightMouseDragged:(NSEvent *)event { [self mouseMoved:event]; }
- (void)otherMouseDown:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::center_down delta:0]; }
- (void)otherMouseUp:(NSEvent *)event { [self sendMouse:event type:wui::mouse_event_type::center_up delta:0]; }
- (void)scrollWheel:(NSEvent *)event
{
    wheelRemainder+=event.scrollingDeltaY*(event.hasPreciseScrollingDeltas ? 4 : 120);
    int32_t delta=static_cast<int32_t>(wheelRemainder);
    wheelRemainder-=delta;
    if(delta) [self sendMouse:event type:wui::mouse_event_type::wheel delta:delta];
}
- (BOOL)performKeyEquivalent:(NSEvent *)event
{
    if(event.modifierFlags & (NSEventModifierFlagCommand|NSEventModifierFlagControl)) {
        NSString *key=event.charactersIgnoringModifiers.lowercaseString;
        const char *chars=key.UTF8String;
        if(chars && std::strlen(chars)==1 && std::strchr("acvx",chars[0])) {
            if(auto w=owner.lock()) {
                wui::keyboard_event e{}; e.type=wui::keyboard_event_type::key;
                e.key[0]=chars[0]&0x1F; e.key_size=1; e.modifier=wui::vk_lcontrol;
                bridge::keyboard(*w,e);
            }
            return YES;
        }
    }
    return [super performKeyEquivalent:event];
}
- (void)keyDown:(NSEvent *)event
{
    auto w=owner.lock(); if(!w || !bridge::accepts_input(*w)) return;
    if([self performKeyEquivalent:event]) return;
    if(!self.hasMarkedText) {
        wui::keyboard_event e{}; e.type=wui::keyboard_event_type::down; e.modifier=modifier(event);
        e.key[0]=key_code(event.keyCode); e.key_size=1;
        bridge::keyboard(*w,e);
    }
    [self interpretKeyEvents:@[event]];
}
- (void)keyUp:(NSEvent *)event
{
    if(auto w=owner.lock()) {
        wui::keyboard_event e{}; e.type=wui::keyboard_event_type::up; e.modifier=modifier(event);
        e.key[0]=key_code(event.keyCode); e.key_size=1; bridge::keyboard(*w,e);
    }
}
- (void)flagsChanged:(NSEvent *)event
{
    if(auto w=owner.lock()) {
        wui::keyboard_event e{}; e.type=(event.modifierFlags & NSEventModifierFlagShift) ? wui::keyboard_event_type::down : wui::keyboard_event_type::up;
        e.modifier=modifier(event); e.key[0]=key_code(event.keyCode); e.key_size=1;
        bridge::keyboard(*w,e);
    }
}
- (void)insertText:(id)value replacementRange:(NSRange)replacementRange
{
    NSString *text=[value isKindOfClass:NSAttributedString.class] ? [value string] : value;
    [self unmarkText];
    if(auto w=owner.lock()) {
        NSData *bytes=[text dataUsingEncoding:NSUTF8StringEncoding];
        const unsigned char *data=static_cast<const unsigned char *>(bytes.bytes);
        for(NSUInteger i=0;i<bytes.length;) {
            NSUInteger n=data[i]<0x80 ? 1 : data[i]<0xE0 ? 2 : data[i]<0xF0 ? 3 : 4;
            if(i+n>bytes.length) break;
            wui::keyboard_event e{}; e.type=wui::keyboard_event_type::key; e.key_size=n;
            std::memcpy(e.key,data+i,n); i+=n;
            if(n==1 && static_cast<unsigned char>(e.key[0])<32) continue;
            bridge::keyboard(*w,e);
        }
    }
}
- (void)doCommandBySelector:(SEL)selector {} // Navigation is delivered as WUI key-down events.
- (BOOL)hasMarkedText { return marked.length>0; }
- (NSRange)markedRange { return self.hasMarkedText ? NSMakeRange(0,marked.length) : NSMakeRange(NSNotFound,0); }
- (NSRange)selectedRange { return self.hasMarkedText ? markedSelection : NSMakeRange(0,0); }
- (void)setMarkedText:(id)value selectedRange:(NSRange)selection replacementRange:(NSRange)replacement
{
    marked=[value isKindOfClass:NSAttributedString.class] ? [value mutableCopy] : [[NSMutableAttributedString alloc] initWithString:value];
    markedSelection=selection;
    [self setNeedsDisplay:YES];
}
- (void)unmarkText { marked=nil; markedSelection=NSMakeRange(0,0); [self setNeedsDisplay:YES]; }
- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText { return @[]; }
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual
{
    if(!marked || range.location>=marked.length) return nil;
    range=NSIntersectionRange(range,NSMakeRange(0,marked.length)); if(actual) *actual=range;
    return [marked attributedSubstringFromRange:range];
}
- (NSUInteger)characterIndexForPoint:(NSPoint)point { return NSNotFound; }
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual
{
    if(actual) *actual=range;
    wui::rect p{0,0,1,20};
    if(auto w=owner.lock()) p=bridge::composition_rect(*w);
    return [self.window convertRectToScreen:[self convertRect:NSMakeRect(p.left,p.top,p.width(),p.height()) toView:nil]];
}
- (void)windowDidResize:(NSNotification *)note { if(auto w=owner.lock()) bridge::resized(*w); }
- (void)windowDidMove:(NSNotification *)note { if(auto w=owner.lock()) bridge::moved(*w); }
- (void)windowDidBecomeKey:(NSNotification *)note { if(auto w=owner.lock()) bridge::focus(*w,true); }
- (void)windowDidResignKey:(NSNotification *)note { if(auto w=owner.lock()) bridge::focus(*w,false); }
- (void)windowDidDeminiaturize:(NSNotification *)note { if(auto w=owner.lock()) w->normal(); }
@end

static CGFloat screen_top() { return NSMaxY(NSScreen.screens.firstObject.frame); }
static NSScreen *screen_for(wui::system_context& c)
{
    NSWindow *window=(__bridge NSWindow *)c.native_window;
    return window.screen ?: NSScreen.mainScreen;
}
void center_horizontally(wui::rect& p,wui::system_context& c)
{
    NSRect frame=screen_for(c).visibleFrame;
    int32_t width=p.right; p.left=frame.origin.x+(frame.size.width-width)/2; p.right=p.left+width;
}
void center_vertically(wui::rect& p,wui::system_context& c)
{
    NSRect frame=screen_for(c).visibleFrame;
    int32_t height=p.bottom; p.top=screen_top()-NSMaxY(frame)+(frame.size.height-height)/2; p.bottom=p.top+height;
}

namespace wui
{
bool macos_window_backend::create(window& w)
{
    @autoreleasepool {
        if(![NSThread isMainThread]) { w.err={error_type::system_error,"window::init()","AppKit requires the main thread"}; return false; }
        if(w.context_.valid()) { w.err={error_type::already_started,"window::init()","Window already exists"}; return false; }
        framework::init_macos();
        if(w.position_.left==-1) center_horizontally(w.position_,w.context_);
        if(w.position_.top==-1) center_vertically(w.position_,w.context_);
        if(w.position_.width()<=0 || w.position_.height()<=0) {
            w.err={error_type::invalid_value,"window::init()","Window dimensions must be positive"}; return false;
        }
        NSRect frame=NSMakeRect(w.position_.left,screen_top()-w.position_.bottom,w.position_.width(),w.position_.height());
        NSWindowStyleMask mask=NSWindowStyleMaskBorderless;
        if(flag_is_set(w.window_style_,window_style::resizable)) mask|=NSWindowStyleMaskResizable;
        if(flag_is_set(w.window_style_,window_style::minimize_button)) mask|=NSWindowStyleMaskMiniaturizable;
        WUIWindow *native=[[WUIWindow alloc] initWithContentRect:frame styleMask:mask
            backing:NSBackingStoreBuffered defer:NO];
        native.releasedWhenClosed=NO;
        native.hasShadow=YES;
        native.acceptsMouseMovedEvents=YES;
        native->owner=w.shared_from_this();
        WUIView *view=[[WUIView alloc] initWithFrame:NSMakeRect(0,0,frame.size.width,frame.size.height)];
        view->owner=w.shared_from_this();
        native.contentView=view;
        native.delegate=view;
        w.context_.native_window=(__bridge_retained void *)native;
        w.context_.native_view=(__bridge void *)view;
        w.context_.scale=native.backingScaleFactor;
        style(w);
        if(!w.graphic_.init({0,0,w.position_.width(),w.position_.height()},theme_color(w.tcn,window::tv_background,w.theme_))) {
            w.err=w.graphic_.get_error(); close(w,false); return false;
        }
        [native makeFirstResponder:view];
        w.send_internal(internal_event_type::window_created,0,0);
        if(!w.context_.valid()) return true;
        w.send_internal(internal_event_type::size_changed,w.position_.width(),w.position_.height());
        if(w.context_.valid() && w.showed_) [native makeKeyAndOrderFront:nil];
        return true;
    }
}
void macos_window_backend::close(window& w,bool notify)
{
    if(!w.context_.native_window) return;
    WUIWindow *native=(__bridge_transfer WUIWindow *)w.context_.native_window;
    WUIView *view=(WUIView *)native.contentView;
    view->owner.reset(); native->owner.reset(); native.delegate=nil;
    w.context_.native_window=nullptr; w.context_.native_view=nullptr;
    w.graphic_.release();
    [native orderOut:nil]; [native close];
    if(auto parent=w.get_transient_window()) parent->enable();
    if(notify && w.close_callback) { auto callback=w.close_callback; callback(); }
}
void macos_window_backend::invalidate(window& w,rect p)
{
    if (![NSThread isMainThread]) {
        auto weak = w.weak_from_this();
        dispatch_async(dispatch_get_main_queue(), ^{
            if (auto owner = weak.lock(); owner && owner->context_.valid()) invalidate(*owner, p);
        });
        return;
    }
    NSView *view=(__bridge NSView *)w.context_.native_view;
    if(view) [view setNeedsDisplayInRect:NSMakeRect(p.left,p.top,std::max(0,p.width()),std::max(0,p.height()))];
}
void macos_window_backend::position(window& w,rect p)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    if(native) [native setFrame:NSMakeRect(p.left,screen_top()-p.bottom,p.width(),p.height()) display:YES];
}
void macos_window_backend::style(window& w)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    if(!native) return;
    native.title=[[NSString alloc] initWithBytes:w.caption.data() length:w.caption.size() encoding:NSUTF8StringEncoding] ?: @"";
    native.level=flag_is_set(w.window_style_,window_style::topmost) ? NSFloatingWindowLevel : NSNormalWindowLevel;
    native.minSize=NSMakeSize(std::max(1,w.min_width),std::max(1,w.min_height));
    NSWindowStyleMask mask=NSWindowStyleMaskBorderless;
    if(flag_is_set(w.window_style_,window_style::resizable)) mask|=NSWindowStyleMaskResizable;
    if(flag_is_set(w.window_style_,window_style::minimize_button)) mask|=NSWindowStyleMaskMiniaturizable;
    if(native.styleMask!=mask) native.styleMask=mask;
}
void macos_window_backend::show(window& w,bool visible)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    if(visible) [native makeKeyAndOrderFront:nil]; else [native orderOut:nil];
}
void macos_window_backend::minimize(window& w) { [(__bridge NSWindow *)w.context_.native_window miniaturize:nil]; }
void macos_window_backend::expand(window& w)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    [native setFrame:screen_for(w.context_).visibleFrame display:YES];
    w.send_internal(internal_event_type::window_expanded,w.position_.width(),w.position_.height());
}
void macos_window_backend::restore(window& w) { [(__bridge NSWindow *)w.context_.native_window deminiaturize:nil]; }
void macos_window_backend::emit(window& w,int32_t x,int32_t y)
{
    auto weak=w.weak_from_this();
    dispatch_async(dispatch_get_main_queue(), ^{ if(auto locked=weak.lock(); locked && locked->context_.valid()) locked->send_internal(internal_event_type::user_emitted,x,y); });
}
void macos_window_backend::paint(window& w,rect dirty,void *destination)
{
    if(w.skip_draw_ || !w.context_.valid()) return;
    auto ctx=static_cast<CGContextRef>(w.graphic_.drawable());
    if(!ctx) return;
    CGContextSaveGState(ctx);
    CGContextClipToRect(ctx,CGRectMake(dirty.left,dirty.top,dirty.width(),dirty.height()));
    w.graphic_.clear(dirty);
    w.draw_caption(w.graphic_,dirty);
    auto controls=w.controls;
    for(bool topmost : {false,true}) for(auto& control:controls) {
        if(control && control->showed() && control->topmost()==topmost && control->position().in(dirty))
            control->draw(w.graphic_,dirty);
    }
    w.draw_border(w.graphic_);
    CGContextRestoreGState(ctx);
    CGImageRef snapshot=CGBitmapContextCreateImage(ctx);
    if(snapshot) {
        auto dest=static_cast<CGContextRef>(destination);
        CGContextSaveGState(dest);
        CGContextTranslateCTM(dest,0,w.position_.height());
        CGContextScaleCTM(dest,1,-1);
        CGContextDrawImage(dest,CGRectMake(0,0,w.position_.width(),w.position_.height()),snapshot);
        CGContextRestoreGState(dest);
        CGImageRelease(snapshot);
    }
}
void macos_window_backend::resized(window& w)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    if(!native) return;
    NSSize size=native.contentView.bounds.size;
    double scale=native.backingScaleFactor;
    if(w.graphic_.max_size().width()!=static_cast<int32_t>(size.width) ||
       w.graphic_.max_size().height()!=static_cast<int32_t>(size.height) || w.context_.scale!=scale) {
        w.context_.scale=scale;
        w.graphic_.release();
        if(!w.graphic_.init({0,0,static_cast<int32_t>(size.width),static_cast<int32_t>(size.height)},theme_color(w.tcn,window::tv_background,w.theme_)))
            w.err=w.graphic_.get_error();
    }
    w.position_.right=w.position_.left+size.width;
    w.position_.bottom=w.position_.top+size.height;
    w.update_buttons();
    w.send_internal(internal_event_type::size_changed,size.width,size.height);
    [native.contentView setNeedsDisplay:YES];
}
void macos_window_backend::moved(window& w)
{
    NSWindow *native=(__bridge NSWindow *)w.context_.native_window;
    if(!native) return;
    w.position_.put(native.frame.origin.x,screen_top()-NSMaxY(native.frame));
    w.send_internal(internal_event_type::position_changed,w.position_.left,w.position_.top);
}
void macos_window_backend::mouse(window& w,mouse_event e)
{
    if(e.type==mouse_event_type::leave) {
        if(w.active_control) { event ev{};ev.type=event_type::mouse;ev.mouse_event_=e;
            w.send_event_to_control(w.active_control,ev); w.active_control.reset(); }
        w.send_event_to_plains({event_type::mouse,e});
    } else if(w.enabled_ || w.docked_control) w.send_mouse_event(e);
}
void macos_window_backend::keyboard(window& w,keyboard_event e)
{
    if(!w.enabled_ && !w.docked_control) return;
    if(auto dialog=std::dynamic_pointer_cast<window>(w.docked_control)) {
        keyboard(*dialog,e); return;
    }
    if(e.type==keyboard_event_type::down) {
        auto key=static_cast<uint8_t>(e.key[0]);
        if(key==vk_tab) {
            if(e.modifier==vk_lshift || e.modifier==vk_rshift) {
                std::vector<std::shared_ptr<i_control>> candidates;
                for(auto& c:w.controls) if(c->showed() && c->enabled() && c->focusing()) candidates.push_back(c);
                auto current=std::find(candidates.begin(),candidates.end(),w.get_focused());
                if(!candidates.empty()) w.set_focused(current==candidates.begin() || current==candidates.end()
                    ? candidates.back() : *std::prev(current));
            } else w.change_focus();
            return;
        }
        if(key==vk_return) {
            auto input=std::dynamic_pointer_cast<wui::input>(w.get_focused());
            if(!input || (input->get_input_view()!=input_view::multiline && w.default_push_control)) {
                w.execute_focused(); return;
            }
        }
    }
    event ev{}; ev.type=event_type::keyboard; ev.keyboard_event_=e;
    w.send_event_to_plains(ev);
    w.send_event_to_control(w.get_focused(),ev);
}
bool macos_window_backend::hit_control(window& w,int32_t x,int32_t y) { return w.check_control_here(x,y); }
bool macos_window_backend::accepts_input(window& w) { return w.enabled_ || w.docked_control; }
rect macos_window_backend::composition_rect(window& w)
{
    auto control=w.get_focused();
    while(auto child=std::dynamic_pointer_cast<window>(control)) control=child->get_focused();
    if(control) { auto p=control->position(); return {p.left+5,p.top+5,p.right-5,p.top+28}; }
    return {0,0,1,20};
}
bool macos_window_backend::resizable(window& w) { return w.enabled_ && flag_is_set(w.window_style_,window_style::resizable); }
bool macos_window_backend::movable(window& w) { return flag_is_set(w.window_style_,window_style::moving); }
void macos_window_backend::focus(window& w,bool focused)
{
    if(focused) {
        if(auto control=w.macos_saved_focus_.lock()) w.set_focused(control);
        w.macos_saved_focus_.reset();
    } else {
        auto control=w.get_focused();
        w.macos_saved_focus_=control;
        if(control) {
            event e{};e.type=event_type::internal;
            e.internal_event_={internal_event_type::remove_focus,0,0};
            w.send_event_to_control(control,e);
        }
        if(w.active_control) {
            event e{};e.type=event_type::mouse;e.mouse_event_={mouse_event_type::leave,0,0,0};
            w.send_event_to_control(w.active_control,e);
            w.active_control.reset();
        }
    }
}
}
