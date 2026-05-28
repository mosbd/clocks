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
        std::lock_guard guard(m_mutex);
        try
        {
            auto root = ReadRoot();
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
            // Return what we have on any parse error.
        }
        return clocks;
    }

    void SettingsService::SaveClocks(const std::vector<Models::ClockModel>& clocks) const
    {
        std::lock_guard guard(m_mutex);
        try
        {
            // Preserve every other field (e.g. compactRect) — never overwrite the
            // file with just our slice.
            auto root = ReadRoot();
            JsonArray arr;
            for (const auto& clock : clocks)
            {
                arr.Append(ClockToJson(clock));
            }
            root.SetNamedValue(L"clocks", arr);
            root.SetNamedValue(L"firstLaunchComplete", JsonValue::CreateBooleanValue(true));
            WriteRoot(root);
        }
        catch (...)
        {
            // Silently fail
        }
    }

    SettingsService::CompactRect SettingsService::LoadCompactRect() const
    {
        CompactRect rect{ 0, 0, 0, 0, false };
        std::lock_guard guard(m_mutex);
        try
        {
            auto root = ReadRoot();
            if (root.HasKey(L"compactRect"))
            {
                auto obj = root.GetNamedObject(L"compactRect");
                rect.x      = static_cast<int>(obj.GetNamedNumber(L"x", 0));
                rect.y      = static_cast<int>(obj.GetNamedNumber(L"y", 0));
                rect.width  = static_cast<int>(obj.GetNamedNumber(L"width", 0));
                rect.height = static_cast<int>(obj.GetNamedNumber(L"height", 0));
                rect.valid  = rect.width > 0 && rect.height > 0;
            }
        }
        catch (...) {}
        return rect;
    }

    void SettingsService::SaveCompactRect(int x, int y, int width, int height) const
    {
        std::lock_guard guard(m_mutex);
        try
        {
            auto root = ReadRoot();
            JsonObject obj;
            obj.SetNamedValue(L"x",      JsonValue::CreateNumberValue(x));
            obj.SetNamedValue(L"y",      JsonValue::CreateNumberValue(y));
            obj.SetNamedValue(L"width",  JsonValue::CreateNumberValue(width));
            obj.SetNamedValue(L"height", JsonValue::CreateNumberValue(height));
            root.SetNamedValue(L"compactRect", obj);
            WriteRoot(root);
        }
        catch (...) {}
    }

    JsonObject SettingsService::ReadRoot() const
    {
        JsonObject root;
        try
        {
            std::wstring path(GetSettingsFilePath());
            std::wifstream file(path);
            if (!file.is_open()) return root;

            std::wstringstream buffer;
            buffer << file.rdbuf();
            std::wstring content = buffer.str();
            if (content.empty()) return root;

            JsonObject parsed;
            if (JsonObject::TryParse(winrt::hstring(content), parsed))
                return parsed;
        }
        catch (...) {}
        return root;
    }

    void SettingsService::WriteRoot(JsonObject const& root) const
    {
        try
        {
            std::wstring path(GetSettingsFilePath());
            std::wstring tmp = path + L".tmp";
            {
                std::wofstream out(tmp);
                if (!out.is_open()) return;
                out << std::wstring_view(root.Stringify());
                if (!out.good()) return;
            }
            // Atomic replace: avoids leaving a half-written settings file on crash.
            ::MoveFileExW(tmp.c_str(), path.c_str(),
                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        }
        catch (...) {}
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
