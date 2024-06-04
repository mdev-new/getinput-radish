#include "CmdMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>

#include "Mouse.h"
#include "Keyboard.h"

#ifdef MODERN_WINDOWS
#	include "Controller.h"
#	include <atomic>
  using std::atomic_int;
  using std::atomic_float;
#else
#	define atomic_int volatile int
#	define atomic_float volatile float
#endif

atomic_int sleep_time = 1000 / 40;
atomic_float deadzone = 0.24f;

DWORD CALLBACK CmdCommands(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	int pid = GetCurrentProcessId();

	char pipeName[128] = {0};
	strcat(pipeName, "\\\\.\\pipe\\GetinputCmd");
	strcat(pipeName, itoa_(pid));

	hPipe = CreateNamedPipe(
		pipeName,
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		0,
		MAX_CMD_LEN,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL
	);

	while (hPipe != INVALID_HANDLE_VALUE) {
		BOOL pipeConnected = ConnectNamedPipe(hPipe, NULL);
		if (pipeConnected == FALSE) continue;

		while (ReadFile(hPipe, inbuf, MAX_CMD_LEN, &dwRead, NULL) != FALSE) {
			if (dwRead <= 0) continue;

			/* add terminating zero */
			inbuf[dwRead] = '\0';

			if (strcmp(inbuf, "setMouseLimits") == 0) {
			}

			else if (strcmp(inbuf, "setCtrlDeadzone") == 0) {
			}

			else if (strcmp(inbuf, "setPollRate") == 0) {
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}


DWORD CALLBACK CmdMain(void *data) {

	BOOL inFocus = FALSE;

	unsigned long long begin, took;

	while(1) {
		begin = GetTickCount64();

		inFocus = GetForegroundWindow() == GetConsoleWindow();

		if(inFocus) {
			MouseUpdate();
			KeyboardUpdate();

#ifdef MODERN_WINDOWS
			ControllerUpdate(deadzone);
#endif

		}

		took = GetTickCount64() - begin;
		Sleep(_max(0, sleep_time - took));
	}

	return NULL;

}

// The routines that get called from DllMain

BOOL DllMain_load_cmd(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	timeBeginPeriod(1);

	MouseInit();

	HANDLE hCommandThread = CreateThread(
		NULL,
		0,
		CmdCommands,
		NULL,
		0,
		NULL
	);

	SetEnvironmentVariable("getinputInitialized", "1");

	HANDLE hMainThread = CreateThread(
		NULL,
		0,
		CmdMain,
		NULL,
		0,
		NULL
	);

	return TRUE;
}

BOOL DllMain_unload_cmd(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {

	MouseDeinit();
	KeyboardDeinit();

	timeEndPeriod();

	return TRUE;
}
