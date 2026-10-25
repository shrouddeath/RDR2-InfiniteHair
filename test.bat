@echo off
rem Offline test: drawable names in src\groom.cpp against the femga list.
setlocal
cd /d "%~dp0"

set "VS="
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\infinitehair_vs.txt" 2>nul
set /p VS=<"%TEMP%\infinitehair_vs.txt"
del "%TEMP%\infinitehair_vs.txt" >nul 2>&1
if "%VS%"=="" (
    echo [!] Visual Studio with C++ not found.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1

if not exist build\test mkdir build\test
cl /nologo /std:c++17 /EHsc /W4 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /Isrc /Fobuild\test\ ^
   tests\groom_test.cpp src\groom.cpp src\config.cpp /Fe:build\test\groom_test.exe >build\test\build.log 2>&1
if errorlevel 1 (
    type build\test\build.log
    exit /b 1
)
build\test\groom_test.exe tests\femga_player_zero_groom.txt
