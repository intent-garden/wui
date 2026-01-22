//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#ifdef _WIN32

#include <wui/config/config_impl_reg.hpp>
#include <wui/system/tools.hpp>

#include <boost/nowide/convert.hpp>

#include <windows.h>
#include <winreg.h>

namespace wui
{

namespace config
{

config_impl_reg::config_impl_reg(std::string_view base_application_key_, HKEY root_)
    : base_application_key(base_application_key_), root(root_)
{
}

config_impl_reg::~config_impl_reg()
{
}

int32_t config_impl_reg::get_int(std::string_view section, std::string_view entry, int32_t default_)
{
    // base_application_key\\section
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        root,
        wKey.c_str(),
        0,
        KEY_READ,
        &hKey
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        DWORD value = 0;
        DWORD type = 0;
        DWORD size = sizeof(value);

        status = RegQueryValueExW(
            hKey,
            wEntry.c_str(),
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(&value),
            &size
        );

        RegCloseKey(hKey);

        if (status == ERROR_SUCCESS && type == REG_DWORD && size == sizeof(DWORD))
        {
            return static_cast<int32_t>(value);
        }
    }

    return default_;
}

void config_impl_reg::set_int(std::string_view section, std::string_view entry, int32_t value)
{
    // base_application_key\\section
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY hKey = nullptr;
    DWORD disposition = 0;

    // Аналог CRegKey::Create(..., REG_OPTION_NON_VOLATILE, KEY_WRITE)
    LSTATUS status = RegCreateKeyExW(
        root,
        wKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        DWORD dwValue = static_cast<DWORD>(value);

        RegSetValueExW(
            hKey,
            wEntry.c_str(),
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue),
            sizeof(dwValue)
        );

        RegCloseKey(hKey);
    }
}

int64_t config_impl_reg::get_int64(std::string_view section, std::string_view entry, int64_t default_)
{
    // base_application_key\\section
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        root,
        wKey.c_str(),
        0,
        KEY_READ,
        &hKey
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        DWORD value = 0;
        DWORD type = 0;
        DWORD size = sizeof(value);

        status = RegQueryValueExW(
            hKey,
            wEntry.c_str(),
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(&value),
            &size
        );

        RegCloseKey(hKey);

        if (status == ERROR_SUCCESS && type == REG_QWORD && size == sizeof(int64_t))
        {
            return static_cast<int64_t>(value);
        }
    }

    return default_;
}

void config_impl_reg::set_int64(std::string_view section, std::string_view entry, int64_t value)
{
    // base_application_key\\section
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY hKey = nullptr;
    DWORD disposition = 0;

    LSTATUS status = RegCreateKeyExW(
        root,
        wKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        RegSetValueExW(
            hKey,
            wEntry.c_str(),
            0,
            REG_QWORD,
            reinterpret_cast<const BYTE*>(&value),
            sizeof(value)
        );

        RegCloseKey(hKey);
    }
}

std::string config_impl_reg::get_string(std::string_view section,
    std::string_view entry,
    std::string_view default_)
{
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::string out(default_.begin(), default_.end());

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY  hKey = nullptr;
    DWORD disposition = 0;

    LSTATUS status = RegCreateKeyExW(
        root,
        wKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_READ,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        DWORD type = 0;
        DWORD cbData = 0;

        // Первый вызов — узнаём размер
        status = RegQueryValueExW(
            hKey,
            wEntry.c_str(),
            nullptr,
            &type,
            nullptr,
            &cbData
        );

        if (status == ERROR_SUCCESS &&
            (type == REG_SZ || type == REG_EXPAND_SZ) &&
            cbData > sizeof(wchar_t))
        {
            // Ограничение в 2048 символов
            DWORD maxChars = 2048;
            DWORD charsCount = cbData / sizeof(wchar_t);

            if (charsCount > maxChars)
            {
                RegCloseKey(hKey);
                return out;
            }

            std::vector<wchar_t> buf(charsCount);

            status = RegQueryValueExW(
                hKey,
                wEntry.c_str(),
                nullptr,
                &type,
                reinterpret_cast<LPBYTE>(buf.data()),
                &cbData
            );

            if (status == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ))
            {
                charsCount = cbData / sizeof(wchar_t);

                if (charsCount > 0 && buf[charsCount - 1] == L'\0')
                    --charsCount;

                out.clear();
                out.append(
                    boost::nowide::narrow(buf.data(),
                        static_cast<int>(charsCount))
                );
            }
        }

        RegCloseKey(hKey);
    }

    return out;
}

void config_impl_reg::set_string(std::string_view section,
    std::string_view entry,
    std::string_view value)
{
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));
    std::wstring wValue = boost::nowide::widen(std::string(value));

    HKEY  hKey = nullptr;
    DWORD disposition = 0;

    LSTATUS status = RegCreateKeyExW(
        root,
        wKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        const BYTE* data =
            reinterpret_cast<const BYTE*>(wValue.c_str());
        DWORD cbData = static_cast<DWORD>(
            (wValue.size() + 1) * sizeof(wchar_t)
            );

        RegSetValueExW(
            hKey,
            wEntry.c_str(),
            0,
            REG_SZ,
            data,
            cbData
        );

        RegCloseKey(hKey);
    }
}

void config_impl_reg::delete_value(std::string_view section,
    std::string_view entry)
{
    std::string strKey = base_application_key;
    strKey.push_back('\\');
    strKey.append(section);

    std::wstring wKey = boost::nowide::widen(strKey);
    std::wstring wEntry = boost::nowide::widen(std::string(entry));

    HKEY  hKey = nullptr;
    DWORD disposition = 0;

    LSTATUS status = RegCreateKeyExW(
        root,
        wKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        RegDeleteValueW(hKey, wEntry.c_str());
        RegCloseKey(hKey);
    }
}

void config_impl_reg::delete_key(std::string_view section)
{
    std::wstring wBaseKey = boost::nowide::widen(base_application_key);
    std::wstring wSection = boost::nowide::widen(std::string(section));

    HKEY  hKey = nullptr;
    DWORD disposition = 0;

    LSTATUS status = RegCreateKeyExW(
        root,
        wBaseKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    if (status == ERROR_SUCCESS && hKey)
    {
        RegDeleteKeyW(hKey, wSection.c_str());
        RegCloseKey(hKey);
    }
}

error config_impl_reg::get_error() const
{
    return {};
}

}

}

#endif
