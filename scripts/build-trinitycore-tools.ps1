param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$Configuration = "RelWithDebInfo",
    [string]$Scripts = "static",
    [string]$Modules = "none",
    [string[]]$Targets = @("mapextractor", "vmap4extractor", "vmap4assembler", "mmaps_generator")
)

$ErrorActionPreference = "Stop"

if (-not $SourceDir) {
    $SourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path -replace '\\', '/'
}

if (-not $BuildDir) {
    $repoName = Split-Path -Leaf $SourceDir
    $BuildDir = "D:/WOWEmulation/Emulators/Builds/$repoName"
}

# Uses the same configure flags and BuildDir as build-trinitycore-master.ps1 so both
# scripts share one CMake tree. Outputs: $BuildDir/bin/$Configuration/*.exe

cmake -S $SourceDir -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
    "-DSERVERS=1" `
    "-DTOOLS=1" `
    "-DSCRIPTS=$Scripts" `
    "-DMODULES=$Modules" `
    "-DMYSQL_INCLUDE_DIR=C:/Program Files/MySQL/MySQL Server 8.4/include" `
    "-DMYSQL_LIBRARY=C:/Program Files/MySQL/MySQL Server 8.4/lib/libmysql.lib" `
    "-DOPENSSL_ROOT_DIR=C:/Program Files/OpenSSL-Win64"

cmake --build $BuildDir --config $Configuration --target $Targets
