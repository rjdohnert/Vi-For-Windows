<#
.SYNOPSIS
    Builds Vi for Windows.
.DESCRIPTION
    Compiles all C modules in src/*.c into vi.exe using Clang, GCC, or MSVC.
.PARAMETER Configuration
    Build configuration: "Release" (default, -O2) or "Debug" (-g -O0).
.PARAMETER Compiler
    Force a specific compiler: "clang", "gcc", or "cl". Defaults to auto-detection.
.PARAMETER Run
    If set, launches vi.exe after successful build.
.EXAMPLE
    .\build.ps1
.EXAMPLE
    .\build.ps1 -Configuration Debug
.EXAMPLE
    .\build.ps1 -Run
#>
[CmdletBinding()]
param (
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",

    [ValidateSet("auto", "clang", "gcc", "cl")]
    [string]$Compiler = "auto",

    [switch]$Run
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "=== Building Vi for Windows ($Configuration) ===" -ForegroundColor Cyan

# Detect compiler
$selectedCompiler = $Compiler
if ($selectedCompiler -eq "auto") {
    if (Get-Command clang -ErrorAction SilentlyContinue) {
        $selectedCompiler = "clang"
    } elseif (Get-Command gcc -ErrorAction SilentlyContinue) {
        $selectedCompiler = "gcc"
    } elseif (Get-Command cl -ErrorAction SilentlyContinue) {
        $selectedCompiler = "cl"
    } else {
        Write-Error "No supported C compiler found (clang, gcc, or cl). Please install Clang, GCC, or MSVC."
        exit 1
    }
}

Write-Host "Selected Compiler: $selectedCompiler" -ForegroundColor Yellow

$sources = Get-ChildItem -Path "src" -Filter "*.c" | ForEach-Object { $_.FullName }
if (-not $sources) {
    Write-Error "No C source files found in src/."
    exit 1
}

$outputExe = Join-Path $scriptDir "vi.exe"

$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

try {
    switch ($selectedCompiler) {
        "clang" {
            $flags = if ($Configuration -eq "Release") { @("-Wall", "-Wextra", "-O2") } else { @("-Wall", "-Wextra", "-g", "-O0", "-D_DEBUG") }
            & clang @flags src/*.c -luser32 -o $outputExe
        }
        "gcc" {
            $flags = if ($Configuration -eq "Release") { @("-Wall", "-Wextra", "-O2") } else { @("-Wall", "-Wextra", "-g", "-O0", "-D_DEBUG") }
            & gcc @flags src/*.c -luser32 -o $outputExe
        }
        "cl" {
            $flags = if ($Configuration -eq "Release") { @("/nologo", "/W4", "/O2") } else { @("/nologo", "/W4", "/Zi", "/Od", "/D_DEBUG") }
            & cl @flags src\*.c user32.lib /Fe:$outputExe
        }
    }

    if ($LASTEXITCODE -eq 0 -and (Test-Path $outputExe)) {
        $stopwatch.Stop()
        $sizeKb = [math]::Round((Get-Item $outputExe).Length / 1KB, 1)
        Write-Host "Build successful: $outputExe ($sizeKb KB in $($stopwatch.ElapsedMilliseconds)ms)" -ForegroundColor Green

        if ($Run) {
            Write-Host "Launching vi.exe..." -ForegroundColor Magenta
            & $outputExe
        }
    } else {
        Write-Error "Build failed with exit code $LASTEXITCODE."
        exit $LASTEXITCODE
    }
} catch {
    Write-Error "Build error: $_"
    exit 1
}
