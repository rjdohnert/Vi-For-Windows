@echo off
setlocal enabledelayedexpansion

echo Building Vi for Windows...

:: Check for Clang
where clang >nul 2>nul
if %errorlevel% equ 0 (
    echo Using Clang compiler...
    clang -Wall -Wextra -O2 src/*.c -luser32 -o vi.exe
    goto check_result
)

:: Check for GCC
where gcc >nul 2>nul
if %errorlevel% equ 0 (
    echo Using GCC compiler...
    gcc -Wall -Wextra -O2 src/*.c -luser32 -o vi.exe
    goto check_result
)

:: Check for MSVC cl
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo Using MSVC compiler (cl)...
    cl /nologo /W4 /O2 src\*.c user32.lib /Fe:vi.exe
    goto check_result
)

echo ERROR: No supported C compiler found (clang, gcc, or cl).
echo Please install Clang, MinGW (gcc), or Visual Studio Build Tools.
exit /b 1

:check_result
if %errorlevel% equ 0 (
    echo Build successful: vi.exe
    exit /b 0
) else (
    echo Build failed with error code %errorlevel%.
    exit /b %errorlevel%
)
