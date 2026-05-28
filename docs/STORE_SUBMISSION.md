# Microsoft Store Submission Guide

This is the path to a zero-warning installer for everyone. The Store
re-signs your package with a Microsoft-trusted certificate at submission
time, so end users get one-click "Get" with no SmartScreen or unsigned-app
prompts.

## One-time setup

### 1. Create a Partner Center account
- Go to <https://partner.microsoft.com/dashboard>.
- Register as an **Individual** ($19 one-time, no company verification) or
  **Company** ($99 one-time, requires D-U-N-S verification).
- Individual is fine for a side project; switch to Company later if you
  ever need verified publisher checkmarks or to publish under an org name.

### 2. Reserve the app name
- Partner Center → **Apps and games** → **New product** → **MSIX or PWA app**.
- Reserve the name **`Clocks`**. (If that's taken, try `Clocks Plus`,
  `Clocks World`, etc. — name must be globally unique.)
- After reservation, open the app → **Product identity**. You'll see
  three values you need to paste into `Clocks/Package.appxmanifest`:

  | Partner Center field              | Manifest target                            |
  |-----------------------------------|--------------------------------------------|
  | **Package/Identity/Name**         | `<Identity Name="...">`                    |
  | **Package/Identity/Publisher**    | `<Identity Publisher="CN=...">`            |
  | **Package/Properties/PublisherDisplayName** | `<PublisherDisplayName>...</PublisherDisplayName>` |

  Example values (yours will differ):
  ```xml
  <Identity
      Name="12345YourName.Clocks"
      Publisher="CN=ABCDE12345-1234-1234-1234-ABCDEF123456"
      Version="1.0.0.0" />
  ...
  <Properties>
    <DisplayName>Clocks</DisplayName>
    <PublisherDisplayName>Your Display Name</PublisherDisplayName>
    <Logo>Assets\StoreLogo.png</Logo>
  </Properties>
  ```

  > Today these values are placeholders — `Name="cd8328a5-..."` (a random
  > GUID from project creation) and `Publisher="CN=v-willyang"` (the dev
  > cert subject). The Store will refuse the upload until you paste in
  > the Partner Center values exactly. Don't guess — copy from
  > Partner Center.

### 3. Prepare the Store listing
While Partner Center has the name reserved, fill in the listing:
- **Description** (a paragraph or two — what Clocks does).
- **Short description** (one sentence).
- **Screenshots** (at least one, max 9). `docs/screenshots/clocks-grid.png`
  is a starting point; the Store wants ≥1366×768. You can use additional
  shots of compact mode, the meeting planner, and the time-diff page.
- **Store logo** (300×300) — reuse `Clocks/Assets/StoreLogo.scale-400.png`
  scaled or regenerate at exactly 300×300.
- **Category**: Productivity → Personal finance? No — pick **Productivity**.
- **Age rating**: fill the IARC questionnaire (Clocks is "Everyone").
- **Privacy policy URL** is required *only* if your app collects data.
  Clocks stores settings locally and makes no network calls, so you can
  declare no data collection — but you still need a privacy-policy URL.
  A 3-paragraph page on GitHub Pages or in the repo is fine.

## Each release

### 1. Bump the version
Edit `Clocks/Package.appxmanifest`:
```xml
<Identity ... Version="1.0.1.0" />
```
Version must be strictly greater than every prior submission. Use the
form `Major.Minor.Build.Revision` — keep the last digit `0` (Store
reserves it).

### 2. Build the .msixupload
From the repo root, in a Developer PowerShell:
```powershell
.\scripts\build-store-package.ps1
```
This produces:
```
AppPackages\Clocks_<version>_Test\
    Clocks_<version>_x86_x64_arm64_bundle.msixupload
```
The `.msixupload` is *unsigned* — that's correct for Store submission.
Do NOT sign it locally; the Store re-signs.

### 3. Upload to Partner Center
- Partner Center → your app → **Packages** → drag the `.msixupload`.
- Partner Center validates: identity, version is higher than last
  submission, no banned capabilities, etc.
- If you reused values from §1 correctly, validation passes in ~1 minute.

### 4. Submit
- Fill **Pricing and availability** (Free, Available in all markets is
  the default).
- **Submit to the Store**.
- Certification takes a few hours to a few days. You'll get email when
  it's live or if reviewers flag something.

## What end users see
- Search "Clocks" in the Microsoft Store → click **Get** → installs
  instantly with no warning.
- Auto-updates whenever you ship a new submission.
- Uninstall via Settings → Apps, same as any Store app.

## Troubleshooting

### "Package identity doesn't match Partner Center"
The `Name` / `Publisher` in your manifest must match Partner Center
character-for-character (case-sensitive on Name). Re-copy from
**Product identity** in Partner Center.

### "Package version has already been submitted"
Bump `Version` in the manifest. Partner Center keeps every version you
ever uploaded; you can never reuse one.

### "App fails Windows App Certification Kit (WACK)"
Run WACK locally before uploading:
```powershell
& "${env:ProgramFiles(x86)}\Windows Kits\10\App Certification Kit\appcertui.exe"
```
Select the produced `.msixupload`, run the Store certification suite.
Fix anything red before submitting.

### "I changed my mind about the app name"
You can rename in Partner Center (reserve a new name, abandon the old
one) **before** publishing. After publishing, rename is harder — the
package `Name`/`Publisher` are locked because changing them breaks
upgrades for installed users.

## Useful links
- Partner Center: <https://partner.microsoft.com/dashboard>
- MSIX packaging docs: <https://learn.microsoft.com/windows/msix/>
- Store policies: <https://learn.microsoft.com/windows/uwp/publish/store-policies>
- WACK: <https://learn.microsoft.com/windows/uwp/debug-test-perf/windows-app-certification-kit>
