@echo off
setlocal EnableExtensions
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
for %%I in ("%MSYS2_ROOT%") do set "MSYS2_ROOT=%%~fI"
set "UCRT=%MSYS2_ROOT%\ucrt64"
set "PATH=%UCRT%\bin;%MSYS2_ROOT%\usr\bin;%PATH%"
set "BUILD_DIR=%ROOT%\build\rtcg-windows-mingw_x64-debug"
if not exist "%UCRT%\bin\g++.exe" (
    echo [ERROR] MSYS2 UCRT64 compiler not found: %UCRT%\bin\g++.exe
    exit /b 1
)
where cmake.exe >nul 2>&1 || (echo [ERROR] CMake not found.& exit /b 1)
if not exist "%UCRT%\bin\ninja.exe" (
    echo [ERROR] Ninja not found: %UCRT%\bin\ninja.exe
    exit /b 1
)
if not exist "%UCRT%\bin\pkg-config.exe" (
    echo [ERROR] pkg-config not found: %UCRT%\bin\pkg-config.exe
    exit /b 1
)
set "XDG_DATA_DIRS=%UCRT%\share;%MSYS2_ROOT%\usr\share"
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DHIASM_GTK_VERSION=4 -DHIASM_BUILD_RTCG=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="%UCRT%\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="%UCRT%\bin\ninja.exe" -DPKG_CONFIG_EXECUTABLE="%UCRT%\bin\pkg-config.exe"
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --target rtcg_codegen --parallel
if errorlevel 1 exit /b 1
if not exist "%BUILD_DIR%\packs\cpp\compiler\CodeGen.dll" (
    echo [ERROR] RTCG build finished but CodeGen.dll is missing.
    exit /b 1
)
echo Built: %BUILD_DIR%\packs\cpp\compiler\CodeGen.dll
endlocal
exit /b 0
