@echo off
setlocal EnableExtensions
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "FPC_ROOT=%ROOT%\build\deps\fpc-3.2.2\extracted\app"
if not exist "%FPC_ROOT%\bin\i386-win32\ppcrossx64.exe" (
    echo [ERROR] Win64 Free Pascal is missing. Run tools\setup_fpc_windows.ps1.
    exit /b 1
)
"%FPC_ROOT%\bin\i386-win32\ppcrossx64.exe" -n -Twin64 -Px86_64 "-Fu%FPC_ROOT%\units\x86_64-win64\*" "-FD%FPC_ROOT%\bin\i386-win32" %*
exit /b %ERRORLEVEL%
