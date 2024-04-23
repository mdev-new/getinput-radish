#include "CmdMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


DWORD CALLBACK CmdMain(void *data) {

}


BOOL DllMain_load_cmd(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hInst);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CmdMain, NULL, 0, NULL);
    return TRUE;
}

BOOL DllMain_unload_cmd (HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {

    return TRUE;
}
