#pragma once

#include "TimeDiffPage.g.h"

namespace winrt::Clocks::implementation
{
    struct TimeDiffPage : TimeDiffPageT<TimeDiffPage>
    {
        TimeDiffPage()
        {
        }
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct TimeDiffPage : TimeDiffPageT<TimeDiffPage, implementation::TimeDiffPage>
    {
    };
}
