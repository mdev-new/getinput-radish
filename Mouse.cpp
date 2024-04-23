#include "Mouse.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
    short wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

    if (wheelDelta != 0) {
        wheelDelta /= WHEEL_DELTA;
        ENV("getinput_wheeldelta", itoa_(wheelDelta));
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD CALLBACK MouseMessageThread(void* data) {
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
