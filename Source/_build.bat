@echo off
setlocal enabledelayedexpansion

rem Usage: _build.bat OS BITNESS
rem for example: _build.bat 10 64
rem from MSVC command prompt

set os=%1
set bits=%2

if "%os%"=="2000" call :buildWin2k
if "%os%"=="10" call :buildWin10
if "%os%"=="7" call :buildWin7

rem GitHub
if "%os%"=="windows-latest" call :buildWin10
if "%os%"=="windows-2022" call :buildWin10
if "%os%"=="windows-2019" call :buildWin10

rem pause
del *.obj *.lib *.exp

exit /b

:buildWin10
rem Everything - mouse, keyboard, controller, audio, network
set files=*.cpp Conhost\*.cpp Cmd\*.cpp
set libs=ntdll.lib user32.lib shell32.lib shcore.lib
set flags=/std:c++23 /LD /DMODERN_WINDOWS /DVERY_MODERN_WINDOWS

call :build_generic
exit /b

:buildWin7
rem Also everything
set files=*.cpp Conhost\*.cpp Cmd\*.cpp
set libs=ntdll.lib user32.lib shell32.lib
set flags=/LD /DMODERN_WINDOWS

call :build_generic
exit /b

:buildWin2k
set libs=ntdll.lib user32.lib shell32.lib
set flags=

set cmdfiles=*.cpp Cmd\CmdMain.cpp Cmd\Keyboard.cpp Cmd\Mouse.cpp
set conhostfiles=Conhost\ConhostMain.cpp Conhost\Audio.cpp Conhost\ConsoleUtils.cpp

cl /LD !flags! !libs! !cmdfiles! /O2 /I. /ICmd /IConhost /EHsc /link /out:bn%bits%_%os%.dll
cl !flags! !libs! !conhostfiles! /O2 /I. /ICmd /IConhost /EHsc /link /out:bn_ipc.exe
exit /b

:build_generic
cl !flags! !files! !libs! /O2 /I. /ICmd /IConhost /EHsc /link /out:bn%bits%_%os%.dll
exit /b
