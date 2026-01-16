//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://gitverse.ru/udattsk/wui
//

#include <wui/system/uri_tools.hpp>

#ifdef _WIN32

#include <windows.h>

#include <boost/nowide/convert.hpp>

#elif __linux__

#include <stdlib.h>

#endif

namespace wui
{

bool open_uri(std::string_view uri)
{
#ifdef _WIN32
    return reinterpret_cast<int64_t>(ShellExecuteW(NULL, L"open", boost::nowide::widen(uri).c_str(), NULL, NULL, SW_SHOW)) < 32;
#else
    std::string cmd =
#ifdef __APPLE__
        std::string("open ")
#else
        std::string("xdg-open ")
#endif
        + std::string(uri);

    auto res = system(cmd.c_str());
    return res == 0;
#endif        
}

}
