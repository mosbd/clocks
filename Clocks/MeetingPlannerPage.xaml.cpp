#include "pch.h"
#include "MeetingPlannerPage.xaml.h"
#if __has_include("MeetingPlannerPage.g.cpp")
#include "MeetingPlannerPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;

namespace Shapes = Microsoft::UI::Xaml::Shapes;
namespace Media = Microsoft::UI::Xaml::Media;

namespace winrt::Clocks::implementation
{
    MeetingPlannerPage::~MeetingPlannerPage()
    {
        if (m_timer)
        {
            m_timer.Stop();
            m_timer = nullptr;
        }
    }

    MeetingPlannerPage::MeetingPlannerPage()
    {
        Loaded([this](auto&&, auto&&)
        {
            // Idempotent: a cached page can be reloaded; rebuild zone state
            // and the timer from scratch every time so we don't stack duplicates.
            m_zones.clear();

            // Add saved clocks first so their persisted display names take precedence.
            auto savedClocks = m_settingsService.LoadClocks();
            for (const auto& c : savedClocks)
            {
                AddTimezone(c.TimezoneId, c.DisplayName);
            }

            // Ensure the local timezone is present (AddTimezone dedupes by tz id).
            auto localTz = m_tzService.GetLocalTimezoneId();
            auto cities = m_tzService.GetAllCities();
            hstring cityName = L"Local";
            for (const auto& c : cities)
            {
                if (c.TimezoneId == localTz) { cityName = c.CityName; break; }
            }
            AddTimezone(localTz, cityName);

            if (m_timer) { m_timer.Stop(); m_timer = nullptr; }
            m_timer = DispatcherTimer();
            m_timer.Interval(std::chrono::seconds(30));
            m_timer.Tick([this](auto&&, auto&&) { RebuildTimeline(); });
            m_timer.Start();
        });

        Unloaded([this](auto&&, auto&&)
        {
            if (m_timer) { m_timer.Stop(); m_timer = nullptr; }
        });
    }

    void MeetingPlannerPage::BusinessHours_Changed(NumberBox const&, NumberBoxValueChangedEventArgs const&)
    {
        RebuildTimeline();
    }

    void MeetingPlannerPage::AddTimezone(hstring const& tzId, hstring const& cityName)
    {
        // Check for duplicate
        for (const auto& z : m_zones)
        {
            if (z.timezoneId == tzId) return;
        }
        m_zones.push_back({ tzId, cityName });
        RebuildTimeline();
    }

    void MeetingPlannerPage::RebuildTimeline()
    {
        auto panel = TimelinePanel();
        if (!panel) return; // not constructed yet (early ValueChanged during init)
        panel.Children().Clear();

        if (m_zones.empty()) return;

        int startHour = static_cast<int>(StartHourBox().Value());
        int endHour = static_cast<int>(EndHourBox().Value());
        if (endHour <= startHour) endHour = startHour + 1;

        // Hour labels header
        auto headerGrid = Grid();
        headerGrid.Margin(ThicknessHelper::FromLengths(140, 0, 0, 0));
        auto hourLabels = Canvas();
        hourLabels.Height(20);
        double totalWidth = 720.0; // 24 hours * 30px each
        for (int h = 0; h < 24; h += 3)
        {
            auto tb = TextBlock();
            tb.Text(hstring(std::format(L"{:02d}:00", h)));
            tb.FontSize(10);
            tb.Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 140, 140, 140)));
            Canvas::SetLeft(tb, h * 30.0);
            Canvas::SetTop(tb, 0);
            hourLabels.Children().Append(tb);
        }
        headerGrid.Children().Append(hourLabels);
        panel.Children().Append(headerGrid);

        // Compute overlap at minute granularity so half/quarter-hour zones
        // (India +5:30, Nepal +5:45, Newfoundland −3:30, etc.) are honored,
        // and use a per-instant offset lookup so DST transitions falling within
        // the displayed day produce correct local-time-to-UTC mapping.
        constexpr int kMinutesPerDay = 1440;
        std::vector<uint8_t> overlapMin(kMinutesPerDay, 1);
        int startMin = startHour * 60;
        int endMin = endHour * 60;

        // Anchor the 24-hour strip to today's UTC midnight. Every minute on the
        // strip then maps to an exact UTC instant, and we ask each zone for its
        // offset at THAT instant rather than reusing "now's" offset everywhere.
        auto nowTp = std::chrono::system_clock::now();
        auto utcMidnight = std::chrono::floor<std::chrono::days>(nowTp);

        std::vector<std::vector<uint8_t>> bizPerZone(m_zones.size(),
            std::vector<uint8_t>(kMinutesPerDay, 0));

        for (size_t zi = 0; zi < m_zones.size(); ++zi)
        {
            const auto& zone = m_zones[zi];
            for (int utcM = 0; utcM < kMinutesPerDay; ++utcM)
            {
                std::chrono::system_clock::time_point instant =
                    utcMidnight + std::chrono::minutes(utcM);
                int offsetMin = m_tzService.GetOffsetMinutesAt(zone.timezoneId, instant);
                int localM = (utcM + offsetMin) % kMinutesPerDay;
                if (localM < 0) localM += kMinutesPerDay;
                bool inBiz = (localM >= startMin && localM < endMin);
                bizPerZone[zi][utcM] = inBiz ? 1 : 0;
                if (!inBiz) overlapMin[utcM] = 0;
            }
        }

        // Build timeline rows
        for (size_t zi = 0; zi < m_zones.size(); ++zi)
        {
            panel.Children().Append(CreateTimelineRow(m_zones[zi], bizPerZone[zi]));
        }

        // Overlap summary row
        auto overlapRow = Grid();
        overlapRow.ColumnDefinitions().Append(ColumnDefinition());
        overlapRow.ColumnDefinitions().GetAt(0).Width(GridLengthHelper::FromPixels(140));
        overlapRow.ColumnDefinitions().Append(ColumnDefinition());

        auto overlapLabel = TextBlock();
        overlapLabel.Text(L"Overlap");
        overlapLabel.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
        overlapLabel.Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 180, 80)));
        overlapLabel.VerticalAlignment(VerticalAlignment::Center);
        overlapLabel.Margin(ThicknessHelper::FromLengths(0, 0, 8, 0));
        Grid::SetColumn(overlapLabel, 0);
        overlapRow.Children().Append(overlapLabel);

        auto overlapCanvas = Canvas();
        overlapCanvas.Height(24);
        overlapCanvas.Width(totalWidth);

        // Background
        auto overlapBg = Shapes::Rectangle();
        overlapBg.Width(totalWidth);
        overlapBg.Height(24);
        overlapBg.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(30, 128, 128, 128)));
        overlapBg.RadiusX(4);
        overlapBg.RadiusY(4);
        overlapCanvas.Children().Append(overlapBg);

        // Draw overlap as contiguous runs at minute precision (30px/hour = 0.5px/min).
        constexpr double kPxPerMin = 30.0 / 60.0;
        int i = 0;
        while (i < kMinutesPerDay)
        {
            if (!overlapMin[i]) { ++i; continue; }
            int j = i;
            while (j < kMinutesPerDay && overlapMin[j]) ++j;
            auto seg = Shapes::Rectangle();
            seg.Width((j - i) * kPxPerMin);
            seg.Height(24);
            seg.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(200, 0, 180, 80)));
            seg.RadiusX(2);
            seg.RadiusY(2);
            Canvas::SetLeft(seg, i * kPxPerMin);
            overlapCanvas.Children().Append(seg);
            i = j;
        }

        Grid::SetColumn(overlapCanvas, 1);
        overlapRow.Children().Append(overlapCanvas);
        panel.Children().Append(overlapRow);

        // Summary text (counted in minutes, displayed as h/m)
        int overlapMinutes = 0;
        for (uint8_t b : overlapMin) if (b) ++overlapMinutes;
        auto summaryText = TextBlock();
        if (overlapMinutes > 0)
        {
            int hh = overlapMinutes / 60;
            int mm = overlapMinutes % 60;
            if (mm == 0)
                summaryText.Text(hstring(std::format(L"{}h overlapping business hours", hh)));
            else
                summaryText.Text(hstring(std::format(L"{}h {:02d}m overlapping business hours", hh, mm)));
        }
        else
        {
            summaryText.Text(L"No overlapping business hours found");
        }
        summaryText.FontSize(12);
        summaryText.Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 140, 140, 140)));
        summaryText.Margin(ThicknessHelper::FromLengths(140, 8, 0, 0));
        panel.Children().Append(summaryText);
    }

    UIElement MeetingPlannerPage::CreateTimelineRow(
        ZoneEntry const& entry,
        std::vector<uint8_t> const& bizMinByUtc)
    {
        double totalWidth = 720.0;

        auto rowGrid = Grid();
        rowGrid.ColumnDefinitions().Append(ColumnDefinition());
        rowGrid.ColumnDefinitions().GetAt(0).Width(GridLengthHelper::FromPixels(140));
        rowGrid.ColumnDefinitions().Append(ColumnDefinition());

        // City label + remove button
        auto labelPanel = StackPanel();
        labelPanel.Orientation(Orientation::Horizontal);
        labelPanel.VerticalAlignment(VerticalAlignment::Center);

        auto cityLabel = TextBlock();
        cityLabel.Text(entry.cityName);
        cityLabel.VerticalAlignment(VerticalAlignment::Center);
        cityLabel.TextTrimming(TextTrimming::CharacterEllipsis);
        cityLabel.MaxWidth(110);
        labelPanel.Children().Append(cityLabel);

        auto removeBtn = Button();
        removeBtn.Padding(ThicknessHelper::FromUniformLength(2));
        removeBtn.Background(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0)));
        removeBtn.BorderThickness(ThicknessHelper::FromUniformLength(0));
        removeBtn.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
        auto removeIcon = FontIcon();
        removeIcon.Glyph(L"\xE711");
        removeIcon.FontSize(10);
        removeBtn.Content(removeIcon);

        hstring tzId = entry.timezoneId;
        removeBtn.Click([weak = get_weak(), tzId](auto&&, auto&&)
        {
            auto self = weak.get();
            if (!self) return;
            self->m_zones.erase(
                std::remove_if(self->m_zones.begin(), self->m_zones.end(),
                    [&tzId](const ZoneEntry& z) { return z.timezoneId == tzId; }),
                self->m_zones.end());
            self->RebuildTimeline();
        });
        labelPanel.Children().Append(removeBtn);

        Grid::SetColumn(labelPanel, 0);
        rowGrid.Children().Append(labelPanel);

        // Timeline bar
        auto canvas = Canvas();
        canvas.Height(28);
        canvas.Width(totalWidth);

        // Full 24h background
        auto bg = Shapes::Rectangle();
        bg.Width(totalWidth);
        bg.Height(28);
        bg.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(30, 128, 128, 128)));
        bg.RadiusX(4);
        bg.RadiusY(4);
        canvas.Children().Append(bg);

        // Business hours highlight: draw contiguous runs from the precomputed
        // per-minute mask. This is DST-correct because the mask was built by
        // sampling the zone's offset at each minute's UTC instant.
        auto ti = m_tzService.GetCurrentTime(entry.timezoneId);
        constexpr double kPxPerMin = 30.0 / 60.0;
        constexpr int kMinutesPerDay = 1440;

        auto addSegment = [&](double xPx, double widthPx)
        {
            auto seg = Shapes::Rectangle();
            seg.Width(widthPx);
            seg.Height(28);
            seg.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(180, 0, 120, 212)));
            seg.RadiusX(2);
            seg.RadiusY(2);
            Canvas::SetLeft(seg, xPx);
            canvas.Children().Append(seg);
        };

        {
            int i = 0;
            while (i < kMinutesPerDay)
            {
                if (i >= static_cast<int>(bizMinByUtc.size()) || !bizMinByUtc[i]) { ++i; continue; }
                int j = i;
                while (j < kMinutesPerDay && j < static_cast<int>(bizMinByUtc.size()) && bizMinByUtc[j]) ++j;
                addSegment(i * kPxPerMin, (j - i) * kPxPerMin);
                i = j;
            }
        }

        // Current time marker: use the zone's offset AT NOW (not at today's
        // UTC midnight), so the marker is accurate even on a DST transition day.
        int offsetMin = ti.totalOffsetMinutes;
        int currentUtcMin = ti.hours * 60 + ti.minutes - offsetMin;
        currentUtcMin %= kMinutesPerDay;
        if (currentUtcMin < 0) currentUtcMin += kMinutesPerDay;
        double markerX = currentUtcMin * kPxPerMin;

        auto marker = Shapes::Rectangle();
        marker.Width(2);
        marker.Height(28);
        marker.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 255, 60, 60)));
        Canvas::SetLeft(marker, markerX);
        canvas.Children().Append(marker);

        // Local time label
        auto timeLabel = TextBlock();
        timeLabel.Text(ti.formattedTime);
        timeLabel.FontSize(9);
        timeLabel.Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 220, 220, 220)));
        Canvas::SetLeft(timeLabel, markerX + 4);
        Canvas::SetTop(timeLabel, 6);
        canvas.Children().Append(timeLabel);

        Grid::SetColumn(canvas, 1);
        rowGrid.Children().Append(canvas);

        return rowGrid;
    }
}
