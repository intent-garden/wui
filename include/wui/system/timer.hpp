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
#include <windows.h>
#endif

#include <atomic>
#include <functional>
#include <exception>
#include <cstdint>

namespace wui
{

#ifndef _WIN32
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
class timer
{
public:
	explicit timer(std::function<void(void)> callback_)
		: callback(callback_), h_timer(nullptr), h_timer_queue(nullptr), started(false)
	{
	}

	~timer()
	{
		stop();
	}

	void start(const uint32_t interval = 1000)
	{
		if (started || h_timer_queue)
		{
			return;
		}
		
        h_timer_queue = CreateTimerQueue();
		if (!h_timer_queue)
		{
			throw std::exception("Error CreateTimerQueue()");
		}

		if (!CreateTimerQueueTimer(&h_timer, h_timer_queue,
			(WAITORTIMERCALLBACK)TimerRoutine, this, 0, interval, WT_EXECUTEDEFAULT))
		{
			throw std::exception("Error CreateTimerQueueTimer()");
		}
		started = true;
	}

	void stop()
	{
		if (!started)
		{
			return;
		}
				
		if (h_timer && h_timer_queue)
		{
			DeleteTimerQueueTimer(h_timer_queue, h_timer, INVALID_HANDLE_VALUE);
            h_timer = nullptr;
		}
		if (h_timer_queue)
		{
			DeleteTimerQueueEx(h_timer_queue, INVALID_HANDLE_VALUE);
            h_timer_queue = nullptr;
		}
		started = false;
	}

	timer(const timer&) = delete;
	timer& operator=(const timer&) = delete;

private:
	std::function<void(void)> callback;

	HANDLE h_timer;
    HANDLE h_timer_queue;

	std::atomic<bool> started;
	
	static VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN)
	{
		static_cast<timer*>(lpParam)->callback();
	}
};
#endif

}
