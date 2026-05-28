# Privacy Policy for World Clock for Windows

**Last updated:** May 28, 2026
**App:** World Clock for Windows (the "App")
**Publisher:** mosbd
**Contact:** https://github.com/mosbd/clocks/issues

This policy explains how the App handles information. The short version
is: **the App does not collect, transmit, or share any personal data.**
Every piece of information it works with stays on your device.

## Information the App handles

The App stores the following on your local device only, inside the per-user
application data folder Windows allocates to it
(`%LOCALAPPDATA%\Packages\mosbd.WorldClockforWindows_*\LocalState\`):

- The list of cities / time zones you've added.
- Your display preferences (analog vs. digital clock style, compact mode,
  window size and position, meeting-planner column choices, etc.).

This data is written to a single JSON file (`clocks_settings.json`) inside
that folder. It contains no personal identifiers, no contact information,
no location data, and no usage history.

## Information the App does NOT collect

- **No personal identifiers.** The App does not ask for your name, email,
  phone number, address, account credentials, or any other identifying
  information.
- **No location.** The App does not request or use the Windows location
  service. Time-zone offsets are computed from the IANA time zone you
  select, never from your physical location.
- **No telemetry or analytics.** The App does not contain any analytics,
  crash-reporting, advertising, or tracking SDKs. It does not measure how
  you use the App and reports nothing back to the publisher.
- **No network access.** The App makes no outbound network requests of
  any kind. You can confirm this by inspecting it with Windows Resource
  Monitor or any firewall — the App never opens a socket.
- **No microphone, camera, contacts, files, or other sensors.**
- **No cookies or web tracking** — the App has no web component.

## Information shared with third parties

**None.** Because the App does not collect any data and does not connect
to the network, there is nothing to share with anyone — including the
publisher, advertisers, analytics providers, or any other third party.

## Information shared with Microsoft

The App is distributed through the Microsoft Store. Microsoft may collect
information related to your acquisition, installation, and updates of the
App (for example, store account, device type, install/uninstall events,
and crash signatures from Windows Error Reporting). That collection is
governed by the [Microsoft Privacy
Statement](https://www.microsoft.com/en-us/privacy/privacystatement), not
by this policy. The App publisher (mosbd) does not receive personally
identifying information from these channels.

## Data retention and deletion

The App's local settings file persists for as long as the App is installed.

To delete all data the App has stored:

- **From the App:** open Settings → "Reset to defaults" (clears the
  settings file).
- **From Windows:** Settings → Apps → Installed apps → *World Clock for
  Windows* → **Uninstall**. Windows removes the App's `LocalState` folder
  along with the App. After uninstall, no data remains on your device.

Because nothing is transmitted off the device, there is no remote copy
to request deletion of.

## Children's privacy

The App is suitable for general audiences and does not knowingly collect
data from anyone, including children under 13 (or the equivalent minimum
age in your jurisdiction).

## Your rights (GDPR, CCPA, and similar laws)

Privacy laws such as the EU General Data Protection Regulation (GDPR)
and the California Consumer Privacy Act (CCPA) give you rights to access,
correct, export, and delete personal data that a service holds about you.

The App and its publisher hold **no personal data** about you, so there
is nothing to access, correct, export, or delete on our side. You can
exercise full control over the App's local settings file at any time by
editing or deleting it directly (see "Data retention and deletion").

## Open source

The App's source code is published at
<https://github.com/mosbd/clocks>. You can inspect it to verify the
statements in this policy.

## Changes to this policy

If a future version of the App adds a feature that handles data
differently — for example, optional cloud sync of your clock list — this
policy will be updated **before** that feature ships, and the
"Last updated" date at the top of this document will change. The current
version is always available at:

<https://github.com/mosbd/clocks/blob/master/docs/PRIVACY.md>

## Contact

Questions or concerns about this policy: please open an issue at
<https://github.com/mosbd/clocks/issues>.
