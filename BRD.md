# Business Requirements Document (BRD)

## Clocks — World Clock Desktop Application

| Field | Value |
|-------|-------|
| **Document Version** | 1.0 |
| **Product Name** | Clocks |
| **Platform** | Windows Desktop (WinUI 3) |
| **Status** | In Development |

---

## 1. Executive Summary

Clocks is a Windows desktop application that displays multiple world clocks simultaneously. Users can view the current time across different cities and time zones in a customizable grid layout, with both analog and digital clock faces offering multiple visual styles. The application also provides tools for comparing time differences between cities and planning meetings across time zones.

## 2. Business Objectives

| # | Objective |
|---|-----------|
| BO-1 | Provide users with an at-a-glance view of current time across multiple global cities |
| BO-2 | Enable quick time zone comparisons for international collaboration |
| BO-3 | Deliver a visually appealing, modern Windows desktop experience that follows platform conventions |
| BO-4 | Support always-on-top display so clocks remain accessible during other work |

## 3. Scope

### 3.1 In Scope

- World clock display with customizable city selection
- Analog and digital clock rendering with multiple visual themes
- Time difference calculator between selected cities
- Meeting planner showing overlapping business hours across time zones
- Compact overlay (always-on-top / picture-in-picture) mode
- Persistence of user preferences across sessions
- System light/dark theme support

### 3.2 Out of Scope

- Alarms, timers, or stopwatch functionality
- Mobile or web versions
- Cloud sync of settings across devices
- Calendar integration
- Notifications or alerts
- Daylight Saving Time change notifications (DST is handled silently)

## 4. Stakeholders

| Role | Responsibility |
|------|---------------|
| End User | Uses the application to track time across cities |
| Developer | Designs, builds, and maintains the application |

## 5. Functional Requirements

### FR-1: World Clock Grid Display

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-1.1 | The application SHALL display multiple clocks in a responsive grid/card layout | Must |
| FR-1.2 | Each clock card SHALL show: city name, current time, current date, UTC offset, and time difference from local | Must |
| FR-1.3 | Clocks SHALL update every second (tick animation) | Must |
| FR-1.4 | The grid SHALL adapt its column count based on window width | Must |
| FR-1.5 | There SHALL be no hard limit on the number of clocks displayed | Should |
| FR-1.6 | On first launch, the application SHALL display the user's local time zone | Must |

### FR-2: Clock Visual Styles

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-2.1 | Each clock SHALL support two display modes: Analog and Digital | Must |
| FR-2.2 | Users SHALL be able to switch display mode per clock independently | Must |
| FR-2.3 | Analog mode SHALL offer three visual styles: Classic, Modern, Minimal | Must |
| FR-2.4 | Digital mode SHALL offer three visual styles: LED, Clean, Retro | Must |
| FR-2.5 | Users SHALL be able to change visual style per clock via a context menu | Must |
| FR-2.6 | Smooth-sweep second hand MAY be offered as an optional enhancement | Could |

#### FR-2.3.1: Analog Style Definitions

| Style | Description |
|-------|-------------|
| **Classic** | Traditional round face, Roman numerals (XII, III, VI, IX), ornate gold-toned hands, red second hand, dark face with gold bezel |
| **Modern** | Clean face (theme-aware), Arabic numerals (12, 3, 6, 9), sleek tapered hands, accent-colored second hand, thin bezel |
| **Minimal** | Transparent/borderless face, no numerals, thin tick marks at 12/3/6/9 only, ultra-thin line hands, subtle second hand |

#### FR-2.4.1: Digital Style Definitions

| Style | Description |
|-------|-------------|
| **LED** | Seven-segment display aesthetic (Consolas font), green-on-dark color scheme, full time with seconds, 12-hour format |
| **Clean** | Modern sans-serif (Segoe UI), theme-default colors, 12-hour format with small seconds below, AM/PM indicator |
| **Retro** | Monospaced font (Cascadia Mono), green text on dark background, 24-hour format, dot separators (08.45.23) |

### FR-3: City / Time Zone Management

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-3.1 | Users SHALL be able to add new cities/time zones via a search dialog | Must |
| FR-3.2 | The search SHALL support autocomplete filtering from the IANA timezone database (~600 zones) | Must |
| FR-3.3 | Search SHALL match against city name, timezone ID, and country name | Should |
| FR-3.4 | Users SHALL be able to remove any clock from the grid | Must |
| FR-3.5 | Removing the last clock SHOULD show a confirmation prompt | Should |

### FR-4: Persistence

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-4.1 | The application SHALL persist the user's selected clocks across app restarts | Must |
| FR-4.2 | The application SHALL persist each clock's display mode and visual style | Must |
| FR-4.3 | Settings SHALL be stored locally (no cloud dependency) | Must |
| FR-4.4 | Corrupted or missing settings SHALL result in graceful fallback to defaults | Must |

### FR-5: Navigation

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-5.1 | The application SHALL use a NavigationView with three sections: Clocks, Time Difference, Meeting Planner | Must |
| FR-5.2 | The Clocks page SHALL be the default/home page | Must |
| FR-5.3 | Navigation SHALL preserve page state when switching between sections | Should |

### FR-6: Time Difference Calculator

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-6.1 | Users SHALL be able to select two or more cities and view the time difference between them | Must |
| FR-6.2 | Differences SHALL be displayed in hours and minutes with directional labels (e.g., "New York is 13 hours behind Tokyo") | Must |
| FR-6.3 | Users SHALL be able to select cities from their clock grid or from the full timezone database | Should |

### FR-7: Meeting Planner

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-7.1 | Users SHALL be able to select multiple time zones to plan a meeting | Must |
| FR-7.2 | The planner SHALL display a visual horizontal timeline per timezone | Must |
| FR-7.3 | Each timeline bar SHALL indicate business hours (default: 9 AM – 5 PM) | Must |
| FR-7.4 | The planner SHALL highlight the overlapping time window where all selected zones have business hours | Must |
| FR-7.5 | A current-time marker SHALL be shown on the timeline | Should |
| FR-7.6 | Business hours SHOULD be configurable per timezone | Could |

### FR-8: Compact Overlay / Always-on-Top

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-8.1 | Users SHALL be able to toggle compact overlay (picture-in-picture) mode | Must |
| FR-8.2 | The compact view SHALL use a dedicated minimal layout within OS-enforced bounds (150×150 – 500×500 px) | Must |
| FR-8.3 | Users SHALL be able to select which clock(s) appear in compact mode (1–4 clocks) | Should |
| FR-8.4 | The compact view SHALL remain always-on-top of other windows | Must |

## 6. Non-Functional Requirements

### NFR-1: Performance

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-1.1 | Application startup time | < 2 seconds to interactive |
| NFR-1.2 | Clock update latency | < 50ms per tick across all visible clocks |
| NFR-1.3 | Memory usage (10 clocks) | < 100 MB |
| NFR-1.4 | GPU usage at idle (ticking mode) | < 5% |

### NFR-2: Compatibility

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-2.1 | Minimum OS version | Windows 10 version 1903 (10.0.18362) |
| NFR-2.2 | Supported architectures | x86, x64, ARM64 |
| NFR-2.3 | Packaging format | MSIX |

### NFR-3: Usability

| ID | Requirement |
|----|-------------|
| NFR-3.1 | The application SHALL follow the Windows 11 design language (WinUI 3 controls, rounded corners, Mica/Acrylic) |
| NFR-3.2 | The application SHALL automatically adapt to the system light/dark theme |
| NFR-3.3 | All clock visuals SHALL adapt their color palette to the active theme |
| NFR-3.4 | The application SHALL be usable with keyboard navigation |

### NFR-4: Reliability

| ID | Requirement |
|----|-------------|
| NFR-4.1 | Invalid or unrecognized timezone IDs SHALL be handled gracefully (display fallback text, do not crash) |
| NFR-4.2 | DST transitions SHALL be handled automatically via the IANA timezone database |
| NFR-4.3 | Corrupted settings files SHALL not prevent application launch |

## 7. Technical Architecture

### 7.1 Technology Stack

| Component | Technology |
|-----------|-----------|
| Language | C++20 / C++/WinRT |
| UI Framework | WinUI 3 (Windows App SDK 2.1.3) |
| Clock Rendering | XAML Shapes + RotateTransform (compositor-accelerated) |
| Packaging | MSIX |
| Timezone Data | C++20 `std::chrono` timezone support (IANA tz database via ICU) |
| Persistence | Windows.Data.Json → ApplicationData.LocalFolder |
| Pattern | MVVM (Model-View-ViewModel) |

### 7.2 Architecture Overview

```
┌─────────────────────────────────────────────┐
│                  MainWindow                  │
│  ┌─────────────────────────────────────────┐│
│  │           NavigationView                ││
│  │  ┌──────────┬──────────┬──────────────┐ ││
│  │  │ Clocks   │ Time Diff│ Meeting Plan │ ││
│  │  │ Page     │ Page     │ Page         │ ││
│  │  └──────────┴──────────┴──────────────┘ ││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
		 │              │              │
	┌────┴────┐    ┌────┴────┐   ┌────┴────┐
	│ClockCard│    │TimeDiff │   │Meeting  │
	│(×N)     │    │ViewModel│   │Planner  │
	└────┬────┘    └────┬────┘   │ViewModel│
		 │              │        └────┬────┘
	┌────┴──────────────┴─────────────┴────┐
	│           Services Layer             │
	│  TimeZoneService  │  SettingsService │
	└──────────────────────────────────────┘
```

### 7.3 Key Components

| Component | Responsibility |
|-----------|---------------|
| `MainWindow` | NavigationView shell, hosts page navigation |
| `ClocksPage` | Grid of ClockCard controls, DispatcherTimer, add/remove logic |
| `ClockCard` | Single clock display (analog or digital), style management, time rendering |
| `TimeDiffPage` | City selection, time difference computation and display |
| `MeetingPlannerPage` | Multi-timezone timeline visualization |
| `CompactClockView` | Minimal clock layout for compact overlay mode |
| `TimeZoneService` | IANA timezone enumeration, time queries, difference calculations |
| `SettingsService` | JSON-based persistence to local app data |
| `ViewModelBase` | CRTP base class with INotifyPropertyChanged + helper macros |

## 8. User Interaction Flows

### 8.1 First Launch
1. Application opens to Clocks page
2. Local timezone clock is automatically added
3. Grid shows one clock card with local time

### 8.2 Adding a Clock
1. User clicks "Add Clock" button
2. Search dialog appears with AutoSuggestBox
3. User types a city name → autocomplete shows matches
4. User selects a city → clock card appears in the grid
5. Selection is persisted automatically

### 8.3 Customizing a Clock
1. User clicks the ⋯ menu on a clock card
2. Menu shows: Switch mode (analog ↔ digital), style options, Remove
3. User selects a style → clock updates immediately
4. Preference is persisted automatically

### 8.4 Using Time Difference Calculator
1. User navigates to "Time Difference" page
2. Selects two or more cities from dropdown/search
3. Application displays pairwise time differences with labels

### 8.5 Using Meeting Planner
1. User navigates to "Meeting Planner" page
2. Adds multiple time zones
3. Visual timeline renders showing each zone's business hours
4. Overlap region is highlighted
5. Current time marker indicates "now"

### 8.6 Enabling Compact Overlay
1. User activates compact mode (button/menu)
2. Window shrinks to compact size (always-on-top)
3. Displays selected clock(s) in minimal layout
4. User can toggle back to full mode

## 9. Acceptance Criteria

| # | Criterion |
|---|-----------|
| AC-1 | Application launches and displays at least one clock within 2 seconds |
| AC-2 | Users can add 20+ clocks and all tick in sync without visible lag |
| AC-3 | Closing and reopening the app restores all previously configured clocks with their styles |
| AC-4 | All three analog styles are visually distinct and render correctly |
| AC-5 | All three digital styles are visually distinct and render correctly |
| AC-6 | Switching between analog/digital and style changes is instantaneous |
| AC-7 | City search returns relevant results within 100ms of typing |
| AC-8 | Time difference calculator shows correct differences accounting for DST |
| AC-9 | Meeting planner correctly highlights overlapping business hours |
| AC-10 | Compact overlay stays on top and displays selected clocks |
| AC-11 | Application respects system light/dark theme changes |

## 10. Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| `std::chrono` timezone database unavailable on some Windows builds | Clock times show errors | Low | Fallback to hardcoded city list with manual offset data |
| Many clocks degrade rendering performance | UI stutters | Medium | Tick (1fps) by default; virtualize off-screen cards |
| Compact overlay API limitations on older Windows 10 | Feature unavailable | Low | Gracefully disable compact mode; show informational message |
| IANA tz database out of date in OS | Incorrect offsets after political tz changes | Low | Rely on Windows Update to keep ICU data current |

## 11. Implementation Phases

| Phase | Scope | Status |
|-------|-------|--------|
| **Phase 1: Foundation** | Project setup, MVVM scaffolding, TimeZoneService, SettingsService, NavigationView shell | ✅ Complete |
| **Phase 2: Core Clock UI** | Analog rendering (3 styles), digital rendering (3 styles), ClockCard control, ClocksPage grid with timer | 🔄 In Progress |
| **Phase 3: City Management** | AddCityDialog with autocomplete, remove clock, first-launch behavior | ⏳ Pending |
| **Phase 4: Additional Features** | Time difference calculator, meeting planner, compact overlay | ⏳ Pending |
| **Phase 5: Polish** | Theme adaptation, animations, performance optimization, edge cases | ⏳ Pending |

---

*End of document.*
