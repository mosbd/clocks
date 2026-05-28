#include "pch.h"
#include "ClocksPage.xaml.h"
#if __has_include("ClocksPage.g.cpp")
#include "ClocksPage.g.cpp"
#endif
#include "ClockCard.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace winrt::Clocks::implementation
{
    ClocksPage::ClocksPage()
    {
        m_clockCards = winrt::single_threaded_observable_vector<UIElement>();

        Loaded([this](auto&&, auto&&)
        {
            // One-time wiring of controls owned by the page's visual tree.
            if (!m_wired)
            {
                m_addClickToken = AddClockButton().Click({ this, &ClocksPage::AddClock_Click });
                ClockGrid().ItemsSource(m_clockCards);
                m_wired = true;
            }

            // Idempotent reload: pages can be unloaded and reloaded (e.g. when
            // cached by the navigation frame). Clear the card list so we don't
            // stack duplicates each time.
            m_clockCards.Clear();
            LoadClocks();

            if (m_timer) { m_timer.Stop(); m_timer = nullptr; }
            m_timer = DispatcherTimer();
            m_timer.Interval(std::chrono::seconds(1));
            m_timer.Tick({ this, &ClocksPage::OnTimerTick });
            m_timer.Start();
        });

        Unloaded([this](auto&&, auto&&)
        {
            if (m_timer) { m_timer.Stop(); m_timer = nullptr; }
        });
    }

    ClocksPage::~ClocksPage()
    {
        if (m_timer)
        {
            m_timer.Stop();
        }
    }

    void ClocksPage::ReloadClocks()
    {
        if (!m_clockCards) return;
        m_clockCards.Clear();
        LoadClocks();
    }

    void ClocksPage::LoadClocks()
    {
        auto saved = m_settingsService.LoadClocks();
        if (saved.empty())
        {
            auto localTz = m_tzService.GetLocalTimezoneId();
            auto cities = m_tzService.GetAllCities();
            hstring cityName = L"Local";
            for (const auto& c : cities)
            {
                if (c.TimezoneId == localTz)
                {
                    cityName = c.CityName;
                    break;
                }
            }
            AddClockCard(localTz, cityName);
            SaveClocks();
        }
        else
        {
            for (const auto& clock : saved)
            {
                AddClockCard(clock.TimezoneId, clock.DisplayName,
                             clock.DisplayMode, clock.AnalogVisualStyle, clock.DigitalVisualStyle);
            }
        }
        UpdateEmptyState();
    }

    void ClocksPage::SaveClocks()
    {
        std::vector<Models::ClockModel> models;
        for (uint32_t i = 0; i < m_clockCards.Size(); i++)
        {
            auto card = m_clockCards.GetAt(i).as<Clocks::ClockCard>();
            Models::ClockModel model;
            model.TimezoneId = card.TimezoneId();
            model.DisplayName = card.CityName();
            model.DisplayMode = static_cast<Models::ClockDisplayMode>(card.DisplayMode());
            model.AnalogVisualStyle = static_cast<Models::AnalogStyle>(card.AnalogStyle());
            model.DigitalVisualStyle = static_cast<Models::DigitalStyle>(card.DigitalStyle());
            models.push_back(model);
        }
        m_settingsService.SaveClocks(models);
    }

    void ClocksPage::AddClockCard(hstring const& timezoneId, hstring const& cityName,
                                  Models::ClockDisplayMode displayMode,
                                  Models::AnalogStyle analogStyle,
                                  Models::DigitalStyle digitalStyle)
    {
        auto card = winrt::make<Clocks::implementation::ClockCard>();
        card.Initialize(timezoneId, cityName);
        // Apply persisted style without firing save (StyleChanged not yet hooked).
        card.SetAnalogStyle(static_cast<int32_t>(analogStyle));
        card.SetDigitalStyle(static_cast<int32_t>(digitalStyle));
        if (static_cast<int32_t>(displayMode) != card.DisplayMode())
        {
            card.ToggleDisplayMode();
        }
        card.RemoveRequested({ this, &ClocksPage::OnClockRemoveRequested });
        card.StyleChanged({ this, &ClocksPage::OnClockStyleChanged });
        m_clockCards.Append(card);
    }

    void ClocksPage::OnClockStyleChanged(IInspectable const&, hstring const&)
    {
        SaveClocks();
    }

    void ClocksPage::OnClockRemoveRequested(IInspectable const& sender, hstring const&)
    {
        for (uint32_t i = 0; i < m_clockCards.Size(); i++)
        {
            if (m_clockCards.GetAt(i) == sender.as<UIElement>())
            {
                m_clockCards.RemoveAt(i);
                SaveClocks();
                UpdateEmptyState();
                break;
            }
        }
    }

    void ClocksPage::OnTimerTick(IInspectable const&, IInspectable const&)
    {
        for (uint32_t i = 0; i < m_clockCards.Size(); i++)
        {
            auto card = m_clockCards.GetAt(i).as<Clocks::ClockCard>();
            card.UpdateTime();
        }
    }

    winrt::fire_and_forget ClocksPage::AddClock_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto lifetime = get_strong();

        auto dialog = ContentDialog();
        dialog.Title(box_value(L"Add Clock"));
        dialog.PrimaryButtonText(L"Add");
        dialog.CloseButtonText(L"Cancel");
        dialog.DefaultButton(ContentDialogButton::Primary);
        dialog.XamlRoot(this->XamlRoot());

        auto suggestBox = AutoSuggestBox();
        suggestBox.PlaceholderText(L"Search city or timezone...");
        suggestBox.QueryIcon(SymbolIcon(Symbol::Find));

        // Per-invocation selection state. Owned by the lambdas via shared_ptr
        // so it survives across callbacks without leaning on page-scoped fields
        // (which would race if multiple dialogs were ever open at once).
        auto selection = std::make_shared<std::pair<hstring, hstring>>();

        suggestBox.TextChanged([weak = get_weak(), selection]
            (AutoSuggestBox const& sender, AutoSuggestBoxTextChangedEventArgs const& args)
        {
            if (args.Reason() != AutoSuggestionBoxTextChangeReason::UserInput) return;
            auto self = weak.get();
            if (!self) return;
            auto results = self->m_tzService.SearchCities(sender.Text());
            auto items = winrt::single_threaded_vector<IInspectable>();
            for (const auto& city : results)
            {
                items.Append(box_value(city.CityName + L" (" + city.TimezoneId + L")"));
            }
            sender.ItemsSource(items);
        });

        suggestBox.SuggestionChosen([selection]
            (AutoSuggestBox const&, AutoSuggestBoxSuggestionChosenEventArgs const& args)
        {
            auto selected = unbox_value<hstring>(args.SelectedItem());
            std::wstring str(selected);
            auto openParen = str.rfind(L'(');
            auto closeParen = str.rfind(L')');
            if (openParen != std::wstring::npos && closeParen != std::wstring::npos)
            {
                selection->first = hstring(str.substr(openParen + 1, closeParen - openParen - 1));
                std::wstring city = str.substr(0, openParen);
                while (!city.empty() && city.back() == L' ') city.pop_back();
                selection->second = hstring(city);
            }
        });

        dialog.Content(suggestBox);

        // co_await keeps us on the UI thread — no threading issues
        auto result = co_await dialog.ShowAsync();

        if (result == ContentDialogResult::Primary && !selection->first.empty())
        {
            AddClockCard(selection->first, selection->second);
            SaveClocks();
            UpdateEmptyState();
        }
    }

    void ClocksPage::UpdateEmptyState()
    {
        if (m_clockCards.Size() == 0)
        {
            EmptyState().Visibility(Visibility::Visible);
            ClockScrollViewer().Visibility(Visibility::Collapsed);
            TitleText().Text(L"World Clocks");
        }
        else
        {
            EmptyState().Visibility(Visibility::Collapsed);
            ClockScrollViewer().Visibility(Visibility::Visible);
            TitleText().Text(hstring(std::format(L"World Clocks ({})", m_clockCards.Size())));
        }
    }
}
