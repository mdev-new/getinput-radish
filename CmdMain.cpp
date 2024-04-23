#include "CmdMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HHOOK keyboardLowLevelHook = NULL;
HHOOK noAltF4Hook = NULL;


DWORD CALLBACK CmdMain(void *data) {

}


BOOL DllMain_load_cmd(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hInst);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CmdMain, NULL, 0, NULL);
    return TRUE;
}

BOOL DllMain_unload_cmd (HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
    if(keyboardLowLevelHook != NULL) {
        UnhookWindowsHookEx(keyboardLowLevelHook);
    }

    if (noAltF4Hook != NULL) {
        UnhookWindowsHookEx(noAltF4Hook);
    }

    // this is awfully hacky, lets goooo
    SetSystemCursor(LoadCursorA(NULL, IDC_ARROW), OCR_NORMAL);
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

    return TRUE;
}
