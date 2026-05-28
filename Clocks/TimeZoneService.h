#pragma once
#include "pch.h"

namespace winrt::Clocks::Services
{
    struct CityInfo
    {
        winrt::hstring TimezoneId;
        winrt::hstring CityName;
        winrt::hstring Country;
        winrt::hstring Region;  // e.g., "Americas", "Europe", "Asia"
    };

    struct TimeInfo
    {
        int hours;
        int minutes;
        int seconds;
        int year;
        int month;
        int day;
        int weekday;  // 0=Sunday
        winrt::hstring formattedTime;
        winrt::hstring formattedDate;
        winrt::hstring utcOffset;
        int totalOffsetMinutes;
    };

    class TimeZoneService
    {
    public:
        TimeZoneService();

        std::vector<CityInfo> const& GetAllCities() const;
        std::vector<CityInfo> SearchCities(std::wstring_view query) const;
        TimeInfo GetCurrentTime(std::wstring_view timezoneId) const;
        // Returns the UTC offset (in minutes) that `timezoneId` has at the given
        // UTC instant. Use this instead of `TimeInfo::totalOffsetMinutes` when
        // mapping a 24-hour window: a single day can straddle a DST transition,
        // and using "now's" offset for every hour produces a one-hour error
        // across the transition.
        int GetOffsetMinutesAt(std::wstring_view timezoneId,
                               std::chrono::system_clock::time_point instant) const;
        winrt::hstring GetLocalTimezoneId() const;
        int GetTimeDifferenceMinutes(std::wstring_view tz1, std::wstring_view tz2) const;

    private:
        void InitializeCityData();
        static winrt::hstring ExtractCityName(std::wstring_view tzId);
        static winrt::hstring ExtractRegion(std::wstring_view tzId);

        std::vector<CityInfo> m_cities;
    };
}
