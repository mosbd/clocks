#pragma once
#include "pch.h"
#include "ClockModel.h"
#include <mutex>

namespace winrt::Clocks::Services
{
    class SettingsService
    {
    public:
        SettingsService();

        std::vector<Models::ClockModel> LoadClocks() const;
        void SaveClocks(const std::vector<Models::ClockModel>& clocks) const;

        struct CompactRect { int x, y, width, height; bool valid; };
        CompactRect LoadCompactRect() const;
        void SaveCompactRect(int x, int y, int width, int height) const;

    private:
        winrt::hstring GetSettingsFilePath() const;
        winrt::Windows::Data::Json::JsonObject ClockToJson(const Models::ClockModel& clock) const;
        Models::ClockModel JsonToClock(const winrt::Windows::Data::Json::JsonObject& json) const;

        // Read-merge-write helpers; callers must hold m_mutex.
        winrt::Windows::Data::Json::JsonObject ReadRoot() const;
        void WriteRoot(winrt::Windows::Data::Json::JsonObject const& root) const;

        // Serializes all settings I/O so concurrent SaveClocks / SaveCompactRect
        // calls don't clobber unrelated fields or interleave a partial write.
        mutable std::mutex m_mutex;
    };
}
