// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/media/devices.hpp>
#include "../../src/media/frame_rate.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#define CHECK(condition) do { if (!(condition)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); std::abort();} } while (0)
int main()
{
    using namespace wui::media;
    // A C270 advertises discrete 30/24/20/15/... rates, not a 5..30 range.
    detail::frame_rate_selection c270;
    for (double fps : {30., 24., 20., 15., 10., 7.5, 5.}) c270.consider(25, fps, fps);
    CHECK(c270.value == 24);
    CHECK(!c270.consider(25, 10, 10)); // A later, slower format cannot replace it.
    detail::frame_rate_selection continuous;
    CHECK(continuous.consider(25, 1, 30) && continuous.value == 25);
    detail::frame_rate_selection higher;
    CHECK(higher.consider(25, 60, 60));
    CHECK(higher.consider(25, 30, 30) && higher.value == 30);
    detail::frame_rate_selection fractional;
    CHECK(fractional.consider(8, 7.5, 7.5) && fractional.value == 7.5);
    CHECK(!fractional.consider(8, 0, 30));
    CHECK(!fractional.consider(8, 30, 1));
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
