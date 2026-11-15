# Native media devices on macOS (experimental)

Enable `WUI_ENABLE_MEDIA=ON` in CMake and link `wui`. The optional API lives in
`wui/media/devices.hpp`; enabled macOS builds export `WUI_HAS_NATIVE_MEDIA=1`.
The option is off by default and adds no media dependencies to ordinary WUI builds.
Windows and Linux media backends are not implemented yet.

The API provides device enumeration with persistent OS IDs, microphone capture,
audio playback, camera capture and primary-display capture. It does not depend on
RTP, codecs, conference state or an audio-processing library. Applications own
AEC/AGC/noise suppression and pass PCM/video frames across this boundary.

## Audio

`audio_stream` captures or renders signed 16-bit interleaved stereo PCM at 48 kHz.
A playback callback fills 1,920 samples (960 stereo frames / 20 ms). Capture can
return short blocks; buffer these if downstream processing requires fixed blocks.
Playback buffers are cleared before each callback. An empty device ID selects the
system default; use `devices(device_kind::microphone/speaker)` for explicit IDs.
Each stream owns a separate AudioQueue. `start_*` first stops the previous stream.

## Camera and screen

`video_capture::start_camera()` selects a camera by ID and exact dimensions. The frame rate is a
target: choose the highest supported rate at or below it, or the lowest supported
rate above it if no lower rate exists. Thus a camera advertising discrete
30/24/20 FPS starts at 24 FPS when 25 FPS is requested. `device::sizes` lists dimensions, but frame-rate negotiation happens when
starting. Unsupported resolutions fail with `error()` rather than silently changing
size. Callbacks receive top-down BGRA pixels with a row stride; do not assume rows
are tightly packed. `start_screen()` scales the primary display to the requested
size using ScreenCaptureKit and requires macOS 12.3 or newer.

## Permissions and lifetime

Add `NSCameraUsageDescription` and `NSMicrophoneUsageDescription` to the app's
Info.plist. When access has not been decided, starting capture requests permission
and returns false. Retry after the user grants access. Screen recording needs the
macOS Screen Recording permission. `error()` describes start failures.

Callbacks run on media threads and borrow buffers only until they return. They
must not throw or directly modify WUI controls; use `window::emit_event()` to hand
work to the main thread. `stop()` drains callbacks synchronously, as does the
destructor. Do not stop/destroy a stream from its own callback. Start/stop/device
changes must be serialized by the application's control thread. Device unplug
notifications and runtime error callbacks are not part of this initial API.

## Verification

```sh
cmake -S . -B build-media -DWUI_ENABLE_MEDIA=ON -DWUI_BUILD_TESTS=ON -DWUI_BUILD_EXAMPLES=OFF
cmake --build build-media --target wui_macos_test wui_macos_media_test
ctest --test-dir build-media -R '^macos_(integration|media)$' --output-on-failure
```

The media test enumerates devices, checks invalid arguments, runs silent playback
through repeated starts/stops, and verifies callbacks stop after `stop()` returns.
It does not request microphone/camera/screen permissions. Capture and actual sound
quality require a manual check on hardware.
