#pragma once

#include "ClocksPage.g.h"
#include "TimeZoneService.h"
#include "SettingsService.h"
#include "ClockModel.h"

namespace winrt::Clocks::implementation
{
    struct ClocksPage : ClocksPageT<ClocksPage>
    {
        ClocksPage();
        ~ClocksPage();

        void ReloadClocks();

    private:
        winrt::fire_and_forget AddClock_Click(winrt::Windows::Foundation::IInspectable const& sender,
                            Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void LoadClocks();
        void SaveClocks();
        void AddClockCard(winrt::hstring const& timezoneId, winrt::hstring const& cityName,
                          Models::ClockDisplayMode displayMode = Models::ClockDisplayMode::Analog,
                          Models::AnalogStyle analogStyle = Models::AnalogStyle::Modern,
                          Models::DigitalStyle digitalStyle = Models::DigitalStyle::Clean);
        void OnClockRemoveRequested(winrt::Windows::Foundation::IInspectable const& sender,
                                    winrt::hstring const& timezoneId);
        void OnClockStyleChanged(winrt::Windows::Foundation::IInspectable const& sender,
                                 winrt::hstring const& timezoneId);
        void OnTimerTick(winrt::Windows::Foundation::IInspectable const& sender,
                         winrt::Windows::Foundation::IInspectable const& e);
        void UpdateEmptyState();

        Services::TimeZoneService m_tzService;
        Services::SettingsService m_settingsService;
        Microsoft::UI::Xaml::DispatcherTimer m_timer{ nullptr };
        winrt::Windows::Foundation::Collections::IObservableVector<Microsoft::UI::Xaml::UIElement> m_clockCards{ nullptr };
        winrt::event_token m_addClickToken{};
        bool m_wired{ false };
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct ClocksPage : ClocksPageT<ClocksPage, implementation::ClocksPage>
    {
    };
}
