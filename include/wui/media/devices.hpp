// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace wui::media
{
// Optional native media API. Currently available on macOS.
// IDs are persistent OS device IDs. An empty ID selects the default device.
enum class device_kind { microphone, speaker, camera };
struct video_size { int width = 0; int height = 0; };
struct device
{
    std::string id;
    std::string name;
    std::vector<video_size> sizes;
};
std::vector<device> devices(device_kind kind);

// Callbacks run on media threads. Buffers are borrowed until the callback returns.
// stop() synchronously drains callbacks; do not call it from a media callback.
// Callback code must not throw or call window/control methods directly.
// PCM is signed 16-bit, interleaved stereo, 48 kHz, in 20 ms blocks.
class audio_stream
{
public:
    using capture_callback = std::function<void(const int16_t*, size_t)>;
    using render_callback = std::function<void(int16_t*, size_t)>;
    audio_stream();
    ~audio_stream();
    audio_stream(const audio_stream&) = delete;
    audio_stream& operator=(const audio_stream&) = delete;
    bool start_capture(const std::string& id, capture_callback callback);
    bool start_playback(const std::string& id, render_callback callback);
    void stop();
    const std::string& error() const;
private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

// Camera and screen callbacks deliver top-down BGRA pixels, including row stride.
class video_capture
{
public:
    using callback = std::function<void(const uint8_t*, int, int, size_t)>;
    video_capture();
    ~video_capture();
    video_capture(const video_capture&) = delete;
    video_capture& operator=(const video_capture&) = delete;
    // Exact dimensions; fps is a target, negotiated against the camera ranges.
    bool start_camera(const std::string& id, video_size size, unsigned fps, callback receive);
    bool start_screen(video_size size, unsigned fps, callback receive);
    void stop();
    const std::string& error() const;
private:
    struct impl;
    std::unique_ptr<impl> impl_;
};
}
