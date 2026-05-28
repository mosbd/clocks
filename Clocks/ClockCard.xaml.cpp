#include "pch.h"
#include "ClockCard.xaml.h"
#if __has_include("ClockCard.g.cpp")
#include "ClockCard.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::UI;

namespace winrt::Clocks::implementation
{
    // Construct lazily on first call so we don't depend on undefined static
    // initialization order across TUs. `std::chrono::get_tzdb()` (called
    // transitively from TimeZoneService) is not safe to invoke at SIOF time.
    Services::TimeZoneService& ClockCard::TzService()
    {
        static Services::TimeZoneService instance;
        return instance;
    }

    static constexpr double kCanvasSize = 170.0;
    static constexpr double kCenter = 85.0;
    static constexpr double kFaceRadius = 78.0;
    static constexpr double kPi = 3.14159265358979323846;

    ClockCard::ClockCard()
    {
    }

    void ClockCard::Initialize(hstring const& timezoneId, hstring const& cityName)
    {
        m_timezoneId = timezoneId;
        m_cityName = cityName;
        CityNameText().Text(cityName);
        BuildAnalogClock();
        UpdateTime();
    }

    hstring ClockCard::TimezoneId() { return m_timezoneId; }
    hstring ClockCard::CityName() { return m_cityName; }
    int32_t ClockCard::DisplayMode() { return static_cast<int32_t>(m_displayMode); }
    int32_t ClockCard::AnalogStyle() { return static_cast<int32_t>(m_analogStyle); }
    int32_t ClockCard::DigitalStyle() { return static_cast<int32_t>(m_digitalStyle); }

    event_token ClockCard::RemoveRequested(Windows::Foundation::EventHandler<hstring> const& handler)
    {
        return m_removeRequested.add(handler);
    }
    void ClockCard::RemoveRequested(event_token const& token) noexcept
    {
        m_removeRequested.remove(token);
    }
    event_token ClockCard::StyleChanged(Windows::Foundation::EventHandler<hstring> const& handler)
    {
        return m_styleChanged.add(handler);
    }
    void ClockCard::StyleChanged(event_token const& token) noexcept
    {
        m_styleChanged.remove(token);
    }

    void ClockCard::ToggleDisplayMode()
    {
        if (m_displayMode == Models::ClockDisplayMode::Analog)
        {
            m_displayMode = Models::ClockDisplayMode::Digital;
            AnalogContainer().Visibility(Visibility::Collapsed);
            DigitalContainer().Visibility(Visibility::Visible);
            ToggleModeItem().Text(L"Switch to Analog");
        }
        else
        {
            m_displayMode = Models::ClockDisplayMode::Analog;
            AnalogContainer().Visibility(Visibility::Visible);
            DigitalContainer().Visibility(Visibility::Collapsed);
            ToggleModeItem().Text(L"Switch to Digital");
        }
        UpdateTime();
        m_styleChanged(*this, m_timezoneId);
    }

    void ClockCard::SetAnalogStyle(int32_t style)
    {
        m_analogStyle = static_cast<Models::AnalogStyle>(style);
        ClockCanvas().Children().Clear();
        m_tickMarks.clear();
        m_numerals.clear();
        BuildAnalogClock();
        UpdateAnalogHands();
        m_styleChanged(*this, m_timezoneId);
    }

    void ClockCard::SetDigitalStyle(int32_t style)
    {
        m_digitalStyle = static_cast<Models::DigitalStyle>(style);
        UpdateDigitalDisplay();
        m_styleChanged(*this, m_timezoneId);
    }

    void ClockCard::ToggleMode_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ToggleDisplayMode();
    }

    void ClockCard::StyleChange_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto item = sender.as<MenuFlyoutItem>();
        auto tag = unbox_value<hstring>(item.Tag());
        std::wstring tagStr(tag);

        if (tagStr.starts_with(L"analog"))
        {
            int style = tagStr.back() - L'0';
            SetAnalogStyle(style);
            if (m_displayMode != Models::ClockDisplayMode::Analog)
                ToggleDisplayMode();
        }
        else if (tagStr.starts_with(L"digital"))
        {
            int style = tagStr.back() - L'0';
            SetDigitalStyle(style);
            if (m_displayMode != Models::ClockDisplayMode::Digital)
                ToggleDisplayMode();
        }
    }

    void ClockCard::Remove_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_removeRequested(*this, m_timezoneId);
    }

    void ClockCard::BuildAnalogClock()
    {
        auto canvas = ClockCanvas();
        bool isDark = (ActualTheme() == ElementTheme::Dark);

        auto faceColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30));
        auto bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 100, 100, 100));
        auto handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 220, 220, 220));
        auto secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 220, 50, 50));
        auto tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 180, 180, 180));
        double bezelThickness = 2.0;
        bool showNumerals = false;
        bool romanNumerals = false;
        int tickCount = 12;
        double tickLength = 8.0;
        double tickWidth = 2.0;
        double hourHandWidth = 4.0;
        double minuteHandWidth = 3.0;
        double secondHandWidth = 1.0;
        double hourHandLength = 35.0;
        double minuteHandLength = 55.0;
        double secondHandLength = 60.0;

        switch (m_analogStyle)
        {
        case Models::AnalogStyle::Classic:
            faceColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 25, 25, 35));
            bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 180, 155, 80));
            handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 180, 120));
            secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 60, 60));
            tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 180, 155, 80));
            bezelThickness = 3.0;
            showNumerals = true;
            romanNumerals = true;
            tickCount = 60;
            tickLength = 6.0;
            tickWidth = 1.0;
            hourHandWidth = 5.0;
            minuteHandWidth = 3.5;
            break;

        case Models::AnalogStyle::Modern:
            if (isDark)
            {
                faceColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 25, 25, 30));
                bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 100, 100, 100));
                handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 220, 220, 220));
                secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 120, 212));
                tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 180, 180, 180));
            }
            else
            {
                faceColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 240, 240, 245));
                bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 80, 80, 80));
                handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 40, 40, 40));
                secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 120, 212));
                tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 60, 60, 60));
            }
            bezelThickness = 2.0;
            showNumerals = true;
            romanNumerals = false;
            tickCount = 12;
            tickLength = 10.0;
            tickWidth = 2.0;
            hourHandWidth = 5.0;
            minuteHandWidth = 3.0;
            hourHandLength = 38.0;
            minuteHandLength = 55.0;
            break;

        case Models::AnalogStyle::Minimal:
            faceColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0));
            if (isDark)
            {
                bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 180, 180, 180));
                handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 200, 200));
                secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 200, 200));
                tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 170, 170, 170));
            }
            else
            {
                bezelColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 120, 120, 120));
                handColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 100, 100, 100));
                secondColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 100, 100, 100));
                tickColor = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 110, 110, 110));
            }
            bezelThickness = 1.0;
            showNumerals = false;
            tickCount = 4;
            tickLength = 12.0;
            tickWidth = 1.0;
            hourHandWidth = 2.5;
            minuteHandWidth = 1.5;
            secondHandWidth = 0.5;
            hourHandLength = 30.0;
            minuteHandLength = 50.0;
            secondHandLength = 55.0;
            break;
        }

        // Clock face
        m_face = Shapes::Ellipse();
        m_face.Width(kFaceRadius * 2);
        m_face.Height(kFaceRadius * 2);
        m_face.Fill(faceColor);
        m_face.Stroke(bezelColor);
        m_face.StrokeThickness(bezelThickness);
        Canvas::SetLeft(m_face, kCenter - kFaceRadius);
        Canvas::SetTop(m_face, kCenter - kFaceRadius);
        canvas.Children().Append(m_face);

        // Tick marks
        for (int i = 0; i < (tickCount == 4 ? 4 : tickCount); i++)
        {
            double angle;
            double currentTickLength = tickLength;
            double currentTickWidth = tickWidth;

            if (tickCount == 4)
            {
                angle = i * 90.0;
            }
            else if (tickCount == 60)
            {
                angle = i * 6.0;
                if (i % 5 != 0)
                {
                    currentTickLength = tickLength * 0.5;
                    currentTickWidth = 0.5;
                }
            }
            else
            {
                angle = i * 30.0;
            }

            double radians = (angle - 90.0) * kPi / 180.0;
            double outerR = kFaceRadius - 4.0;
            double innerR = outerR - currentTickLength;

            auto tick = Line();
            tick.X1(kCenter + innerR * cos(radians));
            tick.Y1(kCenter + innerR * sin(radians));
            tick.X2(kCenter + outerR * cos(radians));
            tick.Y2(kCenter + outerR * sin(radians));
            tick.Stroke(tickColor);
            tick.StrokeThickness(currentTickWidth);
            canvas.Children().Append(tick);
            m_tickMarks.push_back(tick);
        }

        // Numerals
        if (showNumerals)
        {
            const wchar_t* romanNums[] = { L"XII", L"I", L"II", L"III", L"IV", L"V", L"VI", L"VII", L"VIII", L"IX", L"X", L"XI" };
            const wchar_t* arabicNums[] = { L"12", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10", L"11" };

            for (int i = 0; i < 12; i++)
            {
                if (!romanNumerals && (i % 3 != 0)) continue;

                double angle = i * 30.0;
                double radians = (angle - 90.0) * kPi / 180.0;
                double numR = kFaceRadius - 22.0;

                auto tb = TextBlock();
                tb.Text(romanNumerals ? romanNums[i] : arabicNums[i]);
                tb.FontSize(romanNumerals ? 10.0 : 13.0);
                tb.Foreground(tickColor);
                tb.HorizontalAlignment(HorizontalAlignment::Center);
                tb.TextAlignment(TextAlignment::Center);

                double estWidth = romanNumerals ? 22.0 : 16.0;
                double estHeight = romanNumerals ? 14.0 : 16.0;
                double cx = kCenter + numR * cos(radians) - estWidth / 2.0;
                double cy = kCenter + numR * sin(radians) - estHeight / 2.0;

                Canvas::SetLeft(tb, cx);
                Canvas::SetTop(tb, cy);
                canvas.Children().Append(tb);
                m_numerals.push_back(tb);
            }
        }

        // Hour hand
        m_hourHand = Line();
        m_hourHand.X1(kCenter);
        m_hourHand.Y1(kCenter);
        m_hourHand.X2(kCenter);
        m_hourHand.Y2(kCenter - hourHandLength);
        m_hourHand.Stroke(handColor);
        m_hourHand.StrokeThickness(hourHandWidth);
        m_hourHand.StrokeStartLineCap(PenLineCap::Round);
        m_hourHand.StrokeEndLineCap(PenLineCap::Round);
        auto hourTransform = RotateTransform();
        hourTransform.CenterX(kCenter);
        hourTransform.CenterY(kCenter);
        m_hourHand.RenderTransform(hourTransform);
        canvas.Children().Append(m_hourHand);

        // Minute hand
        m_minuteHand = Line();
        m_minuteHand.X1(kCenter);
        m_minuteHand.Y1(kCenter);
        m_minuteHand.X2(kCenter);
        m_minuteHand.Y2(kCenter - minuteHandLength);
        m_minuteHand.Stroke(handColor);
        m_minuteHand.StrokeThickness(minuteHandWidth);
        m_minuteHand.StrokeStartLineCap(PenLineCap::Round);
        m_minuteHand.StrokeEndLineCap(PenLineCap::Round);
        auto minTransform = RotateTransform();
        minTransform.CenterX(kCenter);
        minTransform.CenterY(kCenter);
        m_minuteHand.RenderTransform(minTransform);
        canvas.Children().Append(m_minuteHand);

        // Second hand
        m_secondHand = Line();
        m_secondHand.X1(kCenter);
        m_secondHand.Y1(kCenter + 15.0);
        m_secondHand.X2(kCenter);
        m_secondHand.Y2(kCenter - secondHandLength);
        m_secondHand.Stroke(secondColor);
        m_secondHand.StrokeThickness(secondHandWidth);
        auto secTransform = RotateTransform();
        secTransform.CenterX(kCenter);
        secTransform.CenterY(kCenter);
        m_secondHand.RenderTransform(secTransform);
        canvas.Children().Append(m_secondHand);

        // Center dot
        m_centerDot = Shapes::Ellipse();
        m_centerDot.Width(8);
        m_centerDot.Height(8);
        m_centerDot.Fill(secondColor);
        Canvas::SetLeft(m_centerDot, kCenter - 4);
        Canvas::SetTop(m_centerDot, kCenter - 4);
        canvas.Children().Append(m_centerDot);
    }

    void ClockCard::UpdateAnalogHands()
    {
        if (!m_hourHand) return;

        double hourAngle = (m_lastTimeInfo.hours % 12) * 30.0 + m_lastTimeInfo.minutes * 0.5;
        double minAngle = m_lastTimeInfo.minutes * 6.0 + m_lastTimeInfo.seconds * 0.1;
        double secAngle = m_lastTimeInfo.seconds * 6.0;

        m_hourHand.RenderTransform().as<RotateTransform>().Angle(hourAngle);
        m_minuteHand.RenderTransform().as<RotateTransform>().Angle(minAngle);
        m_secondHand.RenderTransform().as<RotateTransform>().Angle(secAngle);
    }

    void ClockCard::UpdateDigitalDisplay()
    {
        auto& ti = m_lastTimeInfo;

        switch (m_digitalStyle)
        {
        case Models::DigitalStyle::LED:
        {
            DigitalTimeText().FontFamily(Media::FontFamily(L"Consolas"));
            DigitalTimeText().FontSize(42);
            DigitalTimeText().Foreground(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 255, 100)));
            DigitalTimeText().Text(ti.formattedTime);
            DigitalSecondsText().Visibility(Visibility::Collapsed);
            break;
        }
        case Models::DigitalStyle::Clean:
        {
            int h12 = ti.hours % 12;
            if (h12 == 0) h12 = 12;
            auto ampm = ti.hours >= 12 ? L"PM" : L"AM";
            DigitalTimeText().FontFamily(Media::FontFamily(L"Segoe UI"));
            DigitalTimeText().FontSize(44);
            DigitalTimeText().ClearValue(TextBlock::ForegroundProperty());
            DigitalTimeText().Text(hstring(std::format(L"{}:{:02d}:{:02d} {}", h12, ti.minutes, ti.seconds, ampm)));
            DigitalSecondsText().Visibility(Visibility::Collapsed);
            break;
        }
        case Models::DigitalStyle::Retro:
        {
            DigitalTimeText().FontFamily(Media::FontFamily(L"Cascadia Mono"));
            DigitalTimeText().FontSize(38);
            DigitalTimeText().Foreground(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 50, 205, 50)));
            DigitalTimeText().Text(hstring(std::format(L"{:02d}.{:02d}.{:02d}", ti.hours, ti.minutes, ti.seconds)));
            DigitalSecondsText().Visibility(Visibility::Collapsed);
            break;
        }
        }
    }

    void ClockCard::UpdateTime()
    {
        m_lastTimeInfo = TzService().GetCurrentTime(m_timezoneId);

        if (m_displayMode == Models::ClockDisplayMode::Analog)
        {
            UpdateAnalogHands();
        }
        else
        {
            UpdateDigitalDisplay();
        }

        DateText().Text(m_lastTimeInfo.formattedDate);
        OffsetText().Text(m_lastTimeInfo.utcOffset);

        // Tint the card background continuously based on the local time at this
        // timezone. Five keyframes around the 24h cycle (deep night → sunrise →
        // midday → sunset → deep night) are interpolated so every hour gets a
        // distinct gradient.
        {
            double t = m_lastTimeInfo.hours + m_lastTimeInfo.minutes / 60.0; // 0..24

            struct Stop { double hour; Windows::UI::Color top; Windows::UI::Color bottom; };
            // 25 keyframes (one per hour, wrapping 24 → 0) tuned to evoke the
            // typical "feel" of each hour: deep night, pre-dawn, sunrise,
            // morning, midday, afternoon, golden hour, sunset, dusk, evening.
            static const Stop stops[] = {
                {  0.0, { 255,   6,  10,  32 }, { 255,   2,   4,  14 } }, // 00 midnight
                {  1.0, { 255,   8,  12,  36 }, { 255,   2,   4,  16 } }, // 01
                {  2.0, { 255,  10,  14,  40 }, { 255,   3,   5,  18 } }, // 02
                {  3.0, { 255,  14,  20,  50 }, { 255,   4,   8,  22 } }, // 03
                {  4.0, { 255,  30,  38,  78 }, { 255,  10,  16,  40 } }, // 04 pre-dawn
                {  5.0, { 255,  80,  80, 130 }, { 255,  40,  50,  90 } }, // 05 twilight
                {  6.0, { 255, 200, 140, 150 }, { 255, 130,  90, 140 } }, // 06 dawn pink
                {  7.0, { 255, 250, 180, 130 }, { 255, 230, 150, 130 } }, // 07 sunrise warm
                {  8.0, { 255, 230, 210, 170 }, { 255, 200, 220, 220 } }, // 08 early morning
                {  9.0, { 255, 180, 220, 240 }, { 255, 220, 235, 230 } }, // 09 fresh morning
                { 10.0, { 255, 140, 200, 240 }, { 255, 210, 230, 235 } }, // 10
                { 11.0, { 255, 110, 185, 240 }, { 255, 215, 230, 220 } }, // 11
                { 12.0, { 255,  90, 175, 240 }, { 255, 230, 225, 180 } }, // 12 midday bright
                { 13.0, { 255,  95, 180, 235 }, { 255, 230, 220, 175 } }, // 13
                { 14.0, { 255, 110, 185, 225 }, { 255, 235, 215, 165 } }, // 14
                { 15.0, { 255, 130, 185, 210 }, { 255, 240, 205, 150 } }, // 15 warming
                { 16.0, { 255, 170, 180, 195 }, { 255, 245, 195, 130 } }, // 16 late afternoon
                { 17.0, { 255, 230, 170, 130 }, { 255, 240, 160, 100 } }, // 17 golden hour
                { 18.0, { 255, 245, 130,  80 }, { 255, 210,  80, 110 } }, // 18 sunset
                { 19.0, { 255, 200,  90, 110 }, { 255, 110,  50, 120 } }, // 19 afterglow
                { 20.0, { 255, 110,  60, 130 }, { 255,  50,  30,  90 } }, // 20 dusk violet
                { 21.0, { 255,  60,  50, 110 }, { 255,  25,  20,  60 } }, // 21 early night
                { 22.0, { 255,  30,  35,  80 }, { 255,  10,  15,  40 } }, // 22 night
                { 23.0, { 255,  15,  20,  55 }, { 255,   5,   8,  25 } }, // 23 late night
                { 24.0, { 255,   6,  10,  32 }, { 255,   2,   4,  14 } }, // wrap to 00
            };

            auto lerpChannel = [](uint8_t a, uint8_t b, double f) -> uint8_t {
                double v = a + (b - a) * f;
                if (v < 0) v = 0; else if (v > 255) v = 255;
                return static_cast<uint8_t>(v);
            };
            auto lerpColor = [&](Windows::UI::Color a, Windows::UI::Color b, double f) {
                return Windows::UI::ColorHelper::FromArgb(
                    lerpChannel(a.A, b.A, f),
                    lerpChannel(a.R, b.R, f),
                    lerpChannel(a.G, b.G, f),
                    lerpChannel(a.B, b.B, f));
            };

            const Stop* lo = &stops[0];
            const Stop* hi = &stops[1];
            for (size_t i = 0; i + 1 < std::size(stops); ++i)
            {
                if (t >= stops[i].hour && t <= stops[i + 1].hour)
                {
                    lo = &stops[i];
                    hi = &stops[i + 1];
                    break;
                }
            }
            double span = hi->hour - lo->hour;
            double f = span > 0 ? (t - lo->hour) / span : 0.0;

            auto top    = lerpColor(lo->top,    hi->top,    f);
            auto bottom = lerpColor(lo->bottom, hi->bottom, f);

            auto gradient = LinearGradientBrush();
            gradient.StartPoint({ 0.0, 0.0 });
            gradient.EndPoint({ 0.0, 1.0 });
            GradientStop s1; s1.Color(top);    s1.Offset(0.0); gradient.GradientStops().Append(s1);
            GradientStop s2; s2.Color(bottom); s2.Offset(1.0); gradient.GradientStops().Append(s2);
            CardBorder().Background(gradient);
        }

        auto localTz = TzService().GetLocalTimezoneId();
        int diffMin = TzService().GetTimeDifferenceMinutes(localTz, m_timezoneId);
        if (diffMin == 0)
        {
            DiffText().Text(L"Local");
        }
        else
        {
            int absH = std::abs(diffMin) / 60;
            int absM = std::abs(diffMin) % 60;
            auto sign = diffMin > 0 ? L"+" : L"-";
            if (absM > 0)
                DiffText().Text(hstring(std::format(L"{}{}h{}m", sign, absH, absM)));
            else
                DiffText().Text(hstring(std::format(L"{}{}h", sign, absH)));
        }
    }
}
