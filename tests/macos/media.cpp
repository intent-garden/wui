// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/media/devices.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#define CHECK(condition) do { if (!(condition)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); std::abort();} } while (0)
int main()
{
    using namespace wui::media;
    audio_stream audio;
    CHECK(!audio.start_playback("", {}));
    CHECK(!audio.error().empty());
    CHECK(!audio.start_capture("", {}));
    audio.stop(); audio.stop();
    video_capture video;
    CHECK(!video.start_camera("", {0,0}, 25, {}));
    CHECK(!video.start_screen({0,0}, 25, {}));
    video.stop(); video.stop();
    auto speakers = devices(device_kind::speaker);
    auto microphones = devices(device_kind::microphone);
    auto cameras = devices(device_kind::camera);
    std::printf("Devices: %zu output, %zu input, %zu cameras\n", speakers.size(), microphones.size(), cameras.size());
    if (speakers.empty()) {std::puts("No output hardware; playback lifecycle skipped"); return 0;}
    std::atomic<unsigned> blocks{0};
    for (unsigned restart = 0; restart < 3; ++restart) {
        CHECK(audio.start_playback("", [&](int16_t*, size_t samples) {CHECK(samples == 1920); ++blocks;}));
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        audio.stop();
        auto stopped = blocks.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(blocks == stopped);
    }
    CHECK(blocks > 9);
    std::puts("Media validation, playback restart and callback draining: OK");
}
