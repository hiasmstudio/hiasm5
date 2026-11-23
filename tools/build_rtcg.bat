@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_qt_windows.ps1" -Configuration Debug -RtcgOnly -BuildDir "%~dp0..\build\rtcg-windows-mingw_x64-debug"
exit /b %ERRORLEVEL%
