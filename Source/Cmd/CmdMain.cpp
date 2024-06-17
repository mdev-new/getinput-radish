#include "CmdMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>

#include "Mouse.h"
#include "Keyboard.h"
#include "Global.h"
#include "Utilities.h"

#ifdef MODERN_WINDOWS
#	include "Controller.h"
#	include <atomic>
	typedef std::atomic<float> atomic_float;
	typedef std::atomic<int> atomic_int;
#else
#	define atomic_int volatile int
#	define atomic_float volatile float
#endif

atomic_int sleep_time = 1000 / 40;
atomic_float deadzone = 0.24f;

SOCKET ipc_socket = INVALID_SOCKET;

DWORD CALLBACK CmdCommands(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	int pid = GetCurrentProcessId();

	char pipeName[128] = {0};
	sprintf(pipeName, "\\\\.\\pipe\\BnCmd%d", pid);

	hPipe = CreateNamedPipe(
		pipeName,
		PIPE_ACCESS_INBOUND, // todo both way pipe
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

			char *word1 = NULL;
			char *word2 = NULL;
			char *word3 = NULL; // todo make this optional

			// todo more rigid arg parsing
			sscanf(inbuf, "%s %s %s ", word1, word2, word3);

			if (strcmp(inbuf, "SetMouseLimits") == 0) {
			}

			else if (strcmp(inbuf, "SetDeadzone") == 0) {
			}

			else if (strcmp(inbuf, "SetPollRate") == 0) {
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}


DWORD CALLBACK CmdMain(void *data) {

	BOOL inFocus = FALSE;

	int sleep_time = 1000 / getenvnum_ex("bn_pollrate", 40);
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
		Sleep(max(0, sleep_time - took));
	}

	return NULL;

}


SOCKET InitSocket(const char *address, const char *port) {
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	WORD winsock_version = MAKEWORD(2,2);

	// Initialize Winsock
	iResult = WSAStartup(winsock_version, &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(address, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	do {
		// Attempt to connect to an address until one succeeds
		for(ptr=result; ptr != NULL; ptr=ptr->ai_next) {

			// Create a SOCKET for connecting to server
			ConnectSocket = socket(
				ptr->ai_family,
				ptr->ai_socktype,
				ptr->ai_protocol
			);

			if (ConnectSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				return 1;
			}

			// Connect to server.
			iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR) {
				break;
			} else {
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue;
			}
		}
	} while(ConnectSocket == INVALID_SOCKET);

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	return ConnectSocket;

}


// The routines that get called from the real DllMain

BOOL DllMain_load_cmd(HINSTANCE hInst, LPVOID lpReserved) {
	timeBeginPeriod(1);

	MouseInit();
	ipc_socket = InitSocket("127.0.0.1", IPC_PORT);

	HANDLE hCommandThread = CreateThread(
		NULL,
		0,
		CmdCommands,
		NULL,
		0,
		NULL
	);

	HANDLE hMainThread = CreateThread(
		NULL,
		0,
		CmdMain,
		NULL,
		0,
		NULL
	);

	SetEnvironmentVariable("bn_initialized", "1");
	SetEnvironmentVariable("bn_pipeid", itoa_(GetCurrentProcessId()));

	return TRUE;
}

BOOL DllMain_unload_cmd(HINSTANCE hInst, LPVOID lpReserved) {

	timeEndPeriod(1);

	MouseDeinit();
	KeyboardDeinit();

	closesocket(ipc_socket);
	ipc_socket = INVALID_SOCKET;
	WSACleanup();

	return TRUE;
}
