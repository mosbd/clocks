<#
.SYNOPSIS
    Build a Microsoft Store-ready .msixupload bundle for Clocks.

.DESCRIPTION
    Produces a single .msixupload file containing x86/x64/ARM64 .msix packages,
    ready to upload to Partner Center. The Store re-signs everything with a
    Microsoft-trusted certificate on submission, so no local signing cert is
    needed — and end users install with zero certificate warnings.

    Output: AppPackages\Clocks_<version>_Test\Clocks_<version>_x86_x64_arm64_bundle.msixupload

.PARAMETER MSBuildPath
    Full path to MSBuild.exe. Defaults to the VS 2026/18 Community install.

.PARAMETER Platforms
    Architectures to include in the bundle. Default: x86, x64, ARM64
    (Store requires at least one; all three is the recommended set).

.EXAMPLE
    .\scripts\build-store-package.ps1
#>
[CmdletBinding()]
param(
    [string] $MSBuildPath = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
    [string[]] $Platforms = @('x86', 'x64', 'ARM64')
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$project  = Join-Path $repoRoot 'Clocks\Clocks.vcxproj'
$outDir   = Join-Path $repoRoot 'AppPackages'

if (-not (Test-Path $MSBuildPath)) {
    throw "MSBuild not found at '$MSBuildPath'. Pass -MSBuildPath to point at your VS install."
}
if (-not (Test-Path $project)) {
    throw "Project not found at '$project'."
}

$bundlePlatforms = ($Platforms -join '|')

Write-Host "Building Store package for: $bundlePlatforms" -ForegroundColor Cyan
Write-Host "Output dir: $outDir" -ForegroundColor Cyan
Write-Host ""

# Notes on the properties:
#   UapAppxPackageBuildMode=StoreUpload  -> emit a .msixupload (the format
#                                           Partner Center accepts)
#   AppxBundle=Always                    -> always produce a bundle, even for
#                                           a single arch (Store requires it)
#   AppxBundlePlatforms                  -> which arches go in the bundle
#   AppxPackageSigningEnabled=false     -> do not try to sign locally; the
#                                          Store will re-sign on submission
#   GenerateAppxPackageOnBuild=true     -> emit the .msix at build time
#
# We use /t:Rebuild rather than /t:Build because the AppX packaging targets
# have a known issue where they reuse cached per-arch .msix files from
# Clocks\<arch>\Release\Clocks\Upload\ if they exist, even when the source
# Package.appxmanifest has changed. That cache made the first Store upload
# fail with stale PublisherDisplayName values. Rebuild deletes the cache
# every run, which is slow but correct.
& $MSBuildPath $project `
    /t:Rebuild `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /p:AppxBundlePlatforms="$bundlePlatforms" `
    /p:UapAppxPackageBuildMode=StoreUpload `
    /p:AppxBundle=Always `
    /p:AppxPackageSigningEnabled=false `
    /p:GenerateAppxPackageOnBuild=true `
    /p:AppxPackageDir="$outDir\\" `
    /m /nologo /verbosity:minimal

if ($LASTEXITCODE -ne 0) {
    throw "Build failed (exit $LASTEXITCODE)."
}

$upload = Get-ChildItem -Path $outDir -Recurse -Filter '*.msixupload' |
          Sort-Object LastWriteTime -Descending |
          Select-Object -First 1

if (-not $upload) {
    throw "Build succeeded but no .msixupload was produced. Check $outDir."
}

Write-Host ""
Write-Host "Success." -ForegroundColor Green
Write-Host "Upload this file to Partner Center -> Packages:" -ForegroundColor Green
Write-Host "  $($upload.FullName)" -ForegroundColor Yellow
Write-Host ""
Write-Host "Size: $([math]::Round($upload.Length / 1MB, 2)) MB"
