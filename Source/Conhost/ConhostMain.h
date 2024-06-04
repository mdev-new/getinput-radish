#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL DllMain_load_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);
BOOL DllMain_unload_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);
