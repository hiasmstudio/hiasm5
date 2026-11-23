@echo off
setlocal EnableExtensions
if not defined HIASM_QT_COMPILER_DIR set "HIASM_QT_COMPILER_DIR=C:\Qt\Tools\llvm-mingw1706_64\bin"
set "MAKE=%HIASM_QT_COMPILER_DIR%\mingw32-make.exe"
if not exist "%MAKE%" (
    echo Cannot find Qt SDK make: "%MAKE%"
    exit /b 1
)
if not defined HIASM_CMAKE set "HIASM_CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
for %%I in ("%HIASM_CMAKE%") do set "PATH=%%~dpI;%HIASM_QT_COMPILER_DIR%;%PATH%"
pushd "%~dp0.."
if errorlevel 1 exit /b 1
"%MAKE%" -f Makefile.win %*
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
