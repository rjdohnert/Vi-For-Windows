<#
.SYNOPSIS
    Installs Vi for Windows to the user's environment and adds it to PATH.

.DESCRIPTION
    Copies vi.exe to %LOCALAPPDATA%\Programs\vi and adds that directory to the
    User PATH environment variable so 'vi' can be run from any terminal.
    Supports -Uninstall to cleanly remove Vi and its PATH entry.

.PARAMETER AllUsers
    Installs system-wide to %ProgramFiles%\vi and updates Machine PATH (requires Administrator).
    Ideal for Windows Server Core multi-admin or remote SSH/PowerShell environments.

.PARAMETER Uninstall
    Removes Vi from the system and removes the PATH entry.
#>

[CmdletBinding()]
param (
    [switch]$AllUsers,
    [switch]$Uninstall,
    [switch]$ContextMenu
)

$ErrorActionPreference = "Stop"

$TargetScope = if ($AllUsers) { [EnvironmentVariableTarget]::Machine } else { [EnvironmentVariableTarget]::User }
$ScopeName = if ($AllUsers) { "System (Machine)" } else { "User" }
$InstallDir = if ($AllUsers) { Join-Path $env:ProgramFiles "vi" } else { Join-Path $env:LOCALAPPDATA "Programs\vi" }
$TargetExe = Join-Path $InstallDir "vi.exe"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if ($AllUsers) {
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Error "Administrative privileges are required to install with -AllUsers. Please run PowerShell as Administrator."
        exit 1
    }
}

function Get-TargetPath {
    return [Environment]::GetEnvironmentVariable("Path", $TargetScope)
}

function Set-TargetPath([string]$PathVal) {
    [Environment]::SetEnvironmentVariable("Path", $PathVal, $TargetScope)
}

if ($Uninstall) {
    Write-Host "Uninstalling Vi for Windows ($ScopeName)..." -ForegroundColor Yellow

    if (Test-Path $InstallDir) {
        Remove-Item -Path $InstallDir -Recurse -Force
        Write-Host "  Removed: $InstallDir" -ForegroundColor Green
    }

    $TargetPath = Get-TargetPath
    if ($TargetPath) {
        $Paths = $TargetPath -split ';' | Where-Object { $_ -ne '' -and $_ -ne $InstallDir }
        $NewPath = $Paths -join ';'
        Set-TargetPath $NewPath
        Write-Host "  Removed '$InstallDir' from $ScopeName PATH." -ForegroundColor Green
    }

    $regKey = "HKCU:\Software\Classes\*\shell\Open with Vi"
    if (Test-Path $regKey) {
        Remove-Item -Path $regKey -Recurse -Force
        Write-Host "  Removed Windows Explorer context menu entry." -ForegroundColor Green
    }

    Write-Host "Vi for Windows has been successfully uninstalled." -ForegroundColor Green
    exit 0
}

# 1. Build if vi.exe does not exist
$SourceExe = Join-Path $ScriptDir "vi.exe"
if (-not (Test-Path $SourceExe)) {
    Write-Host "vi.exe not found in script directory. Building vi..." -ForegroundColor Cyan
    $BuildScript = Join-Path $ScriptDir "build.ps1"
    if (Test-Path $BuildScript) {
        & $BuildScript
    } else {
        Write-Error "build.ps1 not found. Cannot compile vi.exe."
        exit 1
    }
}

if (-not (Test-Path $SourceExe)) {
    Write-Error "Build failed: vi.exe was not generated."
    exit 1
}

# 2. Create Target Directory
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Write-Host "Created installation directory: $InstallDir" -ForegroundColor Gray
}

# 3. Copy vi.exe and create view.exe alias
Copy-Item -Path $SourceExe -Destination $TargetExe -Force
$TargetViewExe = Join-Path $InstallDir "view.exe"
Copy-Item -Path $SourceExe -Destination $TargetViewExe -Force
Write-Host "Copied vi.exe and view.exe to $InstallDir" -ForegroundColor Green

# 4. Check & Update PATH
$CurrentPath = Get-TargetPath
$Paths = if ($CurrentPath) { $CurrentPath -split ';' | Where-Object { $_ -ne '' } } else { @() }

if ($Paths -contains $InstallDir) {
    Write-Host "Directory is already in $ScopeName PATH: $InstallDir" -ForegroundColor Gray
} else {
    $NewPath = if ($CurrentPath) { "$CurrentPath;$InstallDir" } else { $InstallDir }
    Set-TargetPath $NewPath
    Write-Host "Added '$InstallDir' to $ScopeName PATH." -ForegroundColor Green
}

# Update current session PATH so vi is immediately accessible in this terminal
if ($env:Path -split ';' -notcontains $InstallDir) {
    $env:Path = "$env:Path;$InstallDir"
}

# 5. Windows Explorer Context Menu Integration
if ($ContextMenu) {
    $regPath = "HKCU:\Software\Classes\*\shell\Open with Vi"
    New-Item -Path $regPath -Force | Out-Null
    Set-ItemProperty -Path $regPath -Name "(Default)" -Value "Open with Vi"
    Set-ItemProperty -Path $regPath -Name "Icon" -Value "$TargetExe,0"
    $cmdPath = Join-Path $regPath "command"
    New-Item -Path $cmdPath -Force | Out-Null
    Set-ItemProperty -Path $cmdPath -Name "(Default)" -Value "`"$TargetExe`" `"%1`""
    Write-Host "Registered Windows Explorer Context Menu: 'Open with Vi'" -ForegroundColor Green
}

Write-Host ""
Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  Vi for Windows installed successfully!" -ForegroundColor Green
Write-Host "  Location: $TargetExe" -ForegroundColor White
Write-Host "  You can now run 'vi' (or 'view' for read-only) from any terminal window." -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Green
