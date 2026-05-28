#pragma once
#include "pch.h"

namespace winrt::Clocks::Models
{
    enum class ClockDisplayMode
    {
        Analog,
        Digital
    };

    enum class AnalogStyle
    {
        Classic,
        Modern,
        Minimal
    };

    enum class DigitalStyle
    {
        LED,
        Clean,
        Retro
    };

    struct ClockModel
    {
        winrt::hstring TimezoneId;
        winrt::hstring DisplayName;
        winrt::hstring Country;
        ClockDisplayMode DisplayMode{ ClockDisplayMode::Analog };
        AnalogStyle AnalogVisualStyle{ AnalogStyle::Modern };
        DigitalStyle DigitalVisualStyle{ DigitalStyle::Clean };
    };
}
