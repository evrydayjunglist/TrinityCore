param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$Configuration = "RelWithDebInfo",
    [string]$Scripts = "static",
    [string]$Modules = "none",
    [string[]]$Targets = @("worldserver", "bnetserver"),
    [switch]$SkipConfigure
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "trinitycore-build-common.ps1")

$SourceDir = Get-TrinityCoreSourceDir $SourceDir
$BuildDir = Get-TrinityCoreBuildDir $SourceDir $BuildDir

if (-not $SkipConfigure) {
    Invoke-TrinityCoreCmakeConfigure -SourceDir $SourceDir -BuildDir $BuildDir -Scripts $Scripts -Modules $Modules
}
else {
    Write-Host "Skipping CMake configure (-SkipConfigure). Use only when no new source files or CMake changes." -ForegroundColor Yellow
}

cmake --build $BuildDir --config $Configuration --target $Targets
