#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "ClocksPage.xaml.h"
#include "TimeDiffPage.xaml.h"
#include "MeetingPlannerPage.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::Clocks::implementation
{
    MainWindow::MainWindow()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

        // After window is loaded, select the first nav item
        Activated([this](auto&&, auto&&)
        {
            // Select "Clocks" by default
            if (auto navView = NavView())
            {
                if (navView.MenuItems().Size() > 0)
                {
                    navView.SelectedItem(navView.MenuItems().GetAt(0));
                }
            }
        });
    }

    void MainWindow::NavView_SelectionChanged(
        NavigationView const& /*sender*/,
        NavigationViewSelectionChangedEventArgs const& args)
    {
        if (auto selectedItem = args.SelectedItem())
        {
            auto navItem = selectedItem.as<NavigationViewItem>();
            auto tag = winrt::unbox_value<winrt::hstring>(navItem.Tag());
            NavigateToPage(tag);
        }
    }

    void MainWindow::NavigateToPage(winrt::hstring const& tag)
    {
        if (tag == L"clocks")
        {
            ContentFrame().Navigate(winrt::xaml_typename<Clocks::ClocksPage>());
        }
        else if (tag == L"timediff")
        {
            ContentFrame().Navigate(winrt::xaml_typename<Clocks::TimeDiffPage>());
        }
        else if (tag == L"meeting")
        {
            ContentFrame().Navigate(winrt::xaml_typename<Clocks::MeetingPlannerPage>());
        }
    }
}
