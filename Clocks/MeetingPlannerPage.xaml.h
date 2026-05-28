#pragma once

#include "MeetingPlannerPage.g.h"
#include "TimeZoneService.h"
#include "SettingsService.h"

namespace winrt::Clocks::implementation
{
    struct MeetingPlannerPage : MeetingPlannerPageT<MeetingPlannerPage>
    {
        MeetingPlannerPage();
        ~MeetingPlannerPage();

        void BusinessHours_Changed(Microsoft::UI::Xaml::Controls::NumberBox const& sender,
                                    Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);

    private:
        struct ZoneEntry
        {
            winrt::hstring timezoneId;
            winrt::hstring cityName;
        };

        void AddTimezone(winrt::hstring const& tzId, winrt::hstring const& cityName);
        void RebuildTimeline();
        Microsoft::UI::Xaml::UIElement CreateTimelineRow(
            ZoneEntry const& entry,
            std::vector<uint8_t> const& bizMinByUtc);

        Services::TimeZoneService m_tzService;
        Services::SettingsService m_settingsService;
        std::vector<ZoneEntry> m_zones;
        Microsoft::UI::Xaml::DispatcherTimer m_timer{ nullptr };
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct MeetingPlannerPage : MeetingPlannerPageT<MeetingPlannerPage, implementation::MeetingPlannerPage>
    {
    };
}
