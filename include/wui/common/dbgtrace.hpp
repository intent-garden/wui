//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifdef _WIN32

#include <windows.h>   // OutputDebugStringA
#include <cstdio>      // _snprintf_s / std::snprintf

#ifndef NDEBUG   // Only in Debug-build

// Buffer size for debug trace
#ifndef DBG_BUFSIZE
#   define DBG_BUFSIZE 512
#endif

// Debug trace macro: TRACE("x=%d", x);
#define DBG_TRACE(fmt, ...)                                                           \
        do {                                                                          \
            char _dbgBuf[DBG_BUFSIZE];                                                \
            _snprintf_s(_dbgBuf, sizeof(_dbgBuf), _TRUNCATE,                          \
                        "%s(%d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
            OutputDebugStringA(_dbgBuf);                                              \
        } while (0)

#else          // In Release build no output

#define DBG_TRACE(...)    ((void)0)

#endif

#else // NO WIN32

#ifndef NDEBUG   // Only in Debug-build

// Buffer size for debug trace
#ifndef DBG_BUFSIZE
#   define DBG_BUFSIZE 512
#endif

// Debug trace macro: TRACE("x=%d", x);
#define DBG_TRACE(fmt, ...)                                                           \
        do {                                                                          \
            char _dbgBuf[DBG_BUFSIZE];                                                \
            std::snprintf(_dbgBuf, sizeof(_dbgBuf),                                   \
                        "%s(%d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
            printf(_dbgBuf);                                                          \
        } while (0)

#else          // In Release build no output

#define DBG_TRACE(...)    ((void)0)

#endif

#endif // WIN32
