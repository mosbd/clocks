#pragma once

#include "ClocksPage.g.h"

namespace winrt::Clocks::implementation
{
    struct ClocksPage : ClocksPageT<ClocksPage>
    {
        ClocksPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
        }
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct ClocksPage : ClocksPageT<ClocksPage, implementation::ClocksPage>
    {
    };
}
