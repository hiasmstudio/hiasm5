@echo off
SET THEFILE=C:\work\hiasm\Elements\delphi\Example\Draw\game\GameEngine\Balls.exe
echo Linking %THEFILE%
ld.exe  -s --subsystem windows   -o C:\work\hiasm\Elements\delphi\Example\Draw\game\GameEngine\Balls.exe C:\work\hiasm\Elements\delphi\Example\Draw\game\GameEngine\link.res
if errorlevel 1 goto linkend
goto end
:asmend
echo An error occured while assembling %THEFILE%
goto end
:linkend
echo An error occured while linking %THEFILE%
:end
