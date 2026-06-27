function Get-TrinityCoreSourceDir {
    param([string]$SourceDir)

    if (-not $SourceDir) {
        $SourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path -replace '\\', '/'
    }

    return $SourceDir
}

function Get-TrinityCoreBuildDir {
    param(
        [string]$SourceDir,
        [string]$BuildDir
    )

    if (-not $BuildDir) {
        $repoName = Split-Path -Leaf $SourceDir
        $BuildDir = "D:/WOWEmulation/Emulators/Builds/$repoName"
    }

    return $BuildDir
}

function Invoke-TrinityCoreCmakeConfigure {
    param(
        [Parameter(Mandatory)]
        [string]$SourceDir,

        [Parameter(Mandatory)]
        [string]$BuildDir,

        [string]$Scripts = "static",
        [string]$Modules = "none"
    )

    # Always reconfigure before building. CMake only discovers new .cpp files (e.g. after
    # upstream merges) when configure runs; incremental MSBuild alone can leave the scripts
    # target missing new sources and cause LNK2019 linker errors.
    Write-Host "CMake configure: $SourceDir -> $BuildDir (SCRIPTS=$Scripts, MODULES=$Modules)" -ForegroundColor Cyan

    cmake -S $SourceDir -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
        "-DSERVERS=1" `
        "-DTOOLS=1" `
        "-DSCRIPTS=$Scripts" `
        "-DMODULES=$Modules" `
        "-DMYSQL_INCLUDE_DIR=C:/Program Files/MySQL/MySQL Server 8.4/include" `
        "-DMYSQL_LIBRARY=C:/Program Files/MySQL/MySQL Server 8.4/lib/libmysql.lib" `
        "-DOPENSSL_ROOT_DIR=C:/Program Files/OpenSSL-Win64"
}
