#pragma once

#include "TimeDiffPage.g.h"
#include "TimeZoneService.h"
#include "SettingsService.h"

namespace winrt::Clocks::implementation
{
    struct TimeDiffPage : TimeDiffPageT<TimeDiffPage>
    {
        TimeDiffPage();
        ~TimeDiffPage();

        void FromCity_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender,
                                       Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void ToCity_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender,
                                     Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    private:
        void PopulateCityBoxes();
        void UpdateTimesAndDiff();

        struct ZoneOption { winrt::hstring tzId; winrt::hstring displayName; };
        std::vector<ZoneOption> m_options;

        Services::TimeZoneService m_tzService;
        Services::SettingsService m_settingsService;
        winrt::hstring m_fromTzId;
        winrt::hstring m_fromCityName;
        winrt::hstring m_toTzId;
        winrt::hstring m_toCityName;
        Microsoft::UI::Xaml::DispatcherTimer m_timer{ nullptr };
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct TimeDiffPage : TimeDiffPageT<TimeDiffPage, implementation::TimeDiffPage>
    {
    };
}
