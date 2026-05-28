#include "pch.h"
#include "TimeZoneService.h"
#include <chrono>
#include <algorithm>
#include <cctype>

namespace winrt::Clocks::Services
{
    TimeZoneService::TimeZoneService()
    {
        InitializeCityData();
    }

    void TimeZoneService::InitializeCityData()
    {
        // Use std::chrono timezone database (requires Windows 10 1903+ for ICU)
        try
        {
            const auto& tzdb = std::chrono::get_tzdb();
            for (const auto& zone : tzdb.zones)
            {
                std::string name{zone.name()};
                // Filter to only named city zones (skip generic like "Etc/GMT+5")
                if (name.find('/') == std::string::npos) continue;
                if (name.starts_with("Etc/")) continue;
                if (name.starts_with("SystemV/")) continue;
                if (name.starts_with("US/")) continue;
                if (name.starts_with("posix/")) continue;
                if (name.starts_with("right/")) continue;

                CityInfo info;
                std::wstring wname(name.begin(), name.end());
                info.TimezoneId = winrt::hstring(wname);
                info.CityName = ExtractCityName(wname);
                info.Region = ExtractRegion(wname);
                // Country left empty for now -- could be extended with a lookup table
                info.Country = L"";

                m_cities.push_back(std::move(info));
            }

            // Sort by city name
            std::sort(m_cities.begin(), m_cities.end(),
                [](const CityInfo& a, const CityInfo& b) {
                    return a.CityName < b.CityName;
                });
        }
        catch (...)
        {
            // Fallback: add some well-known cities if chrono tz fails
            m_cities.push_back({ L"America/New_York", L"New York", L"United States", L"Americas" });
            m_cities.push_back({ L"America/Los_Angeles", L"Los Angeles", L"United States", L"Americas" });
            m_cities.push_back({ L"America/Chicago", L"Chicago", L"United States", L"Americas" });
            m_cities.push_back({ L"Europe/London", L"London", L"United Kingdom", L"Europe" });
            m_cities.push_back({ L"Europe/Paris", L"Paris", L"France", L"Europe" });
            m_cities.push_back({ L"Europe/Berlin", L"Berlin", L"Germany", L"Europe" });
            m_cities.push_back({ L"Asia/Tokyo", L"Tokyo", L"Japan", L"Asia" });
            m_cities.push_back({ L"Asia/Shanghai", L"Shanghai", L"China", L"Asia" });
            m_cities.push_back({ L"Asia/Kolkata", L"Kolkata", L"India", L"Asia" });
            m_cities.push_back({ L"Australia/Sydney", L"Sydney", L"Australia", L"Pacific" });
            m_cities.push_back({ L"Pacific/Auckland", L"Auckland", L"New Zealand", L"Pacific" });
        }
    }

    std::vector<CityInfo> const& TimeZoneService::GetAllCities() const
    {
        return m_cities;
    }

    std::vector<CityInfo> TimeZoneService::SearchCities(std::wstring_view query) const
    {
        std::vector<CityInfo> results;
        if (query.empty()) return results;

        std::wstring lowerQuery(query);
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);

        for (const auto& city : m_cities)
        {
            std::wstring lowerCity(city.CityName);
            std::transform(lowerCity.begin(), lowerCity.end(), lowerCity.begin(), ::towlower);
            std::wstring lowerTzId(city.TimezoneId);
            std::transform(lowerTzId.begin(), lowerTzId.end(), lowerTzId.begin(), ::towlower);
            std::wstring lowerCountry(city.Country);
            std::transform(lowerCountry.begin(), lowerCountry.end(), lowerCountry.begin(), ::towlower);

            if (lowerCity.find(lowerQuery) != std::wstring::npos ||
                lowerTzId.find(lowerQuery) != std::wstring::npos ||
                lowerCountry.find(lowerQuery) != std::wstring::npos)
            {
                results.push_back(city);
            }
        }
        return results;
    }

    TimeInfo TimeZoneService::GetCurrentTime(std::wstring_view timezoneId) const
    {
        TimeInfo info{};
        try
        {
            std::string tzId(timezoneId.begin(), timezoneId.end());
            const auto* tz = std::chrono::locate_zone(tzId);
            auto now = std::chrono::system_clock::now();
            auto zoned = std::chrono::zoned_time{ tz, now };
            auto local = zoned.get_local_time();

            // Extract date/time components
            auto dp = std::chrono::floor<std::chrono::days>(local);
            std::chrono::year_month_day ymd{ dp };
            std::chrono::hh_mm_ss hms{ local - dp };
            std::chrono::weekday wd{ dp };

            info.hours = static_cast<int>(hms.hours().count());
            info.minutes = static_cast<int>(hms.minutes().count());
            info.seconds = static_cast<int>(hms.seconds().count());
            info.year = static_cast<int>(ymd.year());
            info.month = static_cast<unsigned>(ymd.month());
            info.day = static_cast<unsigned>(ymd.day());
            info.weekday = static_cast<int>(wd.c_encoding());

            // Format time string (12-hour)
            int h12 = info.hours % 12;
            if (h12 == 0) h12 = 12;
            std::wstring ampm = info.hours >= 12 ? L"PM" : L"AM";
            info.formattedTime = winrt::hstring(std::format(L"{:d}:{:02d}:{:02d} {}", h12, info.minutes, info.seconds, ampm));

            // Format date string
            static const wchar_t* dayNames[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
            static const wchar_t* monthNames[] = { L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
            info.formattedDate = winrt::hstring(std::format(L"{}, {} {:d}, {:d}",
                dayNames[info.weekday], monthNames[info.month], info.day, info.year));

            // Calculate UTC offset
            auto offsetInfo = tz->get_info(now);
            auto offsetSec = std::chrono::duration_cast<std::chrono::seconds>(offsetInfo.offset);
            info.totalOffsetMinutes = static_cast<int>(offsetSec.count() / 60);
            int absHours = std::abs(info.totalOffsetMinutes) / 60;
            int absMins = std::abs(info.totalOffsetMinutes) % 60;
            wchar_t sign = info.totalOffsetMinutes >= 0 ? L'+' : L'-';
            if (absMins > 0)
                info.utcOffset = winrt::hstring(std::format(L"UTC{}{:d}:{:02d}", sign, absHours, absMins));
            else
                info.utcOffset = winrt::hstring(std::format(L"UTC{}{:d}", sign, absHours));
        }
        catch (...)
        {
            info.formattedTime = L"--:--:--";
            info.formattedDate = L"---";
            info.utcOffset = L"UTC?";
        }
        return info;
    }

    winrt::hstring TimeZoneService::GetLocalTimezoneId() const
    {
        try
        {
            const auto* local = std::chrono::current_zone();
            std::string name{local->name()};
            return winrt::hstring(std::wstring(name.begin(), name.end()));
        }
        catch (...)
        {
            return L"UTC";
        }
    }

    int TimeZoneService::GetTimeDifferenceMinutes(std::wstring_view tz1, std::wstring_view tz2) const
    {
        try
        {
            auto now = std::chrono::system_clock::now();
            std::string tzId1(tz1.begin(), tz1.end());
            std::string tzId2(tz2.begin(), tz2.end());
            const auto* zone1 = std::chrono::locate_zone(tzId1);
            const auto* zone2 = std::chrono::locate_zone(tzId2);
            auto offset1 = zone1->get_info(now).offset;
            auto offset2 = zone2->get_info(now).offset;
            auto diff = std::chrono::duration_cast<std::chrono::minutes>(offset2 - offset1);
            return static_cast<int>(diff.count());
        }
        catch (...)
        {
            return 0;
        }
    }

    winrt::hstring TimeZoneService::ExtractCityName(std::wstring_view tzId)
    {
        auto pos = tzId.rfind(L'/');
        if (pos == std::wstring_view::npos) return winrt::hstring(tzId);
        std::wstring city(tzId.substr(pos + 1));
        // Replace underscores with spaces
        std::replace(city.begin(), city.end(), L'_', L' ');
        return winrt::hstring(city);
    }

    winrt::hstring TimeZoneService::ExtractRegion(std::wstring_view tzId)
    {
        auto pos = tzId.find(L'/');
        if (pos == std::wstring_view::npos) return L"Other";
        std::wstring region(tzId.substr(0, pos));
        if (region == L"America") return L"Americas";
        if (region == L"US") return L"Americas";
        if (region == L"Canada") return L"Americas";
        return winrt::hstring(region);
    }
}
