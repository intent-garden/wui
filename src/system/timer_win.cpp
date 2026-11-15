// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#ifdef _WIN32
#include <wui/system/timer.hpp>
#include <mutex>
#include <system_error>
#include <utility>

namespace wui
{
namespace
{
constexpr UINT apply_timer = WM_APP + 1;
constexpr UINT destroy_timer = WM_APP + 2;
constexpr wchar_t timer_class[] = L"WUI.Internal.Timer.v1";
std::atomic<UINT_PTR> next_id{1};
}

struct timer::state
{
    std::recursive_mutex mutex;
    std::function<void()> callback;
    std::atomic<HWND> window{nullptr};
    const DWORD owner_thread = GetCurrentThreadId();
    std::shared_ptr<state> window_owner;
    uint64_t generation = 0, armed_generation = 0;
    UINT_PTR id = 0;
    uint32_t interval = 1000;
    bool active = false, executing = false;

    // Called only by the owner thread. Each restart uses a new native id:
    // KillTimer does not remove already queued WM_TIMER messages.
    DWORD apply(HWND hwnd)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (active && id && armed_generation == generation) return ERROR_SUCCESS;
        if (id) KillTimer(hwnd, id);
        id = 0;
        if (active)
        {
            // Also avoid id reuse across instances when Windows recycles HWNDs.
            auto token = next_id.fetch_add(1);
            if (!token) token = next_id.fetch_add(1);
            if (!SetTimer(hwnd, token, interval ? interval : 1, nullptr))
            {
                active = false;
                const auto error = GetLastError();
                return error ? error : ERROR_NOT_ENOUGH_MEMORY;
            }
            id = token;
            armed_generation = generation;
        }
        return ERROR_SUCCESS;
    }

    DWORD notify()
    {
        auto hwnd = window.load();
        if (!hwnd) return ERROR_INVALID_WINDOW_HANDLE;
        if (GetCurrentThreadId() == owner_thread) return apply(hwnd);
        return PostMessageW(hwnd, apply_timer, 0, 0) ? ERROR_SUCCESS : GetLastError();
    }

    static LRESULT CALLBACK proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        auto* raw = reinterpret_cast<state*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            raw = static_cast<state*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(raw));
            raw->window = hwnd;
        }
        // Native ownership survives timer destruction on another thread and
        // callback self-destruction, until WM_NCDESTROY releases it.
        auto shared = raw ? raw->window_owner : nullptr;
        if (shared)
        {
            if (message == apply_timer)
            {
                shared->apply(hwnd);
                return 0;
            }
            if (message == destroy_timer)
            {
                DestroyWindow(hwnd);
                return 0;
            }
            if (message == WM_TIMER)
            {
                std::lock_guard<std::recursive_mutex> lock(shared->mutex);
                if (shared->active && !shared->executing && shared->id == wparam &&
                    shared->armed_generation == shared->generation)
                {
                    // Keep the callable alive if it destroys its timer. Nested
                    // modal message loops must not re-enter the same callback.
                    auto callback = shared->callback;
                    shared->executing = true;
                    if (callback) callback();
                    shared->executing = false;
                }
                return 0;
            }
            if (message == WM_NCDESTROY)
            {
                std::lock_guard<std::recursive_mutex> lock(shared->mutex);
                shared->active = false;
                shared->id = 0;
                shared->window = nullptr;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                shared->window_owner.reset();
            }
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
};

timer::timer(std::function<void()> callback) : state_(std::make_shared<state>())
{
    static std::once_flag registered;
    std::call_once(registered, [] {
        WNDCLASSW cls{};
        cls.lpfnWndProc = state::proc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.lpszClassName = timer_class;
        if (!RegisterClassW(&cls))
            throw std::system_error(GetLastError(), std::system_category(), "Register WUI timer window");
    });
    state_->callback = std::move(callback);
    state_->window_owner = state_;
    if (!CreateWindowExW(0, timer_class, L"", 0, 0, 0, 0, 0, HWND_MESSAGE,
        nullptr, GetModuleHandleW(nullptr), state_.get()))
    {
        const auto error = GetLastError();
        state_->window_owner.reset();
        throw std::system_error(error, std::system_category(), "Create WUI timer window");
    }
}

timer::~timer()
{
    stop();
    const auto shared = state_;
    if (auto hwnd = shared->window.load())
    {
        if (GetCurrentThreadId() == shared->owner_thread) DestroyWindow(hwnd);
        else PostMessageW(hwnd, destroy_timer, 0, 0);
    }
}

void timer::start(uint32_t interval)
{
    const auto shared = state_;
    {
        std::lock_guard<std::recursive_mutex> lock(shared->mutex);
        if (shared->active) return;
        shared->interval = interval;
        shared->active = true;
        ++shared->generation;
    }
    if (const auto error = shared->notify())
    {
        std::lock_guard<std::recursive_mutex> lock(shared->mutex);
        shared->active = false;
        ++shared->generation;
        throw std::system_error(error, std::system_category(), "Start WUI timer");
    }
}

void timer::stop()
{
    const auto shared = state_;
    {
        // Wait for a running callback on other threads; allow self-stop and
        // self-destruction from the owner callback through the recursive lock.
        std::lock_guard<std::recursive_mutex> lock(shared->mutex);
        if (!shared->active) return;
        shared->active = false;
        ++shared->generation;
    }
    // Cancellation of the native timer may be queued, but its stale messages
    // cannot call user code after active/generation were invalidated above.
    shared->notify();
}
}
#endif
