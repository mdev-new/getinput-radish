#include "ConhostMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

DWORD CALLBACK ConhostMain(void *data) {

}

BOOL DllMain_load_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hInst);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ConhostMain, NULL, 0, NULL);
    return TRUE;
}

BOOL DllMain_unload_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {

    return TRUE;
}
