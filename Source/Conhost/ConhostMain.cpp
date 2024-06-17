#include "ConhostMain.h"

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <timeapi.h>

#ifdef VERY_MODERN_WINDOWS
#	include <shellscalingapi.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "Global.h"

#ifndef MODERN_WINDOWS
HHOOK noFullscreenHook = NULL;
HHOOK noAltF4Hook = NULL;
#endif


WNDPROC origWindowProc;

enum class QuitDisable {
	QUIT_ENABLED = 0, // 0b00
	QUIT_DISABLED_SOFT = 1, // 0b01
	QUIT_DISABLED_HARD = 3  // 0b11
};

enum class ResizeDisable {
	RESIZE_ENABLED = 0, // 0b00
	RESIZE_DISABLED = 1, // 0b01
	RESIZE_FULLSCREEN_DISABLED = 3  // 0b11
};

short rasterX, rasterY;
short screenX, screenY;
short mLimitX, mLimitY;
ResizeDisable resize;
QuitDisable noQuit;
CONSOLE_FONT_INFOEX base_font;
CONSOLE_FONT_INFOEX current_font;


void DisableCloseButton(HWND hwnd)
{
	EnableMenuItem(
		GetSystemMenu(hwnd, FALSE),
		SC_CLOSE,
		MF_BYCOMMAND | MF_DISABLED | MF_GRAYED
	);
}

void EnableCloseButton(HWND hwnd)
{
	EnableMenuItem(
		GetSystemMenu(hwnd, FALSE),
		SC_CLOSE,
		MF_BYCOMMAND | MF_ENABLED
	);
}

#ifndef MODERN_WINDOWS

// Todo: handle in WindowProc
//https://cboard.cprogramming.com/windows-programming/69905-disable-alt-key-commands.html
LRESULT CALLBACK DisableFullscreenKeys( int nCode, WPARAM wParam, LPARAM lParam ) {
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode < 0 || nCode != HC_ACTION)
		goto end;

	if (p->vkCode == VK_RETURN && p->flags & LLKHF_ALTDOWN)
		return 1; //disable alt-enter

	else if (p->vkCode == VK_F11)
		return 1; // disable F11

end:
	return CallNextHookEx(noFullscreenHook, nCode, wParam, lParam );
}

// Todo: handle in WindowProc
LRESULT CALLBACK DisableAltF4(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode < 0 || nCode != HC_ACTION)
		goto end;

	if (p->vkCode == VK_F4 && p->flags & LLKHF_ALTDOWN)
		return 1; //disable alt-f4

end:
	return CallNextHookEx(noFullscreenHook, nCode, wParam, lParam);
}

#endif


DWORD CALLBACK IpcClientHandler(LPVOID data) {

	SOCKET ClientSocket = (SOCKET)data;

	int iResult, iSendResult;
	const int recvbuflen = 512;

	char buffer[512] = {0}; // log buffer for sprintf
	char recvbuf[recvbuflen] = {0};

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0) {
			// printf("Connection closing...\n");
		} else {
			sprintf(buffer, "recv failed with error: %d\n", WSAGetLastError());
			MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);

	// we're done ; shutdown the connection
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		sprintf(buffer, "shutdown failed with error: %d", WSAGetLastError());
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;

}


DWORD CALLBACK ConhostIPC(LPVOID data) {
	// I hate Windows sockets.

	WSADATA wsaData;
	WORD winsock_version = MAKEWORD(2,2);
	int iResult;
	struct addrinfo *address_info = NULL;
	struct addrinfo hints;
	char buffer[512] = {0}; // log buffer for sprintf

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	HANDLE hClientHandler = INVALID_HANDLE_VALUE;

	// Initialize Winsock
	iResult = WSAStartup(winsock_version, &wsaData);
	if (iResult != 0) {
		sprintf(buffer, "WSAStartup failed: %d", iResult);
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		return 1;
	}

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, IPC_PORT, &hints, &address_info);
	if (iResult != 0) {
		sprintf(buffer, "getaddrinfo failed: %d", iResult);
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for the server to listen for client connections.
	ListenSocket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		sprintf(buffer, "socket failed with error: %ld", WSAGetLastError());
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		freeaddrinfo(address_info);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, address_info->ai_addr, (int)address_info->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		sprintf(buffer, "bind failed with error: %d", WSAGetLastError());
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		freeaddrinfo(address_info);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// We don't need the address info anymore.
	freeaddrinfo(address_info);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		sprintf(buffer, "Listen failed with error: %ld", WSAGetLastError());
		MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	for(;;) {
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			sprintf(buffer, "accept failed with error: %d", WSAGetLastError());
			MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Todo save handles to some sort of a map
		hClientHandler = CreateThread(
			NULL,
			0,
			IpcClientHandler,
			(LPVOID)ClientSocket,
			0,
			NULL
		);

		if(hClientHandler == INVALID_HANDLE_VALUE) {
			sprintf(buffer, "Cannot handle client: Cannot create thread.");
			MessageBox(0, buffer, "WinSock error", MB_ICONERROR);
		} else {
			// Add the thread handle to some list of threads
		}

	}

	closesocket(ListenSocket);

	return 0;
}


DWORD CALLBACK ConhostCommands(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HWND hCon = GetConsoleWindow();

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	int pid = GetCurrentProcessId();

	char pipeName[128] = {0};
	sprintf(pipeName, "\\\\.\\pipe\\BnConhost%d", pid);

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

			if (strncmp(word1, "SetMousePointer", 13) == 0) {
				// Todo try doing this without setting the system cursor...
				if (strcmp(word2, "Hand") == 0) {
					//SetCursor(LoadCursor(NULL, IDC_HAND));
					SetSystemCursor(LoadCursorA(NULL, IDC_HAND), OCR_NORMAL);
				}
				else if (strcmp(word2, "Default") == 0) {
					SetSystemCursor(LoadCursorA(NULL, IDC_ARROW), OCR_NORMAL);
					SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
				}
				else if (strcmp(word2, "Load") == 0) {
					SetSystemCursor(LoadCursorA(NULL, IDC_WAIT), OCR_NORMAL);
				}
				else if (strcmp(word2, "Unavailable") == 0) {
					SetSystemCursor(LoadCursorA(NULL, IDC_NO), OCR_NORMAL);
				}
				else if (strcmp(word2, "BgWork") == 0) {
					SetSystemCursor(LoadCursorA(NULL, IDC_APPSTARTING), OCR_NORMAL);
				}
				else if (strcmp(word2, "Text") == 0) {
					SetSystemCursor(LoadCursorA(NULL, IDC_IBEAM), OCR_NORMAL);
				}
			}

			else if (strcmp(word1, "SetRaster") == 0) {

#ifdef MODERN_WINDOWS

				short rasterx = atoi(word2);
				short rastery = atoi(word3);

				if(rasterx && rastery) {
					current_font.nFont = 0;
					current_font.dwFontSize = { rasterx, rastery };
					current_font.FontFamily = FF_DONTCARE;
					current_font.FontWeight = FW_NORMAL;
					current_font.FaceName = L"Terminal";
					SetCurrentConsoleFontEx(hOut, FALSE, &current_font);					
				}

#else

#endif

			}

			else if (strcmp(word1, "SetScreen") == 0) {
				screenX = atoi(word2);
				screenY = atoi(word3);

				COORD consoleBufferSize = { screenX, screenY };
				SMALL_RECT windowInfo = { 0, 0, screenX - 1, screenY - 1 };

				SetConsoleWindowInfo(hOut, TRUE, &windowInfo);
				SetConsoleScreenBufferSize(hOut, consoleBufferSize);
			}

			else if (strcmp(word1, "SetQuit") == 0) {
				if (strcmp(word2, "DisableQuit") == 0 && noQuit == QuitDisable::QUIT_ENABLED) {
					noAltF4Hook = SetWindowsHookEx(WH_KEYBOARD_LL, DisableAltF4, GetModuleHandle(NULL), 0);
					DisableCloseButton(hCon);
					noQuit = true;
				}

				else if (strcmp(word2, "EnableQuit") == 0 && noQuit != QuitDisable::QUIT_ENABLED) {
					UnhookWindowsHookEx(noAltF4Hook);
					noAltF4Hook = NULL;
					EnableCloseButton(hCon);
					noQuit = false;
				}
			}

			else if (strcmp(word1, "SetResize") == 0) {
				if (strcmp(word2, "DisableResize") == 0) {
					bool brutalNoResize = /* TODO*/ true;

					DWORD windowStyle = GetWindowLong(hCon, GWL_STYLE);
					windowStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
					SetWindowLong(hCon, GWL_STYLE, windowStyle);
					DrawMenuBar(hCon);

#ifndef MODERN_WINDOWS
					noFullscreenHook = SetWindowsHookEx(WH_KEYBOARD_LL, DisableFullscreenKeys, GetModuleHandle(NULL), 0);
#endif
				}

				else if (strcmp(word2, "EnableResize") == 0) {
					DWORD windowStyle = GetWindowLong(hCon, GWL_STYLE);
					windowStyle |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
					SetWindowLong(hCon, GWL_STYLE, windowStyle);
					DrawMenuBar(hCon);

#ifndef MODERN_WINDOWS
					UnhookWindowsHookEx(noFullscreenHook);
					noFullscreenHook = NULL;
#endif
				}
			}

			else if (strcmp(word1, "SetCodepage") == 0) {
				short output = atoi(word2);
				short input = atoi(word3);

				//SetConsoleOutputCodepage

				if(input) {
					//SetConsoleOutputCodepage

				}
			}

			else if (strcmp(word1, "Troll") == 0) {
				SetCursorPos(69, 420);
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}

#ifdef MODERN_WINDOWS

LRESULT WindowProcHook(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
) {
	switch(uMsg) {

		// Todo handle SC_MAXIMIZE etc
		// (if not handled already within WM_WINDOWPOSCHANGED)
		// and maybe WM_SIZE
		// Why the fuck does Windows have so many ways of handling the
		// same thing??????

		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED: {
			if(resize != ResizeDisable::RESIZE_DISABLED) {
				WINDOWPOS *wnd_pos = (PWINDOWPOS)lParam;
				wnd_pos->flags |= SWP_NOSIZE;
				return 0;
			}

			break;
		}

		case WM_CLOSE: {
			if(noQuit != QuitDisable::QUIT_ENABLED) {
				// todo alert batch code that the user wants to quit
				// probably via pipe
				return 0;
			}

			break;
		}

		default: break;
	}

	return CallWindowProc(origWindowProc, hwnd, uMsg, wParam, lParam);

}

#endif

DWORD CALLBACK ConhostMain(void *data) {

	int sleep_time = 1000 / 40;
	unsigned long long begin, took;

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

	while(1) {
		begin = GetTickCount64();

		SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS & ~(ENABLE_QUICK_EDIT_MODE));

		took = GetTickCount64() - begin;
		Sleep(max(0, sleep_time - took));
	}

	return NULL;

}


// The routines that get called from DllMain

BOOL DllMain_load_conhost(HINSTANCE hInst, LPVOID lpReserved) {

	timeBeginPeriod(1);

	ZeroMemory(&current_font, sizeof(CONSOLE_FONT_INFOEX));
	current_font.cbSize = sizeof(CONSOLE_FONT_INFOEX);

#ifdef VERY_MODERN_WINDOWS
	SetProcessDpiAwareness(DPI_AWARENESS_UNAWARE);
#endif

	GetCurrentConsoleFontEx(hOut, FALSE, &base_font);

	HANDLE hCommandThread = CreateThread(
		NULL,
		0,
		ConhostCommands,
		NULL,
		0,
		NULL
	);

#ifdef MODERN_WINDOWS
	// Detour the WindowProc
	// TODO: Should it be just WindowProcHook or &WindowProcHook

	// i have no idea if this is gonna work
	HANDLE hWndSelf = GetConsoleWindow();
	origWindowProc = SetWindowLong(hWndSelf, GWL_WNDPROC, (LONG_PTR)&WindowProcHook);

#endif

	HANDLE hMainThread = CreateThread(
		NULL,
		0,
		ConhostMain,
		NULL,
		0,
		NULL
	);

	SetEnvironmentVariable("getinput_initialized", "1");

	return TRUE;
}

BOOL DllMain_unload_conhost(HINSTANCE hInst, LPVOID lpReserved) {

	if(noFullscreenHook != NULL) {
		UnhookWindowsHookEx(noFullscreenHook);
	}

	if (noAltF4Hook != NULL) {
		UnhookWindowsHookEx(noAltF4Hook);
	}

	// this is awfully hacky, lets goooo
	SetSystemCursor(LoadCursorA(NULL, IDC_ARROW), OCR_NORMAL);
	SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

	timeEndPeriod();

	return TRUE;
}

// Let's make this shit an EXE!!!

#ifndef MODERN_WINDOWS

// This is the part that makes this an EXE.

static HINSTANCE hInstance;

BOOL WINAPI ExitHandler(DWORD dwCtrlType) {

	if(
		dwCtrlType == CTRL_CLOSE_EVENT    ||
		dwCtrlType == CTRL_LOGOFF_EVENT   ||
		dwCtrlType == CTRL_SHUTDOWN_EVENT
	) {
		DllMain_unload_conhost(hInstance, NULL);
		return TRUE;
	}

	return FALSE;

}

INT WINAPI WinMain(
	HWND hwnd,
	HINSTANCE hinst,
	LPSTR lpszCmdLine,
	int nCmdShow
) {
	hInstance = hinst;

	SetConsoleCtrlHandler(ExitHandler,TRUE);
}

#endif
