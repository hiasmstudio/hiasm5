@echo off
setlocal EnableExtensions
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
set "GTK=4"
set "DO_BUILD=0"
if /I "%~1"=="gtk3" (
    set "GTK=3"
) else if not "%~1"=="" if /I not "%~1"=="gtk4" (
    echo [ERROR] Unknown argument: %~1
    echo Usage: tools\run-debug.bat [gtk3^|gtk4]
    exit /b 2
)
if /I "%~2"=="build" set "DO_BUILD=1"
if not "%~2"=="" if /I not "%~2"=="build" (
    echo [ERROR] Too many arguments.
    echo Usage: tools\run-debug.bat [gtk3^|gtk4]
    exit /b 2
)
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
for %%I in ("%MSYS2_ROOT%") do set "MSYS2_ROOT=%%~fI"
if not exist "%MSYS2_ROOT%\ucrt64\bin\g++.exe" (
    echo [ERROR] MSYS2 UCRT64 not found: %MSYS2_ROOT%
    exit /b 1
)
set "PATH=%MSYS2_ROOT%\ucrt64\bin;%MSYS2_ROOT%\usr\bin;%PATH%"
set "XDG_DATA_DIRS=%MSYS2_ROOT%\ucrt64\share;%MSYS2_ROOT%\usr\share"

if "%GTK%"=="4" (
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-GTK4-Debug"
) else (
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-Debug"
)
if not exist "%BUILD_DIR%\hiasm.exe" (
    set "DO_BUILD=1"
)
if not exist "%BUILD_DIR%\int" (
    set "DO_BUILD=1"
)
if not exist "%BUILD_DIR%\packs" (
    set "DO_BUILD=1"
)
if "%DO_BUILD%"=="1" (
    rem Close a previous debug instance so the linker can replace hiasm.exe.
    taskkill /IM hiasm.exe /T /F >nul 2>&1
    echo === Building missing or requested GTK%GTK% Debug runtime
    call "%SCRIPT_DIR%build_windows.bat" gtk%GTK% debug
    if errorlevel 1 exit /b 1
)

echo === Starting hiasm.exe from %BUILD_DIR%
rem Use the native Windows launcher so this helper returns immediately while
rem the GUI keeps running independently from the terminal or VS Code task.
pushd "%BUILD_DIR%" >nul
start "HiAsm" /b "hiasm.exe"
set "START_RC=%ERRORLEVEL%"
popd >nul
if not "%START_RC%"=="0" (
    echo [ERROR] Failed to start hiasm.exe.
    exit /b 1
)
exit /b 0
