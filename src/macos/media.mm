// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/media/devices.hpp>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

namespace
{
std::string utf8(NSString* value) { return value ? std::string(value.UTF8String) : std::string(); }
NSString* ns(const std::string& value) { return [NSString stringWithUTF8String:value.c_str()]; }
bool authorized(AVMediaType type, std::string& error)
{
    auto status = [AVCaptureDevice authorizationStatusForMediaType:type];
    if (status == AVAuthorizationStatusAuthorized) return true;
    if (status == AVAuthorizationStatusNotDetermined)
        [AVCaptureDevice requestAccessForMediaType:type completionHandler:^(BOOL) {}];
    error = "Allow microphone/camera access in System Settings, then start the device again";
    return false;
}
std::string audio_string(AudioDeviceID id, AudioObjectPropertySelector selector)
{
    AudioObjectPropertyAddress address{selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    CFStringRef value = nullptr; UInt32 bytes = sizeof(value);
    if (AudioObjectGetPropertyData(id, &address, 0, nullptr, &bytes, &value) != noErr || !value) return {};
    std::string result = utf8((__bridge NSString*)value);
    CFRelease(value);
    return result;
}
}

@interface WuiVideoReceiver : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate, SCStreamOutput>
@property(nonatomic, copy) void (^receive)(CMSampleBufferRef);
@end
@implementation WuiVideoReceiver
- (void)captureOutput:(AVCaptureOutput*)output didOutputSampleBuffer:(CMSampleBufferRef)sample fromConnection:(AVCaptureConnection*)connection
{
    (void)output; (void)connection;
    if (self.receive) self.receive(sample);
}
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sample ofType:(SCStreamOutputType)type
{
    (void)stream;
    if (type == SCStreamOutputTypeScreen && CMSampleBufferIsValid(sample) && self.receive) self.receive(sample);
}
@end

namespace wui::media
{
std::vector<device> devices(device_kind kind)
{
    @autoreleasepool {
        std::vector<device> result;
        if (kind == device_kind::camera) {
            NSMutableArray<AVCaptureDeviceType>* types = [NSMutableArray arrayWithObject:AVCaptureDeviceTypeBuiltInWideAngleCamera];
            if (@available(macOS 14.0, *)) [types addObject:AVCaptureDeviceTypeExternal];
            else [types addObject:AVCaptureDeviceTypeExternalUnknown];
            if (@available(macOS 13.0, *)) [types addObject:AVCaptureDeviceTypeContinuityCamera];
            auto discovery = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:types mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionUnspecified];
            for (AVCaptureDevice* item in discovery.devices) {
                device entry{utf8(item.uniqueID), utf8(item.localizedName), {}};
                for (AVCaptureDeviceFormat* format in item.formats) {
                    auto size = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
                    if (std::none_of(entry.sizes.begin(), entry.sizes.end(), [&](video_size s) {return s.width == size.width && s.height == size.height;}))
                        entry.sizes.push_back({size.width, size.height});
                }
                result.push_back(std::move(entry));
            }
            return result;
        }
        AudioObjectPropertyAddress address{kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
        UInt32 bytes = 0;
        if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &bytes) != noErr) return result;
        std::vector<AudioDeviceID> ids(bytes / sizeof(AudioDeviceID));
        if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &bytes, ids.data()) != noErr) return result;
        for (auto id : ids) {
            AudioObjectPropertyAddress streams{kAudioDevicePropertyStreams,
                kind == device_kind::microphone ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                kAudioObjectPropertyElementMain};
            UInt32 count = 0;
            if (AudioObjectGetPropertyDataSize(id, &streams, 0, nullptr, &count) == noErr && count)
                result.push_back({audio_string(id, kAudioDevicePropertyDeviceUID), audio_string(id, kAudioObjectPropertyName), {}});
        }
        return result;
    }
}

struct audio_stream::impl
{
    AudioQueueRef queue = nullptr;
    std::atomic<bool> running{false};
    capture_callback capture;
    render_callback render;
    std::string error;
    static constexpr UInt32 block_bytes = 960 * 2 * sizeof(int16_t);
    static void input(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer, const AudioTimeStamp*, UInt32, const AudioStreamPacketDescription*)
    {
        auto& self = *static_cast<impl*>(context);
        if (!self.running) return;
        try { self.capture(static_cast<const int16_t*>(buffer->mAudioData), buffer->mAudioDataByteSize / sizeof(int16_t)); } catch (...) {}
        if (self.running) AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }
    static void output(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer)
    {
        auto& self = *static_cast<impl*>(context);
        if (!self.running) return;
        std::memset(buffer->mAudioData, 0, block_bytes);
        try { self.render(static_cast<int16_t*>(buffer->mAudioData), block_bytes / sizeof(int16_t)); } catch (...) {}
        buffer->mAudioDataByteSize = block_bytes;
        if (self.running) AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }
    void stop()
    {
        running = false;
        if (queue) { AudioQueueStop(queue, true); AudioQueueDispose(queue, true); queue = nullptr; }
        capture = {}; render = {};
    }
    bool start(const std::string& id, bool recording)
    {
        error.clear();
        if (recording && !authorized(AVMediaTypeAudio, error)) return false;
        AudioStreamBasicDescription format{48000, kAudioFormatLinearPCM,
            kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked, 4, 1, 4, 2, 16, 0};
        OSStatus status = recording
            ? AudioQueueNewInput(&format, input, this, nullptr, nullptr, 0, &queue)
            : AudioQueueNewOutput(&format, output, this, nullptr, nullptr, 0, &queue);
        if (status == noErr && !id.empty()) {
            CFStringRef uid = (__bridge CFStringRef)ns(id);
            status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &uid, sizeof(uid));
        }
        running = status == noErr;
        for (unsigned i = 0; running && i < 3; ++i) {
            AudioQueueBufferRef buffer = nullptr;
            status = AudioQueueAllocateBuffer(queue, block_bytes, &buffer);
            if (status != noErr) break;
            if (recording) status = AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
            else output(this, queue, buffer);
            if (status != noErr) break;
        }
        if (status == noErr) status = AudioQueueStart(queue, nullptr);
        if (status != noErr) { error = "AudioQueue failed: " + std::to_string(status); stop(); return false; }
        return true;
    }
};
audio_stream::audio_stream() : impl_(new impl) {}
audio_stream::~audio_stream() { stop(); }
bool audio_stream::start_capture(const std::string& id, capture_callback callback)
{ stop(); if (!callback) {impl_->error = "Capture callback is required"; return false;} impl_->capture = std::move(callback); return impl_->start(id, true); }
bool audio_stream::start_playback(const std::string& id, render_callback callback)
{ stop(); if (!callback) {impl_->error = "Playback callback is required"; return false;} impl_->render = std::move(callback); return impl_->start(id, false); }
void audio_stream::stop() { impl_->stop(); }
const std::string& audio_stream::error() const { return impl_->error; }

struct video_capture::impl
{
    AVCaptureSession* session = nil;
    WuiVideoReceiver* delegate = nil;
    dispatch_queue_t queue = dispatch_queue_create("wui.camera", DISPATCH_QUEUE_SERIAL);
    std::shared_ptr<std::atomic<bool>> accepting = std::make_shared<std::atomic<bool>>(false);
    SCStream* screen = nil;
    std::string error;
    void stop()
    {
        *accepting = false;
        if (session) {
            [session stopRunning];
            for (AVCaptureOutput* output in session.outputs)
                if ([output isKindOfClass:AVCaptureVideoDataOutput.class]) [(AVCaptureVideoDataOutput*)output setSampleBufferDelegate:nil queue:nullptr];
            dispatch_sync(queue, ^{});
            delegate.receive = nil; delegate = nil; session = nil;
        }
        if (@available(macOS 12.3, *)) {
            if (screen) {
                dispatch_semaphore_t finished = dispatch_semaphore_create(0);
                [screen stopCaptureWithCompletionHandler:^(NSError*) {dispatch_semaphore_signal(finished);}];
                [screen removeStreamOutput:delegate type:SCStreamOutputTypeScreen error:nil];
                dispatch_semaphore_wait(finished, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
                dispatch_sync(queue, ^{});
                screen = nil;
            }
        }
        delegate.receive = nil; delegate = nil;
    }
};
video_capture::video_capture() : impl_(new impl) {}
video_capture::~video_capture() { stop(); }
void video_capture::stop() { impl_->stop(); }
const std::string& video_capture::error() const { return impl_->error; }
bool video_capture::start_camera(const std::string& id, video_size size, unsigned fps, callback receive)
{
    stop(); impl_->error.clear();
    if (!receive || size.width <= 0 || size.height <= 0 || fps == 0) {impl_->error = "Invalid capture parameters"; return false;}
    if (!authorized(AVMediaTypeVideo, impl_->error)) return false;
    AVCaptureDevice* camera = id.empty() ? [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo] : [AVCaptureDevice deviceWithUniqueID:ns(id)];
    if (!camera) {impl_->error = "Camera not found"; return false;}
    NSError* error = nil;
    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:camera error:&error];
    if (!input) {impl_->error = utf8(error.localizedDescription); return false;}
    AVCaptureDeviceFormat* selected = nil;
    for (AVCaptureDeviceFormat* format in camera.formats) {
        auto d = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
        for (AVFrameRateRange* range in format.videoSupportedFrameRateRanges)
            if (d.width == size.width && d.height == size.height && range.minFrameRate <= fps && range.maxFrameRate >= fps) selected = format;
    }
    if (!selected) {impl_->error = "Requested camera format/frame rate is unavailable"; return false;}
    AVCaptureSession* session = [AVCaptureSession new];

    [session beginConfiguration];
    if (![session canAddInput:input]) {impl_->error = "Cannot add camera input"; return false;}
    [session addInput:input];
    if (![camera lockForConfiguration:&error]) {impl_->error = utf8(error.localizedDescription); return false;}
    camera.activeFormat = selected;
    camera.activeVideoMinFrameDuration = CMTimeMake(1, fps);
    camera.activeVideoMaxFrameDuration = CMTimeMake(1, fps);
    [camera unlockForConfiguration];
    AVCaptureVideoDataOutput* output = [AVCaptureVideoDataOutput new];
    output.alwaysDiscardsLateVideoFrames = YES;
    output.videoSettings = @{(__bridge NSString*)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
    if (![session canAddOutput:output]) {impl_->error = "Cannot add camera output"; return false;}
    [session addOutput:output];
    [session commitConfiguration];
    impl_->delegate = [WuiVideoReceiver new];
    auto accepting = impl_->accepting = std::make_shared<std::atomic<bool>>(true);
    impl_->delegate.receive = ^(CMSampleBufferRef sample) {
        if (!*accepting) return;
        CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
        if (!pixels || CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) return;
        try {receive(static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels)),
            static_cast<int>(CVPixelBufferGetWidth(pixels)), static_cast<int>(CVPixelBufferGetHeight(pixels)), CVPixelBufferGetBytesPerRow(pixels));} catch (...) {}
        CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    };
    [output setSampleBufferDelegate:impl_->delegate queue:impl_->queue];
    impl_->session = session;
    [session startRunning];
    if (!session.isRunning) {impl_->error = "Camera failed to start"; stop(); return false;}
    return true;
}
bool video_capture::start_screen(video_size size, unsigned fps, callback receive)
{
    stop(); impl_->error.clear();
    if (!receive || size.width <= 0 || size.height <= 0 || fps == 0 || fps > 120) {impl_->error = "Invalid screen capture parameters"; return false;}
    if (!CGPreflightScreenCaptureAccess()) {CGRequestScreenCaptureAccess(); impl_->error = "Allow Screen Recording in System Settings"; return false;}
    if (@available(macOS 12.3, *)) {
        dispatch_semaphore_t contentReady = dispatch_semaphore_create(0);
        __block SCShareableContent* content = nil;
        __block NSError* failure = nil;
        [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:YES
            completionHandler:^(SCShareableContent* value, NSError* error) {content = value; failure = error; dispatch_semaphore_signal(contentReady);}];
        if (dispatch_semaphore_wait(contentReady, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)) != 0) {
            impl_->error = "Timed out enumerating screens"; return false;
        }
        SCDisplay* display = content.displays.firstObject;
        if (!display) {impl_->error = failure ? utf8(failure.localizedDescription) : "No display available"; return false;}
        SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
        SCStreamConfiguration* config = [SCStreamConfiguration new];
        config.width = size.width; config.height = size.height;
        config.pixelFormat = kCVPixelFormatType_32BGRA;
        config.minimumFrameInterval = CMTimeMake(1, fps); config.queueDepth = 3;
        SCStream* stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:nil];
        impl_->delegate = [WuiVideoReceiver new];
        auto accepting = impl_->accepting = std::make_shared<std::atomic<bool>>(true);
        impl_->delegate.receive = ^(CMSampleBufferRef sample) {
            if (!*accepting) return;
            CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
            if (!pixels || CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) return;
            try { receive(static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels)),
                static_cast<int>(CVPixelBufferGetWidth(pixels)), static_cast<int>(CVPixelBufferGetHeight(pixels)), CVPixelBufferGetBytesPerRow(pixels)); } catch (...) {}
            CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
        };
        NSError* addError = nil;
        if (![stream addStreamOutput:impl_->delegate type:SCStreamOutputTypeScreen sampleHandlerQueue:impl_->queue error:&addError]) {
            impl_->error = utf8(addError.localizedDescription); impl_->delegate = nil; return false;
        }
        impl_->screen = stream;
        dispatch_semaphore_t started = dispatch_semaphore_create(0);
        [stream startCaptureWithCompletionHandler:^(NSError* error) {failure = error; dispatch_semaphore_signal(started);}];
        if (dispatch_semaphore_wait(started, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)) != 0) {
            impl_->error = "Timed out starting screen capture"; stop(); return false;
        }
        if (failure) {impl_->error = utf8(failure.localizedDescription); stop(); return false;}
        return true;
    }
    impl_->error = "Screen capture requires macOS 12.3 or newer";
    return false;
}
}
