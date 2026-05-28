#include "pch.h"
#include "SettingsService.h"

using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Storage;

namespace winrt::Clocks::Services
{
    SettingsService::SettingsService()
    {
    }

    winrt::hstring SettingsService::GetSettingsFilePath() const
    {
        auto localFolder = ApplicationData::Current().LocalFolder();
        return localFolder.Path() + L"\\clocks_settings.json";
    }

    std::vector<Models::ClockModel> SettingsService::LoadClocks() const
    {
        std::vector<Models::ClockModel> clocks;
        try
        {
            std::wstring path(GetSettingsFilePath());
            std::wifstream file(path);
            if (!file.is_open()) return clocks;

            std::wstringstream buffer;
            buffer << file.rdbuf();
            std::wstring content = buffer.str();
            file.close();

            if (content.empty()) return clocks;

            JsonObject root;
            if (!JsonObject::TryParse(winrt::hstring(content), root)) return clocks;

            if (root.HasKey(L"clocks"))
            {
                auto arr = root.GetNamedArray(L"clocks");
                for (uint32_t i = 0; i < arr.Size(); ++i)
                {
                    auto obj = arr.GetObjectAt(i);
                    clocks.push_back(JsonToClock(obj));
                }
            }
        }
        catch (...)
        {
            // Return empty on any error
        }
        return clocks;
    }

    void SettingsService::SaveClocks(const std::vector<Models::ClockModel>& clocks) const
    {
        try
        {
            JsonObject root;
            JsonArray arr;
            for (const auto& clock : clocks)
            {
                arr.Append(ClockToJson(clock));
            }
            root.SetNamedValue(L"clocks", arr);
            root.SetNamedValue(L"firstLaunchComplete", JsonValue::CreateBooleanValue(true));

            std::wstring path(GetSettingsFilePath());
            std::wofstream file(path);
            if (file.is_open())
            {
                file << std::wstring_view(root.Stringify());
                file.close();
            }
        }
        catch (...)
        {
            // Silently fail
        }
    }

    bool SettingsService::IsFirstLaunch() const
    {
        try
        {
            std::wstring path(GetSettingsFilePath());
            std::wifstream file(path);
            if (!file.is_open()) return true;

            std::wstringstream buffer;
            buffer << file.rdbuf();
            std::wstring content = buffer.str();
            file.close();

            JsonObject root;
            if (!JsonObject::TryParse(winrt::hstring(content), root)) return true;

            if (root.HasKey(L"firstLaunchComplete"))
            {
                return !root.GetNamedBoolean(L"firstLaunchComplete");
            }
        }
        catch (...)
        {
        }
        return true;
    }

    void SettingsService::SetFirstLaunchComplete() const
    {
        // Will be set when clocks are saved for the first time
    }

    JsonObject SettingsService::ClockToJson(const Models::ClockModel& clock) const
    {
        JsonObject obj;
        obj.SetNamedValue(L"timezoneId", JsonValue::CreateStringValue(clock.TimezoneId));
        obj.SetNamedValue(L"displayName", JsonValue::CreateStringValue(clock.DisplayName));
        obj.SetNamedValue(L"country", JsonValue::CreateStringValue(clock.Country));
        obj.SetNamedValue(L"displayMode", JsonValue::CreateNumberValue(static_cast<double>(clock.DisplayMode)));
        obj.SetNamedValue(L"analogStyle", JsonValue::CreateNumberValue(static_cast<double>(clock.AnalogVisualStyle)));
        obj.SetNamedValue(L"digitalStyle", JsonValue::CreateNumberValue(static_cast<double>(clock.DigitalVisualStyle)));
        return obj;
    }

    Models::ClockModel SettingsService::JsonToClock(const JsonObject& json) const
    {
        Models::ClockModel clock;
        clock.TimezoneId = json.GetNamedString(L"timezoneId", L"UTC");
        clock.DisplayName = json.GetNamedString(L"displayName", L"Unknown");
        clock.Country = json.GetNamedString(L"country", L"");
        clock.DisplayMode = static_cast<Models::ClockDisplayMode>(static_cast<int>(json.GetNamedNumber(L"displayMode", 0)));
        clock.AnalogVisualStyle = static_cast<Models::AnalogStyle>(static_cast<int>(json.GetNamedNumber(L"analogStyle", 1)));
        clock.DigitalVisualStyle = static_cast<Models::DigitalStyle>(static_cast<int>(json.GetNamedNumber(L"digitalStyle", 1)));
        return clock;
    }
}
