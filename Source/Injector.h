#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef __cplusplus
#define NOMANGLE extern "C"
#else
#define NOMANGLE
#endif

DWORD getppid();
DWORD getconhost();

void hookSelfToProcess(DWORD pid);

NOMANGLE __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

NOMANGLE __declspec(dllexport) void CALLBACK hook(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
NOMANGLE __declspec(dllexport) void CALLBACK version(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
