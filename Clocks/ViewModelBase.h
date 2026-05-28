#pragma once
#include "pch.h"

namespace winrt::Clocks::implementation
{
    // Macro to reduce INotifyPropertyChanged boilerplate per-property
    #define OBSERVABLE_PROPERTY(type, name, field) \
        type name() const { return field; } \
        void name(type const& value) { \
            if (field != value) { \
                field = value; \
                RaisePropertyChanged(L## #name); \
            } \
        }

    #define OBSERVABLE_PROPERTY_HSTRING(name, field) \
        winrt::hstring name() const { return field; } \
        void name(winrt::hstring const& value) { \
            if (field != value) { \
                field = value; \
                RaisePropertyChanged(L## #name); \
            } \
        }

    template<typename D>
    struct ViewModelBase
    {
        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
        {
            return m_propertyChanged.add(handler);
        }

        void PropertyChanged(winrt::event_token const& token) noexcept
        {
            m_propertyChanged.remove(token);
        }

    protected:
        void RaisePropertyChanged(std::wstring_view propertyName)
        {
            m_propertyChanged(static_cast<D&>(*this), Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ propertyName });
        }

    private:
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
