param(
    [ValidateSet("Prepare", "StartCapture", "Parse", "Status")]
    [string]$Action = "Status",
    [string]$SniffRoot = "C:/Users/jay/Desktop/hello",
    [string]$YmirExe = "C:/Users/jay/Desktop/hello/ffins.exe",
    [string]$WowPacketParserExe = "C:/Users/jay/Desktop/hello/WowPacketParser.exe",
    [string]$SniffClientRoot = "F:/World of Warcraft/_retail_",
    [string]$PktFile,
    [switch]$CopyParsedToTemp
)

$ErrorActionPreference = "Stop"

function Get-RetailSniffPaths {
    @{
        SniffRoot          = $SniffRoot
        YmirExe            = $YmirExe
        WowPacketParserExe = $WowPacketParserExe
        SniffClientRoot    = $SniffClientRoot
        WowExe             = Join-Path $SniffClientRoot "wow.exe"
        CacheDir           = Join-Path $SniffClientRoot "Cache"
        WdbDir             = Join-Path $SniffClientRoot "WDB"
        DumpsDir           = Join-Path $SniffRoot "dumps"
        TempSniffDir       = Join-Path (Split-Path $PSScriptRoot -Parent) "temp/retail-sniff"
    }
}

function Test-RetailSniffToolchain {
    param($Paths)

    $missing = @()
    foreach ($key in @("YmirExe", "WowPacketParserExe", "WowExe", "SniffRoot")) {
        if (-not (Test-Path $Paths[$key])) {
            $missing += "$key → $($Paths[$key])"
        }
    }
    if ($missing.Count -gt 0) {
        throw "Retail sniff toolchain missing:`n  $($missing -join "`n  ")"
    }
}

function Clear-RetailClientCache {
    param($Paths)

    $cleared = @()
    foreach ($dirKey in @("CacheDir", "WdbDir")) {
        $dir = $Paths[$dirKey]
        if (Test-Path $dir) {
            Remove-Item -LiteralPath $dir -Recurse -Force
            $cleared += $dir
        }
    }
    return $cleared
}

function Get-LatestPktFile {
    param($Paths)

    $searchRoots = @($Paths.SniffRoot, $Paths.DumpsDir) | Where-Object { Test-Path $_ }
    $pkts = foreach ($root in $searchRoots) {
        Get-ChildItem -LiteralPath $root -Filter "*.pkt" -File -ErrorAction SilentlyContinue
    }
    if (-not $pkts) {
        return $null
    }
    return $pkts | Sort-Object LastWriteTime -Descending | Select-Object -First 1
}

function Start-YmirCapture {
    param($Paths)

    $existing = Get-Process -Name "ffins" -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Host "Ymir (ffins) already running (PID $($existing.Id))." -ForegroundColor Yellow
        return $existing
    }

    $proc = Start-Process -FilePath $Paths.YmirExe `
        -WorkingDirectory $Paths.SniffRoot `
        -PassThru
    Write-Host "Started Ymir (ffins) PID $($proc.Id) from $($Paths.SniffRoot)" -ForegroundColor Green
    return $proc
}

function Invoke-WowPacketParser {
    param($Paths, [string]$InputPkt, [switch]$CopyToTemp)

    if (-not (Test-Path $InputPkt)) {
        throw "Packet file not found: $InputPkt"
    }

    Push-Location $Paths.SniffRoot
    try {
        & $Paths.WowPacketParserExe $InputPkt
        if ($LASTEXITCODE -ne 0) {
            throw "WowPacketParser exited with code $LASTEXITCode"
        }
    }
    finally {
        Pop-Location
    }

    $dir = [System.IO.Path]::GetDirectoryName($InputPkt)
    $base = [System.IO.Path]::GetFileNameWithoutExtension($InputPkt)
    $parsedPath = Join-Path $dir "${base}_parsed.txt"
    if (-not (Test-Path $parsedPath)) {
        throw "Expected parsed output not found: $parsedPath"
    }

    if ($CopyToTemp) {
        New-Item -ItemType Directory -Force -Path $Paths.TempSniffDir | Out-Null
        $dest = Join-Path $Paths.TempSniffDir ([System.IO.Path]::GetFileName($parsedPath))
        Copy-Item -LiteralPath $parsedPath -Destination $dest -Force
        Write-Host "Copied parsed dump → $dest" -ForegroundColor Cyan
    }

    return @{
        PktFile    = $InputPkt
        ParsedFile = $parsedPath
    }
}

$paths = Get-RetailSniffPaths

switch ($Action) {
    "Prepare" {
        Test-RetailSniffToolchain $paths
        $cleared = Clear-RetailClientCache $paths
        if ($cleared.Count -eq 0) {
            Write-Host "No Cache/WDB folders present under $($paths.SniffClientRoot) (nothing to clear)." -ForegroundColor Yellow
        }
        else {
            Write-Host "Cleared:" -ForegroundColor Green
            $cleared | ForEach-Object { Write-Host "  $_" }
        }
        Write-Host ""
        Write-Host "Next: agent runs -Action StartCapture, then owner logs into LIVE RETAIL (not 127.0.0.1)." -ForegroundColor Cyan
    }

    "StartCapture" {
        Test-RetailSniffToolchain $paths
        $proc = Start-YmirCapture $paths
        Write-Host ""
        Write-Host "OWNER STEPS:" -ForegroundColor Magenta
        Write-Host '  1. Launch live retail WoW from F: install' $paths.WowExe '- NOT the D: emulator copy / NOT portal 127.0.0.1.'
        Write-Host '  2. Perform the scenario (login, UI action, etc.).'
        Write-Host '  3. Exit WoW; stop Ymir (close ffins window or End-Process ffins).'
        Write-Host '  4. Tell the agent capture is done.'
        Write-Host ""
        Write-Host 'Agent next: -Action Parse (or -Action Parse -PktFile path-to-pkt)'
    }

    "Parse" {
        Test-RetailSniffToolchain $paths
        $targetPkt = if ($PktFile) { $PktFile } else { (Get-LatestPktFile $paths).FullName }
        if (-not $targetPkt) {
            throw "No .pkt files found under $($paths.SniffRoot) or $($paths.DumpsDir)"
        }
        $result = Invoke-WowPacketParser -Paths $paths -InputPkt $targetPkt -CopyToTemp:$CopyParsedToTemp
        Write-Host "Parsed:" -ForegroundColor Green
        Write-Host "  PKT:    $($result.PktFile)"
        Write-Host "  Output: $($result.ParsedFile)"
    }

    "Status" {
        Write-Host "Retail sniff toolchain:" -ForegroundColor Cyan
        $paths.GetEnumerator() | Sort-Object Name | ForEach-Object {
            $exists = if ($_.Key -match 'Dir$|Root$') { Test-Path $_.Value } else { Test-Path $_.Value }
            $flag = if ($exists) { "OK" } else { "MISSING" }
            Write-Host ("  [{0}] {1} = {2}" -f $flag, $_.Key, $_.Value)
        }
        $ymir = Get-Process -Name "ffins" -ErrorAction SilentlyContinue
        Write-Host ""
        Write-Host ("Ymir running: {0}" -f [bool]$ymir)
        $latest = Get-LatestPktFile $paths
        if ($latest) {
            Write-Host "Latest .pkt: $($latest.FullName) ($($latest.LastWriteTime))"
        }
    }
}
