@echo off
setlocal enabledelayedexpansion

mode 125,55

SET BATCHNATIVE_BIN_HOME=.
set /a getinput_rasterx=16,getinput_rastery=12
rem set /a limitMouseX=limitMouseY=40
rundll32 %BATCHNATIVE_BIN_HOME%\getInput64.dll,inject

echo Variables are displayed in the title bar
echo The format is: click mousexpos mouseypos wheeldelta ; keyspressed ; controller1 ; controller2 ; controller3 ; controller4

:a
title !getinput_click! !getinput_mousexpos! !getinput_mouseypos! !getinput_wheeldelta! ; !getinput_keyspressed! ; !getinput_controller1_btns! ; !getinput_controller2! ; !getinput_controller3! ; !getinput_controller4!
cmd /c
if "!keyspressed!"=="-32-" (
	<nul set/p="mousePointer load" > \\.\pipe\getinput
	rem set /a rasterx=10,rastery=18
	rem set /a screenx=5,screeny=5
	set /p test=Type text: 
)

goto :a