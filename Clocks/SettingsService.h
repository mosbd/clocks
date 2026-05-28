#pragma once
#include "pch.h"
#include "ClockModel.h"

namespace winrt::Clocks::Services
{
    class SettingsService
    {
    public:
        SettingsService();

        std::vector<Models::ClockModel> LoadClocks() const;
        void SaveClocks(const std::vector<Models::ClockModel>& clocks) const;
        bool IsFirstLaunch() const;
        void SetFirstLaunchComplete() const;

    private:
        winrt::hstring GetSettingsFilePath() const;
        winrt::Windows::Data::Json::JsonObject ClockToJson(const Models::ClockModel& clock) const;
        Models::ClockModel JsonToClock(const winrt::Windows::Data::Json::JsonObject& json) const;
    };
}
