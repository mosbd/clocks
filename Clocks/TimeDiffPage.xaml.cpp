#include "pch.h"
#include "TimeDiffPage.xaml.h"
#if __has_include("TimeDiffPage.g.cpp")
#include "TimeDiffPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;

namespace winrt::Clocks::implementation
{
    TimeDiffPage::~TimeDiffPage()
    {
        if (m_timer)
        {
            m_timer.Stop();
            m_timer = nullptr;
        }
    }

    TimeDiffPage::TimeDiffPage()
    {
        Loaded([this](auto&&, auto&&)
        {
            PopulateCityBoxes();

            // Default selections: From = local, To = first non-local clock.
            int fromIdx = -1, toIdx = -1;
            for (size_t i = 0; i < m_options.size(); ++i)
            {
                if (m_options[i].tzId == m_fromTzId && fromIdx < 0) fromIdx = static_cast<int>(i);
                if (m_options[i].tzId != m_fromTzId && toIdx < 0)   toIdx   = static_cast<int>(i);
            }
            if (fromIdx >= 0) FromCityBox().SelectedIndex(fromIdx);
            if (toIdx   >= 0) ToCityBox().SelectedIndex(toIdx);

            m_timer = DispatcherTimer();
            m_timer.Interval(std::chrono::seconds(1));
            m_timer.Tick([this](auto&&, auto&&) { UpdateTimesAndDiff(); });
            m_timer.Start();

            UpdateTimesAndDiff();
        });

        Unloaded([this](auto&&, auto&&)
        {
            if (m_timer) m_timer.Stop();
        });
    }

    void TimeDiffPage::PopulateCityBoxes()
    {
        m_options.clear();
        m_fromTzId = m_tzService.GetLocalTimezoneId();

        // Use saved clock names first (these are the authoritative display names).
        auto saved = m_settingsService.LoadClocks();
        for (const auto& c : saved)
        {
            m_options.push_back({ c.TimezoneId, c.DisplayName });
        }

        // Ensure local timezone is present (dedupe by tz id).
        bool hasLocal = false;
        for (const auto& o : m_options)
        {
            if (o.tzId == m_fromTzId) { m_fromCityName = o.displayName; hasLocal = true; break; }
        }
        if (!hasLocal)
        {
            hstring localName = L"Local";
            for (const auto& c : m_tzService.GetAllCities())
            {
                if (c.TimezoneId == m_fromTzId) { localName = c.CityName; break; }
            }
            m_fromCityName = localName;
            m_options.insert(m_options.begin(), { m_fromTzId, localName });
        }

        auto items = single_threaded_vector<IInspectable>();
        for (const auto& o : m_options) items.Append(box_value(o.displayName));
        FromCityBox().ItemsSource(items);
        ToCityBox().ItemsSource(items);
    }

    void TimeDiffPage::FromCity_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        int idx = FromCityBox().SelectedIndex();
        if (idx >= 0 && static_cast<size_t>(idx) < m_options.size())
        {
            m_fromTzId = m_options[idx].tzId;
            m_fromCityName = m_options[idx].displayName;
            UpdateTimesAndDiff();
        }
    }

    void TimeDiffPage::ToCity_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        int idx = ToCityBox().SelectedIndex();
        if (idx >= 0 && static_cast<size_t>(idx) < m_options.size())
        {
            m_toTzId = m_options[idx].tzId;
            m_toCityName = m_options[idx].displayName;
            UpdateTimesAndDiff();
        }
    }

    void TimeDiffPage::UpdateTimesAndDiff()
    {
        // Update "From" time display
        if (!m_fromTzId.empty())
        {
            auto ti = m_tzService.GetCurrentTime(m_fromTzId);
            FromTimeText().Text(ti.formattedTime);
            FromDateText().Text(ti.formattedDate);
            FromOffsetText().Text(ti.utcOffset);
        }

        // Update "To" time display
        if (!m_toTzId.empty())
        {
            auto ti = m_tzService.GetCurrentTime(m_toTzId);
            ToTimeText().Text(ti.formattedTime);
            ToDateText().Text(ti.formattedDate);
            ToOffsetText().Text(ti.utcOffset);
        }

        // Show difference if both are selected
        if (!m_fromTzId.empty() && !m_toTzId.empty())
        {
            int diffMin = m_tzService.GetTimeDifferenceMinutes(m_fromTzId, m_toTzId);
            ResultCard().Visibility(Visibility::Visible);

            int absH = std::abs(diffMin) / 60;
            int absM = std::abs(diffMin) % 60;

            if (diffMin == 0)
            {
                DiffResultText().Text(L"Same time zone");
                DiffDetailText().Text(L"No time difference");
            }
            else
            {
                // Format difference
                hstring diffStr;
                if (absM > 0)
                    diffStr = hstring(std::format(L"{} hour{} {} minute{}",
                        absH, absH != 1 ? L"s" : L"", absM, absM != 1 ? L"s" : L""));
                else
                    diffStr = hstring(std::format(L"{} hour{}", absH, absH != 1 ? L"s" : L""));

                DiffResultText().Text(diffStr);

                auto direction = diffMin > 0 ? L"ahead of" : L"behind";
                DiffDetailText().Text(hstring(std::format(L"{} is {} {}",
                    std::wstring_view(m_toCityName), direction, std::wstring_view(m_fromCityName))));
            }
        }
        else
        {
            ResultCard().Visibility(Visibility::Collapsed);
        }
    }
}
