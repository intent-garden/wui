//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifndef _WIN32
#include <chrono>
#include <thread>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <atomic>
#include <functional>
#include <exception>
#include <cstdint>
#include <memory>

namespace wui
{

#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
class timer
{
public:
    explicit timer(std::function<void(void)> callback);
    ~timer();
    void start(uint32_t interval = 1000);
    void stop();
    timer(const timer&) = delete;
    timer& operator=(const timer&) = delete;
private:
    std::function<void(void)> callback_;
    void *native_timer_ = nullptr;
};
#elif !defined(_WIN32)
class timer
{
public:
	explicit timer(std::function<void(void)> callback_)
		: started(false), thread(), callback(callback_), interval(1000)
	{
	}

	~timer()
	{
		stop();
	}

	void start(const uint32_t interval_ = 1000 /* in milliseconds */)
	{
		if (!started)
		{
			interval = interval_;
			started = true;

			thread = std::thread(&timer::run, this);
		}
	}

	void stop()
	{
		if (started)
		{
			started = false;
			if (thread.joinable()) thread.join();
		}
	}

	timer(const timer&) = delete;
	timer& operator=(const timer&) = delete;

private:
	std::atomic<bool> started;
	std::thread thread;
	std::function<void(void)> callback;

	uint32_t interval;

	void run()
	{
	    auto begin = std::chrono::high_resolution_clock::now();

		while (started)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin).count();
			if (elapsed > interval * 1000)
			{
			    callback();
			    begin = std::chrono::high_resolution_clock::now();
			}
			else
			{
			    std::this_thread::sleep_for(std::chrono::microseconds(100));
			}
		}
	}
};
#else
/// Windows UI timer. Construct on the thread that owns the message loop.
/// start/stop may be called from workers; callback delivery remains on the
/// owning thread. stop waits for an in-flight callback, and is safe inside it.
class timer
{
public:
    explicit timer(std::function<void()> callback);
    ~timer();
    void start(uint32_t interval = 1000);
    void stop();
    timer(const timer&) = delete;
    timer& operator=(const timer&) = delete;
private:
    struct state;
    std::shared_ptr<state> state_;
};
#endif

}
