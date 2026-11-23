@echo off
setlocal EnableExtensions
set "SCRIPT_DIR=%~dp0"
set "CONFIG=Debug"
if /i "%~1"=="qt" shift
if /i "%~1"=="installer" goto installer
if /i "%~1"=="release" set "CONFIG=Release"
if /i "%~1"=="%CONFIG%" shift
set "RUN=-RunIde"
if /i "%~1"=="build-only" set "RUN="
if not "%~1"=="" if /i not "%~1"=="run" if /i not "%~1"=="build-only" goto usage
if not "%~2"=="" goto usage
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build_qt_windows.ps1" -Configuration %CONFIG% %RUN%
exit /b %ERRORLEVEL%
:installer
if not "%~2"=="" goto usage
call "%SCRIPT_DIR%build_windows_installer.bat"
exit /b %ERRORLEVEL%
:usage
echo Usage: build_windows.bat [qt] [debug^|release] [run^|build-only] or build_windows.bat installer
exit /b 2
