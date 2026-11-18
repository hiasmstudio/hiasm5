@echo off
REM HiAsm - build a Release tree, deploy the MSYS2 UCRT64 runtime and
REM compile an NSIS installer.
REM
REM Requires: MSYS2 UCRT64 (GCC/CMake/Ninja/GTK3 or GTK4), NSIS 3.x.
REM Optional environment:
REM   MSYS2_ROOT  MSYS2 installation, default C:\msys64

setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
set "GTK=4"
if /I "%~1"=="gtk4" set "GTK=4"
if /I "%~1"=="gtk3" set "GTK=3"
if not "%~1"=="" if /I not "%~1"=="gtk3" if /I not "%~1"=="gtk4" (
    echo [ERROR] Usage: tools\build_windows_installer.bat [gtk3^|gtk4]
    exit /b 2
)
if "%GTK%"=="4" (
    set "PRESET=windows-mingw-gtk4-release"
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-GTK4-Release"
    set "INSTALL_DIR=%ROOT%\build\windows-installer\gtk4-install"
    set "OUTPUT_FILE=%ROOT%\build\HiAsm_Setup_GTK4.exe"
) else (
    set "PRESET=windows-mingw-release"
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-Release"
    set "INSTALL_DIR=%ROOT%\build\windows-installer\gtk3-install"
    set "OUTPUT_FILE=%ROOT%\build\HiAsm_Setup.exe"
)
set "NSIS_SCRIPT=%SCRIPT_DIR%nsis_installer.nsi"

if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
for %%I in ("%MSYS2_ROOT%") do set "MSYS2_ROOT=%%~fI"
set "UCRT=%MSYS2_ROOT%\ucrt64"
set "PATH=%UCRT%\bin;%MSYS2_ROOT%\usr\bin;%PATH%"

echo ========================================
echo HiAsm - Windows Installer Builder
echo ========================================
echo.

if not exist "%UCRT%\bin\g++.exe" (
    echo [ERROR] MSYS2 UCRT64 compiler not found: %UCRT%\bin\g++.exe
    goto :fail
)
if not exist "%UCRT%\bin\pkg-config.exe" (
    echo [ERROR] pkg-config not found: %UCRT%\bin\pkg-config.exe
    goto :fail
)
if not exist "%UCRT%\bin\sqlite3.exe" (
    echo [ERROR] sqlite3 not found: %UCRT%\bin\sqlite3.exe
    goto :fail
)
if not exist "%UCRT%\bin\ninja.exe" (
    echo [ERROR] Ninja not found: %UCRT%\bin\ninja.exe
    goto :fail
)

set "MAKENSIS="
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
) else if exist "C:\Program Files\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
)
if not defined MAKENSIS (
    for /f "delims=" %%i in ('where makensis 2^>nul') do (
        if not defined MAKENSIS set "MAKENSIS=%%i"
    )
)
if not defined MAKENSIS (
    echo [ERROR] NSIS ^(makensis^) not found.
    echo Install NSIS: https://nsis.sourceforge.io/Download
    goto :fail
)

set "CMAKE_CMD="
for /f "delims=" %%i in ('where cmake 2^>nul') do (
    if not defined CMAKE_CMD set "CMAKE_CMD=%%i"
)
if not defined CMAKE_CMD (
    echo [ERROR] CMake not found. Install mingw-w64-ucrt-x86_64-cmake.
    goto :fail
)

REM Version is kept in one place: src/share.h.
set "PRODUCT_VERSION=6.0.0.0"
set "VER_MAJOR="
set "VER_MINOR="
set "VER_BUILD="
set "VER_TWEAK="
for /f "tokens=3" %%a in ('findstr /b /c:"#define HIASM_VERSION_MAJOR" "%ROOT%\src\share.h"') do set "VER_MAJOR=%%a"
for /f "tokens=3" %%a in ('findstr /b /c:"#define HIASM_VERSION_MINOR" "%ROOT%\src\share.h"') do set "VER_MINOR=%%a"
for /f "tokens=3" %%a in ('findstr /b /c:"#define HIASM_VERSION_BUILD" "%ROOT%\src\share.h"') do set "VER_BUILD=%%a"
for /f "tokens=3" %%a in ('findstr /b /c:"#define HIASM_VERSION_TWEAK" "%ROOT%\src\share.h"') do set "VER_TWEAK=%%a"
if defined VER_MAJOR if defined VER_MINOR if defined VER_BUILD if defined VER_TWEAK set "PRODUCT_VERSION=%VER_MAJOR%.%VER_MINOR%.%VER_BUILD%.%VER_TWEAK%"

if not exist "%ROOT%\resources\hiasm.ico" (
    echo [0/5] Generating resources\hiasm.ico...
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%png_to_ico.ps1" -Source "%ROOT%\resources\hiasm.png" -Output "%ROOT%\resources\hiasm.ico"
    if errorlevel 1 (
        echo [ERROR] Could not generate resources\hiasm.ico.
        goto :fail
    )
)

echo [1/5] CMake configure ^(%PRESET%^)...
pushd "%ROOT%" >nul
set "FTCG_ARGS="
if exist "%ROOT%\build\deps\fpc-3.2.2\extracted\app\bin\i386-win32\ppcrossx64.exe" set FTCG_ARGS=-DHIASM_BUILD_FTCG=ON -DFPC_EXECUTABLE="%SCRIPT_DIR%fpc_windows.bat"
"%CMAKE_CMD%" --preset "%PRESET%" -DCMAKE_CXX_COMPILER="%UCRT%\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="%UCRT%\bin\ninja.exe" -DPKG_CONFIG_EXECUTABLE="%UCRT%\bin\pkg-config.exe" -DHIASM_BUILD_RTCG=ON %FTCG_ARGS%
if errorlevel 1 (
    popd >nul
    echo [ERROR] CMake configure failed.
    goto :fail
)

echo [2/5] Building Release...
"%CMAKE_CMD%" --build --preset "%PRESET%" --parallel
if errorlevel 1 (
    popd >nul
    echo [ERROR] Build failed.
    goto :fail
)
popd >nul

echo [3/5] Installing to %INSTALL_DIR%...
if exist "%INSTALL_DIR%" "%CMAKE_CMD%" -E rm -rf "%INSTALL_DIR%"
"%CMAKE_CMD%" --install "%BUILD_DIR%" --prefix "%INSTALL_DIR%"
if errorlevel 1 (
    echo [ERROR] Install failed.
    goto :fail
)

echo [4/5] Deploying GTK/GLib runtime...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%deploy_gtk_runtime.ps1" -InstallDir "%INSTALL_DIR%" -RuntimeSource "%BUILD_DIR%" -Msys2Root "%MSYS2_ROOT%" -GtkVersion "%GTK%"
if errorlevel 1 (
    echo [ERROR] GTK runtime deployment failed.
    goto :fail
)

echo [5/6] Staging bundled GCC and GTK4 development files...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%stage_cpp_toolchain.ps1" -Destination "%INSTALL_DIR%\toolchains\mingw64" -Msys2Root "%MSYS2_ROOT%"
if errorlevel 1 (
    echo [ERROR] C++ toolchain deployment or isolation verification failed.
    goto :fail
)

echo [6/6] Creating NSIS installer...
pushd "%SCRIPT_DIR%" >nul
"%MAKENSIS%" /DBUILD_DIR="%INSTALL_DIR%" /DOUTPUT_FILE="%OUTPUT_FILE%" /DPRODUCT_VERSION="%PRODUCT_VERSION%" /DGTK_VERSION="%GTK%" "%NSIS_SCRIPT%"
set "NSIS_RC=%ERRORLEVEL%"
popd >nul
if not "%NSIS_RC%"=="0" (
    echo [ERROR] NSIS installer creation failed.
    goto :fail
)

echo.
echo ========================================
echo Done! Installer: %OUTPUT_FILE%
echo Version: %PRODUCT_VERSION%
echo ========================================
endlocal
exit /b 0

:fail
endlocal
exit /b 1
