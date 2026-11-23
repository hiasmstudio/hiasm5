@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
if /i "%~1"=="qt" shift
if not "%~1"=="" exit /b 2
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build_windows_installer.ps1"
exit /b %ERRORLEVEL%
