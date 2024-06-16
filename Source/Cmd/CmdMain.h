#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL DllMain_load_cmd(HINSTANCE hInst, LPVOID lpReserved);
BOOL DllMain_unload_cmd(HINSTANCE hInst, LPVOID lpReserved);
