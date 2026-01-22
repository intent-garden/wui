//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <cstdint>
#include <array>

namespace wui
{

enum class keyboard_event_type
{
    down, 
    up,
    key
};

#ifdef _WIN32
static const uint8_t vk_tab = 0x09;
static const uint8_t vk_return = 0x0D;
static const uint8_t vk_rreturn = 0;
static const uint8_t vk_esc = 0x1B;

static const uint8_t vk_back = 0x08;
static const uint8_t vk_del = 0x2E;

static const uint8_t vk_end = 0x23;
static const uint8_t vk_nend = 0x61;
static const uint8_t vk_home = 0x24;
static const uint8_t vk_nhome = 0x67;
static const uint8_t vk_page_up = 0x21;
static const uint8_t vk_npage_up = 0x69;
static const uint8_t vk_page_down = 0x22;
static const uint8_t vk_npage_down = 0x63;

static const uint8_t vk_up = 0x26;
static const uint8_t vk_nup = 0x68;
static const uint8_t vk_down = 0x28;
static const uint8_t vk_ndown = 0x62;
static const uint8_t vk_left = 0x25;
static const uint8_t vk_nleft = 0x64;
static const uint8_t vk_right = 0x27;
static const uint8_t vk_nright = 0x66;

/// The modifier keys
static const uint8_t vk_capital = 0x14;
static const uint8_t vk_lshift = 0x10;
static const uint8_t vk_rshift = 0x10;
static const uint8_t vk_alt = 0x12;
static const uint8_t vk_lcontrol = 0xA2;
static const uint8_t vk_rcontrol = 0xA3;
static const uint8_t vk_insert = 0x2D;
static const uint8_t vk_numlock = 0x90;
#elif __linux__
static const uint8_t vk_tab = 0x17;
static const uint8_t vk_return = 0x24;
static const uint8_t vk_rreturn = 0x68;
static const uint8_t vk_esc = 0x09;

static const uint8_t vk_back = 0x16;
static const uint8_t vk_del = 0x77;

static const uint8_t vk_end = 0x73;
static const uint8_t vk_nend = 0x57;
static const uint8_t vk_home = 0x6E;
static const uint8_t vk_nhome = 0x4F;
static const uint8_t vk_page_up = 0x70;
static const uint8_t vk_npage_up = 0x51;
static const uint8_t vk_page_down = 0x75;
static const uint8_t vk_npage_down = 0x59;

static const uint8_t vk_up = 0x6F;
static const uint8_t vk_nup = 0x50;
static const uint8_t vk_down = 0x74;
static const uint8_t vk_ndown = 0x58;
static const uint8_t vk_left = 0x71;
static const uint8_t vk_nleft = 0x53;
static const uint8_t vk_right = 0x72;
static const uint8_t vk_nright = 0x55;

/// The modifier keys
static const uint8_t vk_capital = 0x02;
static const uint8_t vk_lshift = 0x32;
static const uint8_t vk_rshift = 0x3E;
static const uint8_t vk_alt = 0x40;
static const uint8_t vk_lcontrol = 0x25;
static const uint8_t vk_rcontrol = 0x69;
static const uint8_t vk_insert = 0x76;
static const uint8_t vk_numlock = 0x4D;
#endif

struct keyboard_event
{
    keyboard_event_type type;

    uint8_t modifier; /// vk_capital or vk_shift or vk_alt or vk_insert
    char key[4];
    uint8_t key_size;
};

}
