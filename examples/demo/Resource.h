//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#ifdef _WIN32

#define IDI_DEMO			    107
#define IMG_LOGO				109

#define IMG_ACCOUNT     		110
#define IMG_MENU    		    111

#define TXT_DARK_THEME          200
#define TXT_LIGHT_THEME         201

#define TXT_LOCALE_EN           210
#define TXT_LOCALE_RU           211

#else // _WIN32

static int TXT_LOCALE_EN = 0, TXT_LOCALE_RU = 0, TXT_DARK_THEME = 0, TXT_LIGHT_THEME = 0;

static constexpr const char* IMG_LOGO               = "logo.png";
static constexpr const char* IMG_ACCOUNT            = "account.png";
static constexpr const char* IMG_MENU               = "menu.png";

#endif
