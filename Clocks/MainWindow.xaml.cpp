#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "ClocksPage.xaml.h"
#include "TimeDiffPage.xaml.h"
#include "MeetingPlannerPage.xaml.h"
#include "ClockCard.xaml.h"

#include <microsoft.ui.xaml.window.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::Clocks::implementation
{
    MainWindow::~MainWindow()
    {
        if (m_compactTimer)
        {
            m_compactTimer.Stop();
            m_compactTimer = nullptr;
        }
        if (m_compactHwnd && m_compactSubclassed)
        {
            ::RemoveWindowSubclass(m_compactHwnd, &MainWindow::CompactSubclassProc, kCompactSubclassId);
            m_compactSubclassed = false;
        }
        m_compactHwnd = nullptr;
    }

    MainWindow::MainWindow()
    {
        InitializeComponent();

        // Extend content into the title bar and use a custom themed title bar.
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());

        // Set a reasonable initial window size scaled to the window's current monitor DPI.
        if (auto appWindow = this->AppWindow())
        {
            HWND hwnd{};
            if (auto windowNative = this->try_as<::IWindowNative>())
                windowNative->get_WindowHandle(&hwnd);
            UINT dpi = hwnd ? ::GetDpiForWindow(hwnd) : ::GetDpiForSystem();
            double scale = dpi / 96.0;
            appWindow.Resize({
                static_cast<int32_t>(1000 * scale),
                static_cast<int32_t>(700 * scale) });
        }
    }

    void MainWindow::NavView_SelectionChanged(
        NavigationView const&,
        NavigationViewSelectionChangedEventArgs const& args)
    {
        if (auto selectedItem = args.SelectedItem())
        {
            auto navItem = selectedItem.as<NavigationViewItem>();
            auto tag = unbox_value<hstring>(navItem.Tag());

            if (tag == L"compact")
            {
                // Action item; handled in ItemInvoked. Should not occur because
                // the compact item has SelectsOnInvoked="False".
                return;
            }
            NavigateToPage(tag);
        }
    }

    void MainWindow::NavView_ItemInvoked(
        NavigationView const&,
        NavigationViewItemInvokedEventArgs const& args)
    {
        if (auto invokedContainer = args.InvokedItemContainer().try_as<NavigationViewItem>())
        {
            auto tag = unbox_value<hstring>(invokedContainer.Tag());
            if (tag == L"compact")
            {
                EnterCompactMode();
            }
        }
    }

    void MainWindow::NavigateToPage(hstring const& tag)
    {
        if (tag == L"clocks")
            ContentFrame().Navigate(xaml_typename<Clocks::ClocksPage>());
        else if (tag == L"timediff")
            ContentFrame().Navigate(xaml_typename<Clocks::TimeDiffPage>());
        else if (tag == L"meeting")
            ContentFrame().Navigate(xaml_typename<Clocks::MeetingPlannerPage>());
    }

    void MainWindow::EnterCompactMode()
    {
        // Idempotent: ignore re-entry. Without this, repeated invocations would
        // stack DispatcherTimers, re-subclass the HWND, and reset window size.
        if (m_isCompact) return;

        m_isCompact = true;

        Title(L"");

        // Apply acrylic backdrop FIRST so the system can begin warming up the
        // composition pipeline in parallel with the presenter/layout changes.
        {
            auto acrylic = Microsoft::UI::Xaml::Media::DesktopAcrylicBackdrop();
            SystemBackdrop(acrylic);
        }

        auto appWindow = this->AppWindow();
        auto presenter = Microsoft::UI::Windowing::OverlappedPresenter::Create();
        presenter.IsAlwaysOnTop(true);
        // Keep the resize border, hide only the title bar. Combined with
        // ExtendsContentIntoTitleBar this gives a borderless-looking, resizable window.
        presenter.SetBorderAndTitleBar(true, false);
        presenter.IsResizable(true);
        presenter.IsMaximizable(false);
        presenter.IsMinimizable(false);
        appWindow.SetPresenter(presenter);

        // Chromeless: collapse the system caption area so there are no caption buttons.
        if (auto titleBar = appWindow.TitleBar())
        {
            titleBar.PreferredHeightOption(Microsoft::UI::Windowing::TitleBarHeightOption::Collapsed);
        }

        // Hide the regular UI and the custom title bar.
        AppTitleBar().Visibility(Visibility::Collapsed);
        NavView().Visibility(Visibility::Collapsed);
        CompactView().Visibility(Visibility::Visible);

        // Make the whole compact view a drag region.
        SetTitleBar(CompactView());
        DispatcherQueue().TryEnqueue([weak = get_weak()]()
        {
            if (auto self = weak.get())
            {
                self->CompactView().Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            }
        });

        UpdateCompactClocks();

        // Size the window to fit the cards (or restore the saved compact rect).
        auto savedRect = m_settingsService.LoadCompactRect();
        bool restored = false;
        if (savedRect.valid)
        {
            RECT rc{ savedRect.x, savedRect.y,
                     savedRect.x + savedRect.width, savedRect.y + savedRect.height };
            HMONITOR mon = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
            if (mon)
            {
                appWindow.MoveAndResize({ savedRect.x, savedRect.y, savedRect.width, savedRect.height });
                restored = true;
            }
        }
        if (!restored)
        {
            auto clocks = m_settingsService.LoadClocks();
            int n = std::min<int>(4, static_cast<int>(clocks.size()));
            if (n <= 0) n = 1;
            const int cardW = 180, cardH = 220, spacing = 8;
            const int hPadding = 16 + 16; // StackPanel Padding=8 (both sides) + extra slack
            const int vPadding = 16 + 16;
            int width = n * cardW + (n - 1) * spacing + hPadding;
            int height = cardH + vPadding;
            auto scale = CompactView().XamlRoot().RasterizationScale();
            appWindow.Resize({
                static_cast<int32_t>(width * scale),
                static_cast<int32_t>(height * scale) });
        }

        m_compactTimer = DispatcherTimer();
        m_compactTimer.Interval(std::chrono::seconds(1));
        m_compactTimer.Tick([this](auto&&, auto&&) { UpdateCompactClocks(); });
        m_compactTimer.Start();

        // Subclass the HWND to lock aspect ratio while resizing.
        {
            auto windowNative = this->try_as<::IWindowNative>();
            if (windowNative) windowNative->get_WindowHandle(&m_compactHwnd);
            if (m_compactHwnd)
            {
                RECT rc{};
                ::GetWindowRect(m_compactHwnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;
                if (h > 0) m_compactAspect = static_cast<double>(w) / h;
                m_compactSubclassed = ::SetWindowSubclass(
                    m_compactHwnd, &MainWindow::CompactSubclassProc,
                    kCompactSubclassId, reinterpret_cast<DWORD_PTR>(this)) != FALSE;

                // Make the compact window semi-transparent when another app is
                // in the foreground, fully opaque when this app is active.
                LONG_PTR exStyle = ::GetWindowLongPtrW(m_compactHwnd, GWL_EXSTYLE);
                ::SetWindowLongPtrW(m_compactHwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
                ::SetLayeredWindowAttributes(m_compactHwnd, 0, 255, LWA_ALPHA);
            }
        }
    }

    LRESULT CALLBACK MainWindow::CompactSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                    UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
    {
        auto self = reinterpret_cast<MainWindow*>(dwRefData);
        if (self && (msg == WM_ACTIVATEAPP || msg == WM_ACTIVATE || msg == WM_NCACTIVATE))
        {
            bool active;
            if (msg == WM_ACTIVATE)       active = LOWORD(wParam) != WA_INACTIVE;
            else                          active = wParam != 0;

            BYTE alpha = active ? 255 : 180;
            ::SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
            // Fall through to DefSubclassProc for WM_ACTIVATE/WM_NCACTIVATE so
            // the OS still draws the active/inactive caption correctly.
        }
        // Suppress the system menu on right-click of the non-client area
        // (the entire compact window is effectively NC since the title bar is hidden).
        if (self && (msg == WM_NCRBUTTONDOWN || msg == WM_NCRBUTTONUP || msg == WM_CONTEXTMENU))
        {
            return 0;
        }
        // Double-click on the drag region (entire compact window) exits compact mode.
        if (self && (msg == WM_NCLBUTTONDBLCLK || msg == WM_LBUTTONDBLCLK))
        {
            // Snapshot dispatcher + weak ref in a single deref of `self`, then
            // hand off the rest to the UI thread so `self` is no longer touched
            // by this subclass procedure after we return.
            auto dq = self->DispatcherQueue();
            auto weak = self->get_weak();
            dq.TryEnqueue([weak]()
            {
                if (auto strong = weak.get()) strong->ExitCompactMode();
            });
            return 0;
        }
        if (self && msg == WM_DPICHANGED)
        {
            // OS suggests a new rect when the window moves to a different-DPI monitor.
            RECT* suggested = reinterpret_cast<RECT*>(lParam);
            if (suggested)
            {
                ::SetWindowPos(hwnd, nullptr,
                    suggested->left, suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                self->m_settingsService.SaveCompactRect(
                    suggested->left, suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top);
            }
            return 0;
        }
        if (self && msg == WM_EXITSIZEMOVE)
        {
            RECT rc{};
            if (::GetWindowRect(hwnd, &rc))
            {
                self->m_settingsService.SaveCompactRect(rc.left, rc.top,
                                                        rc.right - rc.left, rc.bottom - rc.top);
            }
        }
        if (self && msg == WM_SIZING)
        {
            RECT* r = reinterpret_cast<RECT*>(lParam);
            int w = r->right - r->left;
            int h = r->bottom - r->top;
            int newW = w, newH = h;
            switch (wParam)
            {
            case WMSZ_LEFT: case WMSZ_RIGHT:
                newH = static_cast<int>(w / self->m_compactAspect);
                r->bottom = r->top + newH;
                break;
            case WMSZ_TOP: case WMSZ_BOTTOM:
                newW = static_cast<int>(h * self->m_compactAspect);
                r->right = r->left + newW;
                break;
            case WMSZ_TOPLEFT: case WMSZ_TOPRIGHT:
            case WMSZ_BOTTOMLEFT: case WMSZ_BOTTOMRIGHT:
                // Use the larger of the two adjustments to keep cursor on edge.
                if (w / self->m_compactAspect > h)
                    newH = static_cast<int>(w / self->m_compactAspect);
                else
                    newW = static_cast<int>(h * self->m_compactAspect);
                if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
                    r->top = r->bottom - newH;
                else
                    r->bottom = r->top + newH;
                if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
                    r->left = r->right - newW;
                else
                    r->right = r->left + newW;
                break;
            }
            return TRUE;
        }
        return ::DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::ExitCompactMode()
    {
        // Idempotent: ignore if we're not currently compact.
        if (!m_isCompact) return;

        m_isCompact = false;

        Title(L"Clocks");

        if (m_compactTimer)
        {
            m_compactTimer.Stop();
            m_compactTimer = nullptr;
        }

        // Remove subclass BEFORE swapping the presenter, while the HWND we hooked is still current.
        if (m_compactHwnd && m_compactSubclassed)
        {
            RECT rc{};
            if (::GetWindowRect(m_compactHwnd, &rc))
            {
                m_settingsService.SaveCompactRect(rc.left, rc.top,
                                                  rc.right - rc.left, rc.bottom - rc.top);
            }
            ::RemoveWindowSubclass(m_compactHwnd, &MainWindow::CompactSubclassProc, kCompactSubclassId);
            m_compactSubclassed = false;
        }
        if (m_compactHwnd)
        {
            LONG_PTR exStyle = ::GetWindowLongPtrW(m_compactHwnd, GWL_EXSTYLE);
            ::SetWindowLongPtrW(m_compactHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        }
        m_compactHwnd = nullptr;

        auto appWindow = this->AppWindow();
        appWindow.SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::Default);

        if (auto titleBar = appWindow.TitleBar())
        {
            titleBar.PreferredHeightOption(Microsoft::UI::Windowing::TitleBarHeightOption::Standard);
        }

        // Restore Mica backdrop and the normal title bar / nav.
        SystemBackdrop(Microsoft::UI::Xaml::Media::MicaBackdrop());
        AppTitleBar().Visibility(Visibility::Visible);
        SetTitleBar(AppTitleBar());

        CompactView().Visibility(Visibility::Collapsed);
        NavView().Visibility(Visibility::Visible);
        CompactClockList().Children().Clear();
        CompactClockList().ColumnDefinitions().Clear();

        // Refresh the ClocksPage (if currently shown) so any style changes
        // made in compact mode are reflected.
        if (auto page = ContentFrame().Content().try_as<Clocks::ClocksPage>())
        {
            page.ReloadClocks();
        }
    }

    void MainWindow::CompactView_DoubleTapped(
        IInspectable const&,
        Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const&)
    {
        ExitCompactMode();
    }

    void MainWindow::CompactView_KeyDown(
        IInspectable const&,
        Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e)
    {
        if (e.Key() == Windows::System::VirtualKey::Escape)
        {
            ExitCompactMode();
        }
    }

    void MainWindow::EscapeAccelerator_Invoked(
        Microsoft::UI::Xaml::Input::KeyboardAccelerator const&,
        Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args)
    {
        if (m_isCompact)
        {
            args.Handled(true);
            ExitCompactMode();
        }
    }

    void MainWindow::PersistCompactStyles()
    {
        auto panel = CompactClockList();
        auto clocks = m_settingsService.LoadClocks();
        for (uint32_t i = 0; i < panel.Children().Size(); ++i)
        {
            auto card = panel.Children().GetAt(i).try_as<Clocks::ClockCard>();
            if (!card) continue;
            auto tz = card.TimezoneId();
            for (auto& c : clocks)
            {
                if (c.TimezoneId == tz)
                {
                    c.DisplayMode = static_cast<Models::ClockDisplayMode>(card.DisplayMode());
                    c.AnalogVisualStyle = static_cast<Models::AnalogStyle>(card.AnalogStyle());
                    c.DigitalVisualStyle = static_cast<Models::DigitalStyle>(card.DigitalStyle());
                    break;
                }
            }
        }
        m_settingsService.SaveClocks(clocks);
    }

    void MainWindow::UpdateCompactClocks()
    {
        auto panel = CompactClockList();

        if (panel.Children().Size() == 0)
        {
            auto clocks = m_settingsService.LoadClocks();
            int n = std::min<int>(4, static_cast<int>(clocks.size()));
            panel.ColumnDefinitions().Clear();
            for (int i = 0; i < n; ++i)
            {
                auto col = ColumnDefinition();
                col.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
                panel.ColumnDefinitions().Append(col);
            }

            int count = 0;
            for (const auto& clock : clocks)
            {
                if (count >= n) break;
                auto card = winrt::make<Clocks::implementation::ClockCard>();
                card.Initialize(clock.TimezoneId, clock.DisplayName);
                card.SetAnalogStyle(static_cast<int32_t>(clock.AnalogVisualStyle));
                card.SetDigitalStyle(static_cast<int32_t>(clock.DigitalVisualStyle));
                if (static_cast<int32_t>(clock.DisplayMode) != card.DisplayMode())
                {
                    card.ToggleDisplayMode();
                }
                card.StyleChanged([this](auto&&, auto&&) { PersistCompactStyles(); });
                card.HorizontalAlignment(HorizontalAlignment::Stretch);
                card.VerticalAlignment(VerticalAlignment::Stretch);
                Grid::SetColumn(card, count);
                panel.Children().Append(card);
                count++;
            }
        }
        else
        {
            for (uint32_t i = 0; i < panel.Children().Size(); i++)
            {
                if (auto card = panel.Children().GetAt(i).try_as<Clocks::ClockCard>())
                {
                    card.UpdateTime();
                }
            }
        }
    }
}
