# Clocks

A simple, modern **world clock app for Windows** — view multiple cities at a glance, compare time differences, and plan meetings across time zones.

Built with **WinUI 3** and **C++/WinRT** on the Windows App SDK.

![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-0078D6?logo=windows)
![Language](https://img.shields.io/badge/language-C%2B%2B%2FWinRT-blue)
![UI](https://img.shields.io/badge/UI-WinUI%203-9b4dca)

---

## Features

- 🌍 **World clocks** — add cities from around the globe and see analog + digital faces side by side.
- 🕒 **Time difference** — quickly compare any two time zones.
- 📅 **Meeting planner** — visualize overlapping business hours across multiple zones with minute-precise, DST-aware highlighting (half/quarter-hour offsets like India, Nepal, and Newfoundland are honored).
- 🪟 **Compact mode** — always-on-top, chromeless picture-in-picture overlay that stays out of your way.
- 💾 **Persistent settings** — your clock list, window size, and compact-mode position are remembered across sessions.
- 🎨 **System theme** — follows your Windows light/dark preference.

## Screenshots

_Coming soon._

## Requirements

- Windows 10 (1809+) or Windows 11
- [Visual Studio 2022 or newer](https://visualstudio.microsoft.com/) with:
  - **Desktop development with C++** workload
  - **Windows App SDK C++ Templates** component
  - Windows 10/11 SDK (10.0.19041 or later)
- [Windows App SDK](https://learn.microsoft.com/windows/apps/windows-app-sdk/) runtime (restored automatically via NuGet)

## Building

### From Visual Studio

1. Clone the repo:
   ```sh
   git clone https://github.com/mosbd/clocks.git
   ```
2. Open `Clocks.slnx` in Visual Studio.
3. Select the **Debug | x64** configuration (or Release).
4. Press **F5** to build and run.

### From the command line

```powershell
# Restore NuGet packages
nuget restore Clocks.slnx

# Build
& "C:\Program Files\Microsoft Visual Studio\<version>\<edition>\MSBuild\Current\Bin\MSBuild.exe" `
    Clocks\Clocks.vcxproj `
    /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

The resulting executable lives at `Clocks\x64\Debug\Clocks\Clocks.exe`.

## Project layout

```
Clocks.slnx                    Solution
Clocks/                        Main app project
  App.xaml(.h/.cpp/.idl)       Application entry
  MainWindow.xaml(...)         Window shell, navigation, compact mode
  ClocksPage.xaml(...)         World clocks grid
  MeetingPlannerPage.xaml(...) Cross-zone meeting planner timeline
  TimeDiffPage.xaml(...)       Pairwise time difference
  ClockCard.xaml(...)          Reusable analog + digital clock card
  TimeZoneService.{h,cpp}      IANA tz lookup, per-instant offsets (DST-aware)
  SettingsService.{h,cpp}      JSON-backed persistence with atomic writes
  Assets/
    generate_icons.py          Pillow script that regenerates the icon set
BRD.md                         Business Requirements Document
```

## Regenerating app icons

The full icon set (Square44x44, Square150x150, Wide310, StoreLogo, splash, etc.) is generated from a single design in `Clocks/Assets/generate_icons.py`:

```powershell
cd Clocks\Assets
pip install pillow
python generate_icons.py
```

## Status

Phase 1 (MVVM/services/navigation foundation) is complete. Phase 2 (ClockCard control, refined pages, compact mode polish) is in progress.

See [`BRD.md`](BRD.md) for the full business requirements and feature scope.

## License

TBD.

## Acknowledgments

- Built with the [Windows App SDK](https://github.com/microsoft/WindowsAppSDK) and [WinUI 3](https://github.com/microsoft/microsoft-ui-xaml).
- Time zone data via the C++20 `<chrono>` IANA tz database.
