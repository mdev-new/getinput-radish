#include "Injector.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>

#ifdef WIN2K_BUILD
volatile HMODULE hDll = NULL;
#endif

DWORD getppid() {
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	DWORD ppid = 0, pid = GetCurrentProcessId();

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) goto cleanup;

	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);
	if (!Process32First(hSnapshot, &pe32)) goto cleanup;

	do {
		if (pe32.th32ProcessID == pid) {
			ppid = pe32.th32ParentProcessID;
			break;
		}
	} while (Process32Next(hSnapshot, &pe32));

cleanup:
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return ppid;
}

DWORD getconhost() {
	HANDLE hSnapshot, hProc, hSelf;
	PROCESSENTRY32 pe32;
	SYSTEMTIME sConhostTime, sSelfTime;
	FILETIME fProcessTime, ftExit, ftKernel, ftUser;
	DWORD pid = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) goto cleanup;

	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);
	if (!Process32First(hSnapshot, &pe32)) goto cleanup;

	hSelf = GetCurrentProcess();
	GetProcessTimes(hSelf, &fProcessTime, &ftExit, &ftKernel, &ftUser);
	FileTimeToSystemTime(&fProcessTime, &sSelfTime);

	do {
		if (lstrcmp(pe32.szExeFile, "conhost.exe") == 0) {
			hProc = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pe32.th32ProcessID);
			GetProcessTimes(hProc, &fProcessTime, &ftExit, &ftKernel, &ftUser);
			FileTimeToSystemTime(&fProcessTime, &sConhostTime);
			CloseHandle(hProc);

            // todo maybe check milliseconds
			if (
                    sConhostTime.wSecond == sSelfTime.wSecond &&
                sConhostTime.wMinute == sSelfTime.wMinute &&
            sConhostTime.wHour == sSelfTime.wHour &&
            sConhostTime.wDay == sSelfTime.wDay &&
            sConhostTime.wMonth == sSelfTime.wMonth &&
            sConhostTime.wYear == sSelfTime.wYear
                ) {
				pid = pe32.th32ProcessID;
				break;
			}
		}
	} while (Process32Next(hSnapshot, &pe32));

cleanup:
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return pid;
}

void hookSelfToProcess(DWORD pid) {
	char filename[MAX_PATH];

#ifdef WIN2K_BUILD
    // For Win2k, just do a spin loop until the hDll is not written to...
    // It's ugly, but works... (sometimes)

	while (hDll == NULL);
	Sleep(250); //delay to ensure it was REALLY written to
#else
	HMODULE hDll = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&getppid,
		&hDll
	);
#endif

	if (!GetModuleFileName(hDll, filename, MAX_PATH)) return;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	int filename_len = lstrlen(filename);

	LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, filename_len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProcess, lpBaseAddress, filename, filename_len, NULL);

	LPTHREAD_START_ROUTINE startAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, startAddr, lpBaseAddress, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return;
}

NOMANGLE __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
#ifdef WIN2K_BUILD
	hDll = hInst;
#endif

	char name[MAX_PATH];
	GetModuleFileName(NULL, name, sizeof(name));
	if (lstrcmp("rundll32.exe", name + lstrlen(name) - 12) == 0) {
		// MessageBoxA(0, "RUNDLL\n", "Window", 0);
		return TRUE;
	}

	if (dwReason == DLL_PROCESS_ATTACH) {

		if (lstrcmp("conhost.exe", name + lstrlen(name) - 11) == 0) {
			return DllMain_conhost(hInst, dwReason, lpReserved);
		}
		else {
			DWORD conhostPid = getconhost();
			hookSelfToProcess(conhostPid);

			return DllMain_cmd(hInst, dwReason, lpReserved);
		}
	} else if(dwReason == DLL_PROCESS_DETACH) {
		if (lstrcmp("conhost.exe", name + lstrlen(name) - 11) == 0) {
			return DllMain_unload_conhost(hInst, dwReason, lpReserved);
		}
		else {
			return DllMain_unload_cmd(hInst, dwReason, lpReserved);
		}
	}
	return TRUE;
}

NOMANGLE __declspec(dllexport) void CALLBACK inject(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	hookSelfToProcess(getppid());
	return;
}

NOMANGLE __declspec(dllexport) void CALLBACK version(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	char buffer[128] = { 0 };
	sprintf(buffer, "%s %s\n", __DATE__, __TIME__);
	MessageBox(0, buffer, "Build date and time", 0);
	return;
}
