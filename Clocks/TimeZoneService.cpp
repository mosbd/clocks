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

            // Add common-name aliases for major cities that share an IANA zone.
            struct Alias { const wchar_t* tz; const wchar_t* city; const wchar_t* country; const wchar_t* region; };
            static const Alias aliases[] = {
                { L"America/Los_Angeles", L"Seattle",       L"United States", L"America" },
                { L"America/Los_Angeles", L"San Francisco", L"United States", L"America" },
                { L"America/Los_Angeles", L"San Diego",     L"United States", L"America" },
                { L"America/Los_Angeles", L"Portland",      L"United States", L"America" },
                { L"America/Denver",      L"Salt Lake City",L"United States", L"America" },
                { L"America/Phoenix",     L"Phoenix",       L"United States", L"America" },
                { L"America/Chicago",     L"Houston",       L"United States", L"America" },
                { L"America/Chicago",     L"Dallas",        L"United States", L"America" },
                { L"America/Chicago",     L"Austin",        L"United States", L"America" },
                { L"America/Chicago",     L"Minneapolis",   L"United States", L"America" },
                { L"America/New_York",    L"Boston",        L"United States", L"America" },
                { L"America/New_York",    L"Washington DC", L"United States", L"America" },
                { L"America/New_York",    L"Miami",         L"United States", L"America" },
                { L"America/New_York",    L"Atlanta",       L"United States", L"America" },
                { L"America/New_York",    L"Philadelphia",  L"United States", L"America" },
                { L"America/Toronto",     L"Ottawa",        L"Canada",        L"America" },
                { L"America/Vancouver",   L"Victoria",      L"Canada",        L"America" },
                // China (all use Asia/Shanghai / CST UTC+8)
                { L"Asia/Shanghai",       L"Beijing",       L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Guangzhou",     L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Shenzhen",      L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Chengdu",       L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Chongqing",     L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Hangzhou",      L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Nanjing",       L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Tianjin",       L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Wuhan",         L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Xi'an",         L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Suzhou",        L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Qingdao",       L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Dalian",        L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Shenyang",      L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Harbin",        L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Changsha",      L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Zhengzhou",     L"China",         L"Asia" },
                { L"Asia/Shanghai",       L"Kunming",       L"China",         L"Asia" },
                { L"Asia/Urumqi",         L"Urumqi",        L"China",         L"Asia" },
                { L"Asia/Hong_Kong",      L"Hong Kong",     L"China",         L"Asia" },
                { L"Asia/Macau",          L"Macau",         L"China",         L"Asia" },
                { L"Asia/Taipei",         L"Taipei",        L"Taiwan",        L"Asia" },
            };
            for (const auto& a : aliases)
            {
                m_cities.push_back({ winrt::hstring(a.tz), winrt::hstring(a.city),
                                     winrt::hstring(a.country), winrt::hstring(a.region) });
            }

            // Single-timezone countries: allow searching/adding by country name.
            // Each entry adds a record whose "city" is the country, so the user
            // can pick the country itself when it has only one IANA zone.
            struct CountryTz { const wchar_t* tz; const wchar_t* country; const wchar_t* region; };
            static const CountryTz countryZones[] = {
                { L"Europe/London",       L"United Kingdom", L"Europe" },
                { L"Europe/Dublin",       L"Ireland",        L"Europe" },
                { L"Europe/Paris",        L"France",         L"Europe" },
                { L"Europe/Berlin",       L"Germany",        L"Europe" },
                { L"Europe/Rome",         L"Italy",          L"Europe" },
                { L"Europe/Madrid",       L"Spain",          L"Europe" },
                { L"Europe/Lisbon",       L"Portugal",       L"Europe" },
                { L"Europe/Amsterdam",    L"Netherlands",    L"Europe" },
                { L"Europe/Brussels",     L"Belgium",        L"Europe" },
                { L"Europe/Vienna",       L"Austria",        L"Europe" },
                { L"Europe/Zurich",       L"Switzerland",    L"Europe" },
                { L"Europe/Stockholm",    L"Sweden",         L"Europe" },
                { L"Europe/Oslo",         L"Norway",         L"Europe" },
                { L"Europe/Copenhagen",   L"Denmark",        L"Europe" },
                { L"Europe/Helsinki",     L"Finland",        L"Europe" },
                { L"Europe/Warsaw",       L"Poland",         L"Europe" },
                { L"Europe/Prague",       L"Czech Republic", L"Europe" },
                { L"Europe/Budapest",     L"Hungary",        L"Europe" },
                { L"Europe/Bucharest",    L"Romania",        L"Europe" },
                { L"Europe/Athens",       L"Greece",         L"Europe" },
                { L"Europe/Sofia",        L"Bulgaria",       L"Europe" },
                { L"Asia/Tokyo",          L"Japan",          L"Asia" },
                { L"Asia/Seoul",          L"South Korea",    L"Asia" },
                { L"Asia/Shanghai",       L"China",          L"Asia" },
                { L"Asia/Hong_Kong",      L"Hong Kong",      L"Asia" },
                { L"Asia/Singapore",      L"Singapore",      L"Asia" },
                { L"Asia/Bangkok",        L"Thailand",       L"Asia" },
                { L"Asia/Ho_Chi_Minh",    L"Vietnam",        L"Asia" },
                { L"Asia/Manila",         L"Philippines",    L"Asia" },
                { L"Asia/Kolkata",        L"India",          L"Asia" },
                { L"Asia/Karachi",        L"Pakistan",       L"Asia" },
                { L"Asia/Dhaka",          L"Bangladesh",     L"Asia" },
                { L"Asia/Colombo",        L"Sri Lanka",      L"Asia" },
                { L"Asia/Kathmandu",      L"Nepal",          L"Asia" },
                { L"Asia/Dubai",          L"United Arab Emirates", L"Asia" },
                { L"Asia/Riyadh",         L"Saudi Arabia",   L"Asia" },
                { L"Asia/Qatar",          L"Qatar",          L"Asia" },
                { L"Asia/Kuwait",         L"Kuwait",         L"Asia" },
                { L"Asia/Bahrain",        L"Bahrain",        L"Asia" },
                { L"Asia/Tehran",         L"Iran",           L"Asia" },
                { L"Asia/Baghdad",        L"Iraq",           L"Asia" },
                { L"Asia/Jerusalem",      L"Israel",         L"Asia" },
                { L"Africa/Cairo",        L"Egypt",          L"Africa" },
                { L"Africa/Lagos",        L"Nigeria",        L"Africa" },
                { L"Africa/Nairobi",      L"Kenya",          L"Africa" },
                { L"Africa/Johannesburg", L"South Africa",   L"Africa" },
                { L"Africa/Casablanca",   L"Morocco",        L"Africa" },
                { L"Africa/Algiers",      L"Algeria",        L"Africa" },
                { L"Africa/Tunis",        L"Tunisia",        L"Africa" },
                { L"Africa/Accra",        L"Ghana",          L"Africa" },
                { L"Africa/Addis_Ababa",  L"Ethiopia",       L"Africa" },
                { L"Pacific/Auckland",    L"New Zealand",    L"Pacific" },
                { L"Pacific/Fiji",        L"Fiji",           L"Pacific" },
                { L"America/Mexico_City", L"Mexico",         L"Americas" },
                { L"America/Argentina/Buenos_Aires", L"Argentina", L"Americas" },
                { L"America/Bogota",      L"Colombia",       L"Americas" },
                { L"America/Lima",        L"Peru",           L"Americas" },
                { L"America/Caracas",     L"Venezuela",      L"Americas" },
                { L"America/Santiago",    L"Chile",          L"Americas" },
                { L"America/Havana",      L"Cuba",           L"Americas" },
                { L"America/Jamaica",     L"Jamaica",        L"Americas" },
            };
            for (const auto& c : countryZones)
            {
                m_cities.push_back({ winrt::hstring(c.tz), winrt::hstring(c.country),
                                     winrt::hstring(c.country), winrt::hstring(c.region) });
            }

            // US/Canada time-zone friendly names — allow searching by zone name
            // (e.g. "Pacific Time", "PT", "PST") instead of a city.
            // Multiple entries per zone so abbreviations (PST/PDT/EST/EDT/...) all match search.
            struct ZoneAlias { const wchar_t* tz; const wchar_t* name; const wchar_t* country; const wchar_t* region; };
            static const ZoneAlias zoneAliases[] = {
                { L"America/Los_Angeles", L"Pacific Time (PT / PST / PDT)",     L"United States", L"Americas" },
                { L"America/Denver",      L"Mountain Time (MT / MST / MDT)",    L"United States", L"Americas" },
                { L"America/Phoenix",     L"Arizona Time (MST, no DST)",        L"United States", L"Americas" },
                { L"America/Chicago",     L"Central Time (CT / CST / CDT)",     L"United States", L"Americas" },
                { L"America/New_York",    L"Eastern Time (ET / EST / EDT)",     L"United States", L"Americas" },
                { L"America/Anchorage",   L"Alaska Time (AKT / AKST / AKDT)",   L"United States", L"Americas" },
                { L"Pacific/Honolulu",    L"Hawaii Time (HST)",                  L"United States", L"Pacific" },
                { L"America/Vancouver",   L"Pacific Time - Canada (PT / PST / PDT)",  L"Canada", L"Americas" },
                { L"America/Edmonton",    L"Mountain Time - Canada (MT / MST / MDT)", L"Canada", L"Americas" },
                { L"America/Winnipeg",    L"Central Time - Canada (CT / CST / CDT)",  L"Canada", L"Americas" },
                { L"America/Toronto",     L"Eastern Time - Canada (ET / EST / EDT)",  L"Canada", L"Americas" },
                { L"America/Halifax",     L"Atlantic Time (AT / AST / ADT)",     L"Canada",        L"Americas" },
                { L"America/St_Johns",    L"Newfoundland Time (NT / NST / NDT)", L"Canada",        L"Americas" },
            };
            for (const auto& z : zoneAliases)
            {
                m_cities.push_back({ winrt::hstring(z.tz), winrt::hstring(z.name),
                                     winrt::hstring(z.country), winrt::hstring(z.region) });
            }

            std::sort(m_cities.begin(), m_cities.end(),
                [](const CityInfo& a, const CityInfo& b) { return a.CityName < b.CityName; });
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
            std::string tzId;
            tzId.reserve(timezoneId.size());
            for (wchar_t wc : timezoneId)
                tzId.push_back(static_cast<char>(wc));
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

    int TimeZoneService::GetOffsetMinutesAt(std::wstring_view timezoneId,
                                            std::chrono::system_clock::time_point instant) const
    {
        try
        {
            std::string tzId;
            tzId.reserve(timezoneId.size());
            for (wchar_t wc : timezoneId)
                tzId.push_back(static_cast<char>(wc));
            const auto* tz = std::chrono::locate_zone(tzId);
            auto info = tz->get_info(instant);
            auto offsetSec = std::chrono::duration_cast<std::chrono::seconds>(info.offset);
            return static_cast<int>(offsetSec.count() / 60);
        }
        catch (...)
        {
            return 0;
        }
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
            auto toNarrow = [](std::wstring_view w) {
                std::string s;
                s.reserve(w.size());
                for (wchar_t wc : w) s.push_back(static_cast<char>(wc));
                return s;
            };
            std::string tzId1 = toNarrow(tz1);
            std::string tzId2 = toNarrow(tz2);
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
