@echo off
REM HiAsm - Windows build helper (MSYS2 UCRT64 + CMake/Ninja).
REM
REM Usage:
REM   tools\build_windows.bat [gtk3|gtk4] [debug|release] [clean] [run] [installer]
REM
REM   gtk3        GTK3/gtkmm3 compatibility backend, presets windows-mingw-*
REM   gtk4        GTK4/gtkmm4 backend, presets windows-mingw-gtk4-*
REM   debug       configure/build the Debug tree (default)
REM   release     configure/build the Release tree
REM   clean       remove the selected build directory before configuring
REM   run         start hiasm.exe after a successful build
REM   installer   run tools\build_windows_installer.bat (Release + NSIS)
REM
REM Build directories:
REM   build\Windows_MinGW-x64-Debug, build\Windows_MinGW-x64-Release          (gtk3)
REM   build\Windows_MinGW-x64-GTK4-Debug, build\Windows_MinGW-x64-GTK4-Release (gtk4)
REM
REM Environment:
REM   MSYS2_ROOT  MSYS2 installation, default C:\msys64

setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
for %%I in ("%MSYS2_ROOT%") do set "MSYS2_ROOT=%%~fI"
set "GTK=4"
set "BUILD_TYPE=debug"
set "DO_CLEAN=0"
set "DO_RUN=0"
set "DO_INSTALLER=0"

:parse_args
if "%~1"=="" goto :args_done
if /I "%~1"=="gtk3" (
    set "GTK=3"
) else if /I "%~1"=="gtk4" (
    set "GTK=4"
) else if /I "%~1"=="debug" (
    set "BUILD_TYPE=debug"
) else if /I "%~1"=="release" (
    set "BUILD_TYPE=release"
) else if /I "%~1"=="clean" (
    set "DO_CLEAN=1"
) else if /I "%~1"=="run" (
    set "DO_RUN=1"
) else if /I "%~1"=="installer" (
    set "DO_INSTALLER=1"
) else (
    echo [ERROR] Unknown argument: %~1
    echo Usage: tools\build_windows.bat [gtk3^|gtk4] [debug^|release] [clean] [run] [installer]
    exit /b 2
)
shift
goto :parse_args

:args_done
if "%DO_INSTALLER%"=="1" goto :run_installer

set "UCRT=%MSYS2_ROOT%\ucrt64"
if not exist "%UCRT%\bin\g++.exe" (
    echo [ERROR] MSYS2 UCRT64 compiler not found: %UCRT%\bin\g++.exe
    echo Install it with:
    echo   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf ^
    echo     mingw-w64-ucrt-x86_64-gtkmm3 mingw-w64-ucrt-x86_64-gtksourceviewmm3 ^
    echo     mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake
    echo   GTK4 backend additionally needs: mingw-w64-ucrt-x86_64-gtk4 mingw-w64-ucrt-x86_64-gtkmm-4.0
    exit /b 1
)

set "PATH=%UCRT%\bin;%MSYS2_ROOT%\usr\bin;%PATH%"
set "XDG_DATA_DIRS=%UCRT%\share;%MSYS2_ROOT%\usr\share"

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install mingw-w64-ucrt-x86_64-cmake or add CMake to PATH.
    exit /b 1
)
where ninja.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Ninja not found. Install mingw-w64-ucrt-x86_64-ninja.
    exit /b 1
)
if not exist "%UCRT%\bin\pkg-config.exe" (
    echo [ERROR] pkg-config not found: %UCRT%\bin\pkg-config.exe
    exit /b 1
)
if not exist "%UCRT%\bin\sqlite3.exe" (
    echo [ERROR] sqlite3 not found: %UCRT%\bin\sqlite3.exe
    echo Install mingw-w64-ucrt-x86_64-sqlite3.
    exit /b 1
)

if "%GTK%"=="4" (
    set "PRESET_PREFIX=windows-mingw-gtk4"
    set "DIR_TAG=GTK4-"
) else (
    set "PRESET_PREFIX=windows-mingw"
    set "DIR_TAG="
)
if /I "%BUILD_TYPE%"=="release" (
    set "PRESET=%PRESET_PREFIX%-release"
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-%DIR_TAG%Release"
) else (
    set "PRESET=%PRESET_PREFIX%-debug"
    set "BUILD_DIR=%ROOT%\build\Windows_MinGW-x64-%DIR_TAG%Debug"
)

pushd "%ROOT%" >nul

if "%DO_CLEAN%"=="1" (
    echo === Clean: %BUILD_DIR%
    if exist "%BUILD_DIR%" cmake -E rm -rf "%BUILD_DIR%"
)

echo === HiAsm Windows: preset %PRESET%
set "FTCG_ARGS="
if exist "%ROOT%\build\deps\fpc-3.2.2\extracted\app\bin\i386-win32\ppcrossx64.exe" set FTCG_ARGS=-DHIASM_BUILD_FTCG=ON -DFPC_EXECUTABLE="%SCRIPT_DIR%fpc_windows.bat"
cmake --preset "%PRESET%" -DCMAKE_CXX_COMPILER="%UCRT%\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="%UCRT%\bin\ninja.exe" -DPKG_CONFIG_EXECUTABLE="%UCRT%\bin\pkg-config.exe" -DHIASM_BUILD_RTCG=ON %FTCG_ARGS%
if errorlevel 1 goto :fail

echo === Building...
cmake --build --preset "%PRESET%" --parallel
if errorlevel 1 goto :fail

if not exist "%BUILD_DIR%\hiasm.exe" (
    echo [ERROR] Build finished but %BUILD_DIR%\hiasm.exe is missing.
    goto :fail
)

echo.
echo Built: %BUILD_DIR%\hiasm.exe

if "%DO_RUN%"=="1" (
    echo === Starting hiasm.exe
    powershell.exe -NoProfile -Command "$ErrorActionPreference = 'Stop'; Start-Process -FilePath '%BUILD_DIR%\hiasm.exe' -WorkingDirectory '%BUILD_DIR%'"
    if errorlevel 1 goto :fail
)

popd >nul
exit /b 0

:run_installer
call "%SCRIPT_DIR%build_windows_installer.bat" gtk%GTK%
set "RESULT=%ERRORLEVEL%"
endlocal & exit /b %RESULT%

:fail
popd >nul
exit /b 1
