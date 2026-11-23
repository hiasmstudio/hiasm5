@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
if /i "%~1"=="qt" shift
set "CHECK="
if /i "%~1"=="check" set "CHECK=-Check"
if not "%~1"=="" if not defined CHECK exit /b 2
if not "%~2"=="" exit /b 2
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build_qt_windows.ps1" -Configuration Debug -RunIde %CHECK%
exit /b %ERRORLEVEL%
