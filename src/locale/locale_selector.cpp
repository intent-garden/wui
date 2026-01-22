//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/locale/locale_selector.hpp>

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#endif

namespace wui
{

static locales_t instance;
static locale_type default_locale = locale_type::eng;
static size_t locale_pos = 0;

void set_app_locales(const locales_t &ls)
{
    instance = ls;
}

void set_default_locale(locale_type t)
{
    default_locale = t;
}

locale_params get_app_locale(locale_type t)
{
    auto l = std::find(instance.begin(), instance.end(), t);
    if (l != instance.end())
    {
        return *l;
    }

    /// We don't find the locale by type, try to return default locale
    l = std::find(instance.begin(), instance.end(), default_locale);
    if (l != instance.end())
    {
        return *l;
    }

    return {};
}

void set_current_app_locale(locale_type t)
{
    auto it = std::find(instance.begin(), instance.end(), t);
    if (it != instance.end())
    {
        locale_pos = it - instance.begin();
    }
}

locale_type get_next_app_locale()
{
    if (instance.empty())
    {
        return locale_type::eng;
    }

    ++locale_pos;
    if (locale_pos + 1 > instance.size())
    {
        locale_pos = 0;
    }

    return instance[locale_pos].type;
}

const locales_t &get_app_locales()
{
    return instance;
}

locale_type get_default_system_locale()
{
#ifdef _WIN32
    switch (GetUserDefaultUILanguage()) /// See the: https://www.autoitscript.com/autoit3/docs/appendix/OSLangCodes.htm
    {
    case 1033: return locale_type::eng;
    case 1049: return locale_type::rus;
    case 1087: return locale_type::kaz;

    /*case 1087: return locale_type::abk;
    case 1087: return locale_type::aar;
    case 1087: return locale_type::afr;
    case 1087: return locale_type::aka;
    case 1087: return locale_type::alb;
    case 1087: return locale_type::amh;
    case 1087: return locale_type::ara;
    case 1087: return locale_type::arg;
    case 1087: return locale_type::arm;
    case 1087: return locale_type::asm_;
    case 1087: return locale_type::ava;
    case 1087: return locale_type::ave;
    case 1087: return locale_type::aym;
    case 1087: return locale_type::aze;
    case 1087: return locale_type::bam;
    case 1087: return locale_type::bak;
    case 1087: return locale_type::baq;
    case 1087: return locale_type::bel;
    case 1087: return locale_type::ben;
    case 1087: return locale_type::bis;
    case 1087: return locale_type::bos;
    case 1087: return locale_type::bre;
    case 1087: return locale_type::bul;
    case 1087: return locale_type::bur;
    case 1087: return locale_type::cat;
    case 1087: return locale_type::cha;
    case 1087: return locale_type::che;
    case 1087: return locale_type::nya;
    case 1087: return locale_type::chi;
    case 1087: return locale_type::chu;
    case 1087: return locale_type::chv;
    case 1087: return locale_type::cor;
    case 1087: return locale_type::cos;
    case 1087: return locale_type::cre;
    case 1087: return locale_type::hrv;
    case 1087: return locale_type::cze;
    case 1087: return locale_type::dan;
    case 1087: return locale_type::div;
    case 1087: return locale_type::dut;
    case 1087: return locale_type::dzo;
    case 1087: return locale_type::eng;
    case 1087: return locale_type::epo;
    case 1087: return locale_type::est;
    case 1087: return locale_type::ewe;
    case 1087: return locale_type::fao;
    case 1087: return locale_type::fij;
    case 1087: return locale_type::fin;
    case 1087: return locale_type::fre;
    case 1087: return locale_type::fry;
    case 1087: return locale_type::ful;
    case 1087: return locale_type::gla;
    case 1087: return locale_type::glg;
    case 1087: return locale_type::lug;
    case 1087: return locale_type::geo;
    case 1087: return locale_type::ger;
    case 1087: return locale_type::gre;
    case 1087: return locale_type::kal;
    case 1087: return locale_type::grn;
    case 1087: return locale_type::guj;
    case 1087: return locale_type::hat;
    case 1087: return locale_type::hau;
    case 1087: return locale_type::heb;
    case 1087: return locale_type::her;
    case 1087: return locale_type::him;
    case 1087: return locale_type::hmo;
    case 1087: return locale_type::hun;
    case 1087: return locale_type::ice;
    case 1087: return locale_type::ido;
    case 1087: return locale_type::ibo;
    case 1087: return locale_type::ind;
    case 1087: return locale_type::ina;
    case 1087: return locale_type::ile;
    case 1087: return locale_type::iku;
    case 1087: return locale_type::ipk;
    case 1087: return locale_type::gle;
    case 1087: return locale_type::ita;
    case 1087: return locale_type::jpn;
    case 1087: return locale_type::jav;
    case 1087: return locale_type::kan;
    case 1087: return locale_type::kau;
    case 1087: return locale_type::kas;
    case 1087: return locale_type::kaz;
    case 1087: return locale_type::khm;
    case 1087: return locale_type::kik;
    case 1087: return locale_type::kin;
    case 1087: return locale_type::kir;
    case 1087: return locale_type::kom;
    case 1087: return locale_type::kon;
    case 1087: return locale_type::kor;
    case 1087: return locale_type::kua;
    case 1087: return locale_type::kur;
    case 1087: return locale_type::lao;
    case 1087: return locale_type::lat;
    case 1087: return locale_type::lav;
    case 1087: return locale_type::lim;
    case 1087: return locale_type::lin;
    case 1087: return locale_type::lit;
    case 1087: return locale_type::lut;
    case 1087: return locale_type::ltz;
    case 1087: return locale_type::mtz;
    case 1087: return locale_type::mac;
    case 1087: return locale_type::mlg;
    case 1087: return locale_type::may;
    case 1087: return locale_type::mal;
    case 1087: return locale_type::mit;
    case 1087: return locale_type::glv;
    case 1087: return locale_type::mao;
    case 1087: return locale_type::mar;
    case 1087: return locale_type::mah;
    case 1087: return locale_type::mon;
    case 1087: return locale_type::nau;
    case 1087: return locale_type::nav;
    case 1087: return locale_type::nde;
    case 1087: return locale_type::nbl;
    case 1087: return locale_type::ndo;
    case 1087: return locale_type::nep;
    case 1087: return locale_type::nor;
    case 1087: return locale_type::nob;
    case 1087: return locale_type::nno;
    case 1087: return locale_type::ili;
    case 1087: return locale_type::oci;
    case 1087: return locale_type::oji;
    case 1087: return locale_type::ori;
    case 1087: return locale_type::orm;
    case 1087: return locale_type::oss;
    case 1087: return locale_type::pli;
    case 1087: return locale_type::pus;
    case 1087: return locale_type::per;
    case 1087: return locale_type::pol;
    case 1087: return locale_type::por;
    case 1087: return locale_type::pan;
    case 1087: return locale_type::que;
    case 1087: return locale_type::rum;
    case 1087: return locale_type::roh;
    case 1087: return locale_type::run;
    case 1087: return locale_type::rus;
    case 1087: return locale_type::sme;
    case 1087: return locale_type::smo;
    case 1087: return locale_type::sag;
    case 1087: return locale_type::san;
    case 1087: return locale_type::srd;
    case 1087: return locale_type::srp;
    case 1087: return locale_type::sna;
    case 1087: return locale_type::snd;
    case 1087: return locale_type::sin;
    case 1087: return locale_type::slo;
    case 1087: return locale_type::slv;
    case 1087: return locale_type::som;
    case 1087: return locale_type::sot;
    case 1087: return locale_type::spa;
    case 1087: return locale_type::sun;
    case 1087: return locale_type::swa;
    case 1087: return locale_type::ssw;
    case 1087: return locale_type::swe;
    case 1087: return locale_type::tgl;
    case 1087: return locale_type::tah;
    case 1087: return locale_type::tgk;
    case 1087: return locale_type::tam;
    case 1087: return locale_type::tat;
    case 1087: return locale_type::tel;
    case 1087: return locale_type::tha;
    case 1087: return locale_type::tib;
    case 1087: return locale_type::tir;
    case 1087: return locale_type::ton;
    case 1087: return locale_type::tso;
    case 1087: return locale_type::tsn;
    case 1087: return locale_type::tur;
    case 1087: return locale_type::tuk;
    case 1087: return locale_type::twi;
    case 1087: return locale_type::uig;
    case 1087: return locale_type::ukr;
    case 1087: return locale_type::urd;
    case 1087: return locale_type::uzb;
    case 1087: return locale_type::ven;
    case 1087: return locale_type::vie;
    case 1087: return locale_type::vol;
    case 1087: return locale_type::wln;
    case 1087: return locale_type::wel;
    case 1087: return locale_type::wol;
    case 1087: return locale_type::xho;
    case 1087: return locale_type::yid;
    case 1087: return locale_type::yor;
    case 1087: return locale_type::zha;
    case 1087: return locale_type::zul;*/ // todo!
    }
#else
    
#endif

    return locale_type::eng;
}

}
