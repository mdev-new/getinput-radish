@echo off

rem Usage: _build.bat OS BITNESS
rem for example: _build.bat 10 64
rem from MSVC command prompt

set os=%1
set bits=%2

if "%os%"=="10" call :buildWin10
if "%os%"=="7" call :buildWin7
if "%os%"=="2000" call :buildWin2k

cl /LD %files% %libs% /O2 /I. /ICmd /IConhost %flags% /EHsc /link /out:getInput%bits%_win%os%.dll

pause
del *.obj *.lib *.exp

exit /b

:buildWin10
rem Everything - mouse, keyboard, controller, audio, network
set files=*.cpp Conhost\*.cpp Cmd\*.cpp
set libs=ntdll.lib user32.lib
set flags=/DMODERN_WINDOWS /DVERY_MODERN_WINDOWS
exit /b

:buildWin7
set files=*.cpp Conhost\*.cpp Cmd\*.cpp
set libs=ntdll.lib user32.lib
set flags=/DMODERN_WINDOWS
exit /b

:buildWin2k
set files=*.cpp Cmd\CmdMain.cpp Cmd\Keyboard.cpp Cmd\Mouse.cpp
set libs=ntdll.lib user32.lib
set flags=
exit /b
