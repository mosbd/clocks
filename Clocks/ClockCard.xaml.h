#pragma once

#include "ClockCard.g.h"
#include "TimeZoneService.h"
#include "ClockModel.h"

namespace winrt::Clocks::implementation
{
    struct ClockCard : ClockCardT<ClockCard>
    {
        ClockCard();

        void Initialize(winrt::hstring const& timezoneId, winrt::hstring const& cityName);
        void UpdateTime();
        void SetAnalogStyle(int32_t style);
        void SetDigitalStyle(int32_t style);
        void ToggleDisplayMode();
        winrt::hstring TimezoneId();
        winrt::hstring CityName();
        int32_t DisplayMode();
        int32_t AnalogStyle();
        int32_t DigitalStyle();

        winrt::event_token RemoveRequested(winrt::Windows::Foundation::EventHandler<winrt::hstring> const& handler);
        void RemoveRequested(winrt::event_token const& token) noexcept;
        winrt::event_token StyleChanged(winrt::Windows::Foundation::EventHandler<winrt::hstring> const& handler);
        void StyleChanged(winrt::event_token const& token) noexcept;

        // XAML event handlers
        void ToggleMode_Click(winrt::Windows::Foundation::IInspectable const& sender,
                              Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void StyleChange_Click(winrt::Windows::Foundation::IInspectable const& sender,
                               Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Remove_Click(winrt::Windows::Foundation::IInspectable const& sender,
                          Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void BuildAnalogClock();
        void UpdateAnalogHands();
        void ApplyAnalogStyle();
        void UpdateDigitalDisplay();

        winrt::hstring m_timezoneId;
        winrt::hstring m_cityName;
        Models::ClockDisplayMode m_displayMode{ Models::ClockDisplayMode::Analog };
        Models::AnalogStyle m_analogStyle{ Models::AnalogStyle::Modern };
        Models::DigitalStyle m_digitalStyle{ Models::DigitalStyle::Clean };

        static Services::TimeZoneService& TzService();

        // Analog clock shape references
        Microsoft::UI::Xaml::Shapes::Ellipse m_face{ nullptr };
        Microsoft::UI::Xaml::Shapes::Line m_hourHand{ nullptr };
        Microsoft::UI::Xaml::Shapes::Line m_minuteHand{ nullptr };
        Microsoft::UI::Xaml::Shapes::Line m_secondHand{ nullptr };
        Microsoft::UI::Xaml::Shapes::Ellipse m_centerDot{ nullptr };
        std::vector<Microsoft::UI::Xaml::UIElement> m_tickMarks;
        std::vector<Microsoft::UI::Xaml::Controls::TextBlock> m_numerals;

        Services::TimeInfo m_lastTimeInfo{};

        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::hstring>> m_removeRequested;
        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::hstring>> m_styleChanged;
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct ClockCard : ClockCardT<ClockCard, implementation::ClockCard>
    {
    };
}
