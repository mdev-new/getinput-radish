#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <xinput.h>
#pragma comment(lib, "XInput9_1_0.lib")

VOID ControllerUpdate(float deadzone);
