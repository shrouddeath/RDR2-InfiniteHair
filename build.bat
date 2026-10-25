@echo off
rem Builds InfiniteHair.asi with the MSVC compiler from Visual Studio 2022.
rem Usage: build.bat [sdk-dir]   (default: .\sdk)
setlocal
cd /d "%~dp0"

set "SDK=%~1"
if "%SDK%"=="" set "SDK=%~dp0sdk"
if not exist "%SDK%\inc\main.h" (
    echo [!] ScriptHookRDR2 SDK not found: "%SDK%\inc\main.h" is missing.
    echo     Unpack the SDK zip so that sdk\inc\main.h and sdk\lib\ScriptHookRDR2.lib exist.
    exit /b 1
)

rem Via a temp file instead of for /f: "(x86)" in the path breaks for /f parsing.
set "VS="
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\infinitehair_vs.txt" 2>nul
set /p VS=<"%TEMP%\infinitehair_vs.txt"
del "%TEMP%\infinitehair_vs.txt" >nul 2>&1
if "%VS%"=="" (
    echo [!] Visual Studio with C++ not found.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1

if not exist build mkdir build
cl /nologo /std:c++17 /O2 /MT /EHsc /W4 /permissive- /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ^
   /I"%SDK%\inc" /Isrc /Fobuild\ ^
   src\main.cpp src\script.cpp src\config.cpp src\groom.cpp src\log.cpp ^
   /LD /Fe:build\InfiniteHair.asi ^
   /link "%SDK%\lib\ScriptHookRDR2.lib" user32.lib kernel32.lib
if errorlevel 1 (
    echo [!] Build fehlgeschlagen.
    exit /b 1
)
echo.
echo [OK] build\InfiniteHair.asi
