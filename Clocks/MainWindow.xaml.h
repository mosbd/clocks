#pragma once

#include "MainWindow.g.h"
#include "SettingsService.h"

namespace winrt::Clocks::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        void NavView_SelectionChanged(
            Microsoft::UI::Xaml::Controls::NavigationView const& sender,
            Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);
        void NavView_ItemInvoked(
            Microsoft::UI::Xaml::Controls::NavigationView const& sender,
            Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args);
        void CompactView_DoubleTapped(
            winrt::Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
        void CompactView_KeyDown(
            winrt::Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void EscapeAccelerator_Invoked(
            Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender,
            Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args);

    private:
        void NavigateToPage(winrt::hstring const& tag);
        void EnterCompactMode();
        void ExitCompactMode();
        void UpdateCompactClocks();
        void PersistCompactStyles();

        bool m_isCompact{ false };
        double m_compactAspect{ 1.0 };
        HWND m_compactHwnd{};
        bool m_compactSubclassed{ false };
        static constexpr UINT_PTR kCompactSubclassId = 1;
        static LRESULT CALLBACK CompactSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                    UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        Microsoft::UI::Xaml::DispatcherTimer m_compactTimer{ nullptr };
        Services::SettingsService m_settingsService;
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
