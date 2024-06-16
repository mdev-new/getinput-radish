#include "Injector.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>

#ifndef MODERN_WINDOWS
volatile HMODULE hDll = NULL;
#endif

#include "CmdMain.h"
#include "ConhostMain.h"

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

			if(hProc == INVALID_HANDLE_VALUE) {
				continue;
			}

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

#ifdef MODERN_WINDOWS
	HMODULE hDll = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&getppid,
		&hDll
	);
#else
	// For Win2k, just do a spin loop until the hDll is not written to...
	// It's ugly, but works... (sometimes)

	while (hDll == NULL);
	Sleep(250); //delay x amount of ms to ensure it was REALLY written to
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
#ifndef MODERN_WINDOWS
	hDll = hInst;
#endif

	char name[MAX_PATH];
	GetModuleFileName(NULL, name, sizeof(name));

	bool isRunDll = lstrcmp("rundll32.exe", name + lstrlen(name) - 12) == 0;

#ifdef MODERN_WINDOWS
	bool isConhost = lstrcmp("conhost.exe", name + lstrlen(name) - 11) == 0;
#endif

	if (isRunDll) {
		// Exit early if we're running inside RunDLL32
		// we have no work to do here
		return TRUE;
	}

	if (dwReason == DLL_PROCESS_ATTACH) {

		DisableThreadLibraryCalls(hInst);

#ifdef MODERN_WINDOWS

		if (isConhost) {
			return DllMain_load_conhost(hInst, lpReserved);
		}
		else {

			DWORD conhostPid = getenvnum_ex("#getinputInternal#conhostPid", 0);

			// TODO !!!: race condition with the sockets

			// If we haven't injected to conhost yet, atleast in the current env
			// the logic is so that every child process that inherits from the
			// process that has injected should in turn inherit this "magic"
			// env var
			if(conhostPid == 0) {
				// We're not in conhost, so we inject ourselves into it...
				conhostPid = getconhost();
				SetEnvironmentVariable("#getinputInternal#conhostPid", itoa_(conhostPid))
				hookSelfToProcess(conhostPid);
			}

			//...and then start our work
			return DllMain_load_cmd(hInst, lpReserved);
		}

#else

		// We create the process that will provide us the IPC
		// and audio etc

		CreateProcess(

		);


		return DllMain_load_cmd(hInst, lpReserved);
#endif

	} else if(dwReason == DLL_PROCESS_DETACH) {

#ifdef MODERN_WINDOWS

		if (isConhost) {
			return DllMain_unload_conhost(hInst, lpReserved);
		}
		else {
			return DllMain_unload_cmd(hInst, lpReserved);
		}

#else
		return DllMain_unload_cmd(hInst, lpReserved);
#endif

	}
	return TRUE;
}

NOMANGLE __declspec(dllexport) void CALLBACK hook(
	HWND hwnd,
	HINSTANCE hinst,
	LPSTR lpszCmdLine,
	int nCmdShow
) {
	hookSelfToProcess(getppid());
	return;
}

NOMANGLE __declspec(dllexport) void CALLBACK version(
	HWND hwnd,
	HINSTANCE hinst,
	LPSTR lpszCmdLine,
	int nCmdShow
) {
	MessageBox(0, __DATE__ __TIME__, "Build date and time", 0);
	return;
}
