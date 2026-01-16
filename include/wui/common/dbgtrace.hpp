//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#pragma once

#ifdef _WIN32

#include <windows.h>   // OutputDebugStringA
#include <cstdio>      // _snprintf_s / std::snprintf

#ifndef NDEBUG   // активен в Debug-сборке

// Максимальная длина одной строки трассировки
#ifndef DBG_BUFSIZE
#   define DBG_BUFSIZE 512
#endif

// Вариадический макрос: TRACE("x=%d", x);
#define DBG_TRACE(fmt, ...)                                                           \
        do {                                                                          \
            char _dbgBuf[DBG_BUFSIZE];                                                \
            _snprintf_s(_dbgBuf, sizeof(_dbgBuf), _TRUNCATE,                          \
                        "%s(%d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
            OutputDebugStringA(_dbgBuf);                                              \
        } while (0)

#else          // …в Release ничего не генерируем

#define DBG_TRACE(...)    ((void)0)

#endif

#else // NO WIN32

#ifndef NDEBUG   // активен в Debug-сборке

// Максимальная длина одной строки трассировки
#ifndef DBG_BUFSIZE
#   define DBG_BUFSIZE 512
#endif

// Вариадический макрос: TRACE("x=%d", x);
#define DBG_TRACE(fmt, ...)                                                           \
        do {                                                                          \
            char _dbgBuf[DBG_BUFSIZE];                                                \
            _snprintf_s(_dbgBuf, sizeof(_dbgBuf), _TRUNCATE,                          \
                        "%s(%d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
            printf(_dbgBuf);                                              \
        } while (0)

#else          // …в Release ничего не генерируем

#define DBG_TRACE(...)    ((void)0)

#endif

#endif // WIN32