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
        winrt::hstring GetLocalTimezoneId() const;
        int GetTimeDifferenceMinutes(std::wstring_view tz1, std::wstring_view tz2) const;

    private:
        void InitializeCityData();
        static winrt::hstring ExtractCityName(std::wstring_view tzId);
        static winrt::hstring ExtractRegion(std::wstring_view tzId);

        std::vector<CityInfo> m_cities;
    };
}
