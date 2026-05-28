#pragma once

#include "MeetingPlannerPage.g.h"

namespace winrt::Clocks::implementation
{
    struct MeetingPlannerPage : MeetingPlannerPageT<MeetingPlannerPage>
    {
        MeetingPlannerPage()
        {
        }
    };
}

namespace winrt::Clocks::factory_implementation
{
    struct MeetingPlannerPage : MeetingPlannerPageT<MeetingPlannerPage, implementation::MeetingPlannerPage>
    {
    };
}
