#include "ConhostMain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HHOOK keyboardLowLevelHook = NULL;
HHOOK noAltF4Hook = NULL;

WNDPROC origWindowProc;

short rasterX, rasterY;
short screenX, screenY;


//https://stackoverflow.com/questions/7009080/detecting-full-screen-mode-in-windows
bool isFullscreen(HWND windowHandle)
{
	MONITORINFO monitorInfo = { 0 };
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);

	RECT windowRect;
	GetWindowRect(windowHandle, &windowRect);

	return windowRect.left == monitorInfo.rcMonitor.left
		&& windowRect.right == monitorInfo.rcMonitor.right
		&& windowRect.top == monitorInfo.rcMonitor.top
		&& windowRect.bottom == monitorInfo.rcMonitor.bottom;
}

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


//https://cboard.cprogramming.com/windows-programming/69905-disable-alt-key-commands.html
LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam ) {
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode < 0 || nCode != HC_ACTION )
		goto end;

	if (p->vkCode == VK_RETURN && p->flags & LLKHF_ALTDOWN)
		return 1; //disable alt-enter

	else if (p->vkCode == VK_F11)
		return 1; // disable F11

end:
	return CallNextHookEx(keyboardLowLevelHook, nCode, wParam, lParam );
}

LRESULT CALLBACK DisableAltF4(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode < 0 || nCode != HC_ACTION)
		goto end;

	if (p->vkCode == VK_F4 && p->flags & LLKHF_ALTDOWN)
		return 1; //disable alt-f4

end:
	return CallNextHookEx(keyboardLowLevelHook, nCode, wParam, lParam);
}


void resizeConsoleIfNeeded(int newX, int newY) {
	static int lastX = -1, lastY = -1;

	if (newX > 0 && newY > 0 && (newX != lastX) && (newY != lastY)) {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

		COORD consoleBufferSize = { newX, newY };
		SMALL_RECT windowInfo = { 0, 0, newX - 1, newY - 1 };

		SetConsoleWindowInfo(hOut, TRUE /* this has to be TRUE on Windows 11 */, &windowInfo);
		SetConsoleScreenBufferSize(hOut, consoleBufferSize);

		lastX = screenx;
		lastY = screeny;
	}

	return;
}


DWORD CALLBACK ConhostRpcPipe(LPVOID data) {

}


DWORD CALLBACK ConhostCommands(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\GetinputConhost",
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

			char *word1 = NULL;
			char *word2 = NULL;

			if (strncmp(word1, "SetMousePointer", 13) == 0) {
				if (strcmp(word2, "Hand") == 0) {
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
			}

			else if (strcmp(word1, "SetScreen") == 0) {
			}

			else if (strcmp(word1, "SetQuit") == 0) {
				if (strcmp(word2, "DisableQuit") == 0) {
					noAltF4Hook = SetWindowsHookEx(WH_KEYBOARD_LL, DisableAltF4, GetModuleHandle(NULL), 0);
					DisableCloseButton(hCon);
				}

				else if (strcmp(word2, "EnableQuit") == 0) {
					UnhookWindowsHookEx(noAltF4Hook);
					noAltF4Hook = NULL;
					EnableCloseButton(hCon);
				}
			}

			else if (strcmp(word1, "SetResize") == 0) {
				if (strcmp(word2, "DisableResize") == 0) {
					bool brutalNoResize = /* TODO*/ true;

					DWORD windowStyle = GetWindowLong(hCon, GWL_STYLE);
					windowStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
					SetWindowLong(hCon, GWL_STYLE, windowStyle);
					DrawMenuBar(hCon);

					if (brutalNoResize) {
						keyboardLowLevelHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
					}
				}

				else if (strcmp(word2, "EnableResize") == 0) {
					DWORD windowStyle = GetWindowLong(hCon, GWL_STYLE);
					windowStyle |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
					SetWindowLong(hCon, GWL_STYLE, windowStyle);
					DrawMenuBar(hCon);

					if (brutalNoResize) {
						UnhookWindowsHookEx(keyboardLowLevelHook);
						keyboardLowLevelHook = NULL;
					}
				}
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}

LRESULT WindowProcHook(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
) {
	switch(uMsg) {

		// case resize
		// case maximize
		/*
				if(noresize) {
			if (isFullscreen(hCon)) {
				SendMessage(hCon, WM_SYSCOMMAND, SC_RESTORE, 0);
			}

			resizeConsoleIfNeeded(lastscreenx, lastscreeny);
		}
*/

		default:
			return CallWindowProc(origWindowProc, hwnd, uMsg, wParam, lParam);
	}

}

DWORD CALLBACK ConhostMain(void *data) {

#ifdef MODERN_WINDOWS
	CONSOLE_FONT_INFOEX cfi = {
		.cbSize = sizeof(CONSOLE_FONT_INFOEX),
		.nFont = 0,
		.dwFontSize = {0, 0},
		.FontFamily = FF_DONTCARE,
		.FontWeight = FW_NORMAL,
		.FaceName = L"Terminal"
	};
#endif

	//resizeConsoleIfNeeded();

#ifdef VERY_MODERN_WINDOWS
	SetProcessDpiAwareness(DPI_AWARENESS_UNAWARE);
#endif

	int sleep_time = 1000 / getenvnum_ex("getinput_pollrate", 40);
	unsigned long long begin, took;

	while(1) {
		begin = GetTickCount64();

		SetConsoleMode(hIn, (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE));

#ifdef MODERN_WINDOWS

		short rasterx = getenvnum("getinput_rasterx");
		short rastery = getenvnum("getinput_rastery");
		bool isRaster = rasterx && rastery;

		// lets not set the font every frame
		if (isRaster && (prevRasterX != rasterx || prevRasterY != rastery)) {
			prevRasterX = rasterx;
			prevRasterY = rastery;

			cfi.dwFontSize = { rasterx, rastery };
			SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
		}

#endif

		took = GetTickCount64() - begin;
		Sleep(_max(0, sleep_time - took));
	}

	return NULL;

}


// The routines that get called from DllMain

BOOL DllMain_load_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {

	timeBeginPeriod(1);

	HANDLE hCommandThread = CreateThread(
		NULL,
		0,
		ConhostCommands,
		NULL,
		0,
		NULL
	);

	// i have no idea if this is gonna work
	HANDLE hWndSelf = GetConsoleWindow();
	origWindowProc = SetWindowLong(hWndSelf, GWL_WNDPROC, (LONG_PTR)&WindowProcHook);

	HANDLE hMainThread = CreateThread(
		NULL,
		0,
		ConhostMain,
		NULL,
		0,
		NULL
	);

	return TRUE;
}

BOOL DllMain_unload_conhost(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {

	if(keyboardLowLevelHook != NULL) {
		UnhookWindowsHookEx(keyboardLowLevelHook);
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
