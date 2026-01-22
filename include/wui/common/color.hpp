//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

namespace wui
{

typedef unsigned long color;

static inline color make_color(unsigned char red, unsigned char green, unsigned char blue)
{
#ifdef _WIN32
    return ((red) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(blue) << 16);
#elif __linux__
    return ((blue) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(red) << 16);
#endif
}

static inline color make_color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
#ifdef _WIN32
    return ((red) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(blue) << 16) | (static_cast<unsigned long>(alpha) << 24);
#elif __linux__
    return ((blue) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(red) << 16) | (static_cast<unsigned long>(alpha) << 24);
#endif
}

static inline unsigned char get_alpha(color rgb)
{
#ifdef _WIN32
    return (rgb >> 24) & 0xFF;
#else
    return  255 - ((rgb >> 24) & 0xFF);
#endif
}

static inline unsigned char get_red(color rgb)
{
    return (rgb >> 16) & 0xFF;
}

static inline unsigned char get_green(color rgb)
{
    return (rgb >> 8) & 0xFF;
}

static inline unsigned char get_blue(color rgb)
{
    return rgb & 0xFF;
}

}
