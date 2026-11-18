@echo off
setlocal EnableExtensions
set "TOOLS_DIR=%~dp0"
for %%I in ("%TOOLS_DIR%..") do set "PROJECT_ROOT=%%~fI"

if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
for %%I in ("%MSYS2_ROOT%") do set "MSYS2_ROOT=%%~fI"
if not exist "%MSYS2_ROOT%\ucrt64\bin\g++.exe" (
    echo [ERROR] MSYS2 UCRT64 not found: %MSYS2_ROOT%
    exit /b 1
)
set "PATH=%MSYS2_ROOT%\ucrt64\bin;%MSYS2_ROOT%\usr\bin;%PATH%"

where mingw32-make.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] mingw32-make.exe not found in PATH.
    exit /b 1
)
pushd "%PROJECT_ROOT%" >nul
if errorlevel 1 (
    echo [ERROR] Could not enter project directory: %PROJECT_ROOT%
    exit /b 1
)
mingw32-make -f Makefile.win %*
set "EXIT_CODE=%ERRORLEVEL%"
popd

endlocal & exit /b %EXIT_CODE%
