// this code runs on hopes and dreams.
// it was written over the course of 3 years with very varying setups.
// and under varying conditions.
// do not learn from this, nor copy from this.
// this code is a spaghettified mess.
// just the sheer count of #ifdefs talks for itself.
// do not judge me for this. i'm truly sorry.

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define OEMRESOURCE
#include <windows.h>

#include "audio.hpp"

#pragma warning(disable:4838)

//#define NEW_MOUSE

#pragma comment(lib, "user32.lib")

#include <stdio.h>
#include <stdlib.h>

#include <timeapi.h>

#include <vector>
#include <deque>

#define YESHI

//#define WIN2K_BUILD

#ifndef WIN2K_BUILD
#	define ENABLE_CONTROLLER

#	include <atomic>
#	include <thread>
#	pragma comment(lib, "onecore.lib")

using std::atomic_bool;

#else
// lets make it at least volatile
#	define atomic_bool volatile bool
#endif

#ifdef ENABLE_CONTROLLER
#	pragma comment(lib, "XInput9_1_0.lib")
#	include <xinput.h>
#endif

// i am too lazy lmfao
#define ENV SetEnvironmentVariable

// extern "C" void RtlGetNtVersionNumbers(DWORD*, DWORD*, DWORD*);
extern "C" long RtlGetVersion(RTL_OSVERSIONINFOW* lpVersionInformation);

atomic_bool inTextInput = false;

bool inFocus = true;
volatile int sleep_time = 1000;


void resizeConsoleIfNeeded(int *lastScreenX, int *lastScreenY) {
	int screenx = getenvnum("getinput_screenx");
	int screeny = getenvnum("getinput_screeny");
	
	int lastX = *lastScreenX;
	int lastY = *lastScreenY;

	if (screenx && screeny && (screenx != lastX) && (screeny != lastY)) {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

		COORD consoleBufferSize = { screenx, screeny };
		SMALL_RECT windowInfo = { 0, 0, screenx - 1, screeny - 1 };

		SetConsoleWindowInfo(hOut, TRUE /* this has to be TRUE on Windows 11 */, &windowInfo);
		SetConsoleScreenBufferSize(hOut, consoleBufferSize);

		*lastScreenX = screenx;
		*lastScreenY = screeny;
	}

	return;
}

//https://cboard.cprogramming.com/windows-programming/69905-disable-alt-key-commands.html
LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

    if (nCode < 0 || nCode != HC_ACTION )
		goto end;

	if (p->vkCode == VK_RETURN && p->flags & LLKHF_ALTDOWN) return 1; //disable alt-enter
	else if (p->vkCode == VK_F11) return 1;

end:
    return CallNextHookEx(keyboardLowLevelHook, nCode, wParam, lParam );
}

LRESULT CALLBACK DisableAltF4(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode < 0 || nCode != HC_ACTION)
		goto end;

	if (p->vkCode == VK_F4 && p->flags & LLKHF_ALTDOWN) return 1; //disable alt-f4

end:
	return CallNextHookEx(keyboardLowLevelHook, nCode, wParam, lParam);
}

DWORD CALLBACK CommandThread(LPVOID data) {
	HANDLE hPipe = NULL;
	DWORD dwRead = 0;

	const int MAX_CMD_LEN = 512;

	char inbuf[MAX_CMD_LEN] = { 0 };

	hPipe = CreateNamedPipe("\\\\.\\pipe\\getinput",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | FILE_FLAG_FIRST_PIPE_INSTANCE,
		1,
		0,
		MAX_CMD_LEN,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	std::deque<Audio::AudioFile *> queues[12];
	Audio::SndOutStream streams[12];


	while (hPipe != INVALID_HANDLE_VALUE) {
		if (ConnectNamedPipe(hPipe, NULL) != FALSE) {   // wait for someone to connect to the pipe
			while (ReadFile(hPipe, inbuf, MAX_CMD_LEN, &dwRead, NULL) != FALSE) {
				/* add terminating zero */
				if (dwRead >= 1) {
					inbuf[dwRead] = '\0';

					if (strlen(inbuf) > 13 && strncmp(inbuf, "mousePointer ", 13) == 0) {
						//MessageBox(NULL, inbuf + 13, "Got command", 0);
						if (strcmp(inbuf + 13, "hand") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_HAND), OCR_NORMAL);
						}
						if (strcmp(inbuf + 13, "default") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_ARROW), OCR_NORMAL);
                            SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
                        }
						if (strcmp(inbuf + 13, "load") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_WAIT), OCR_NORMAL);
						}
						if (strcmp(inbuf + 13, "unavailable") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_NO), OCR_NORMAL);
						}
						if (strcmp(inbuf + 13, "bg_work") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_APPSTARTING), OCR_NORMAL);
						}
						if (strcmp(inbuf + 13, "text") == 0) {
							SetSystemCursor(LoadCursorA(NULL, IDC_IBEAM), OCR_NORMAL);
						}
					}

					if (strcmp(inbuf, "setRaster") == 0) {
					}

					if (strcmp(inbuf, "setNoResize") == 0) {
					}

					if (strcmp(inbuf, "setBrutalNoReisize") == 0) {
					}

					if (strcmp(inbuf, "setMouseLimits") == 0) {
					}

					if (strcmp(inbuf, "setScreen") == 0) {
					}

				}
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}

DWORD GETINPUT_SUB CALLBACK Process(void*) {
	ENV("getinput_wheeldelta", "0");
	ENV("getinput_mousexpos", "0");
	ENV("getinput_mouseypos", "0");
	ENV("getinput_click", "0");
	ENV("getinputInitialized", "1");
	ENV("getinput_builtOn", __DATE__ " " __TIME__);

	const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	const HWND hCon = GetConsoleWindow();

	BYTE mouseclick;

#ifndef WIN2K_BUILD

	short rasterx, rastery;
	bool isRaster;

	const float deadzone = (float)getenvnum_ex("getinput_deadzone", 24) / 100.f;

	CONSOLE_FONT_INFOEX cfi = {
		.cbSize = sizeof(CONSOLE_FONT_INFOEX),
		.nFont = 0,
		.dwFontSize = {0, 0},
		.FontFamily = FF_DONTCARE,
		.FontWeight = FW_NORMAL,
		.FaceName = L"Terminal"
	};

#ifndef NEW_MOUSE
	int scale = 100, prevScale = scale, roundedScale;
	float fscalex = 1.0f, fscaley = fscalex;
	int mousx, mousy;
	POINT pt;
	CONSOLE_FONT_INFO info = { 0 };
	COORD* fontSz = &info.dwFontSize;
#endif
	WORD prevRasterX = -1;
	WORD prevRasterY = -1;

#ifndef DISABLE_SCALING

	RTL_OSVERSIONINFOW osVersionInfo = { 0 };
	osVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	RtlGetVersion(&osVersionInfo);
	bool bWin10 = osVersionInfo.dwMajorVersion >= 10;

	HRESULT(*GetScaleFactorForMonitorProc)(HMONITOR, int*) = NULL;
	HRESULT(*SetProcessDpiAwarenessProc)(int) = NULL;

	if (bWin10) {
		// for some odd reason loading the library first didn't work
		GetScaleFactorForMonitorProc = (decltype(GetScaleFactorForMonitorProc))GetProcAddress(LoadLibraryA("shcore.dll"), "GetScaleFactorForMonitor");
		SetProcessDpiAwarenessProc = (decltype(SetProcessDpiAwarenessProc))GetProcAddress(LoadLibraryA("shcore.dll"), "SetProcessDpiAwareness");

		if (GetScaleFactorForMonitorProc == NULL || SetProcessDpiAwarenessProc == NULL) {
			bWin10 = FALSE;
			MessageBox(NULL, "Scaling will not work.", "Happy little message", MB_ICONERROR | MB_OK);
		}
		else {
			SetProcessDpiAwarenessProc(DPI_AWARENESS_UNAWARE);
		}
	}

#endif

#endif

	DWORD windowStyle = GetWindowLong(hCon, GWL_STYLE);
	bool noresize = false, brutalNoResize = getenvnum_ex("getinput_brutalNoResize", 0) == 1;

	if ((noresize = (getenvnum("getinput_noresize") == 1)) || brutalNoResize) {
		windowStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hCon, GWL_STYLE, windowStyle);
		DrawMenuBar(hCon);

		if (brutalNoResize) {
			keyboardLowLevelHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
		}
	}

	if (getenvnum("getinput_noquit") == 1) {
		noAltF4Hook = SetWindowsHookEx(WH_KEYBOARD_LL, DisableAltF4, GetModuleHandle(NULL), 0);

        DisableCloseButton(hCon);
    }

	timeBeginPeriod(1);

	sleep_time = 1000 / getenvnum_ex("getinput_pollrate", 40);

#ifdef NEW_MOUSE
	HANDLE hModeThread    = CreateThread(NULL, 0, ModeThread    , NULL, 0, NULL);
	HANDLE hReadThread    = CreateThread(NULL, 0, MousePosThread, NULL, 0, NULL);
    SetThreadPriority(hModeThread, THREAD_PRIORITY_TIME_CRITICAL);
//    SetThreadPriority(hReadThread, THREAD_PRIORITY_TIME_CRITICAL);
#else
    CreateThread(NULL, 0, MouseMessageThread, NULL, 0, NULL);
#endif
	HANDLE hCommandThread = CreateThread(NULL, 0, CommandThread , NULL, 0, NULL);

	unsigned __int64 begin, took;

	int lastscreenx = -1, lastscreeny = -1;
	resizeConsoleIfNeeded(&lastscreenx, &lastscreeny);

    while (TRUE) {
		begin = GetTickCount64();
        SetConsoleMode(hIn, (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE));

#ifndef DISABLE_SCALING
		GetCurrentConsoleFont(hOut, FALSE, &info);
		if (bWin10) {
            HMONITOR monitor = MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST);
            GetScaleFactorForMonitorProc(monitor, &scale);
        }
#endif

#ifndef NEW_MOUSE
        GetPhysicalCursorPos(&pt);
        ScreenToClient(hCon, &pt);
#endif

        mouseclick =
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if (mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick |= mouseclick & 0b11;
		}

#ifndef NEW_MOUSE
		int lmx = getenvnum("getinput_limitMouseX");
		int lmy = getenvnum("getinput_limitMouseY");
		bool bLimitMouse = lmx && lmy;
#endif


#ifndef WIN2K_BUILD

		rasterx = getenvnum("getinput_rasterx");
		rastery = getenvnum("getinput_rastery");
		isRaster = rasterx && rastery;

		// lets not set the font every frame
		if (isRaster && (prevRasterX != rasterx || prevRasterY != rastery)) {
			cfi.dwFontSize = { rasterx, rastery };
			SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
			prevRasterX = rasterx;
			prevRasterY = rastery;
		}

		if (noresize) {
			SetWindowLong(hCon, GWL_STYLE, windowStyle);
			DrawMenuBar(hCon);

			if (isFullscreen(hCon)) {
				SendMessage(hCon, WM_SYSCOMMAND, SC_RESTORE, 0);

			}

#ifndef YESHI
            resizeConsoleIfNeeded(lastscreenx, lastscreeny);
#endif

		}

#ifndef DISABLE_SCALING
        if (bWin10 && prevScale != scale) {
            // this somehow (mostly) works, !!DO NOT TOUCH!!
            if (!isRaster) {
                fscalex = fscaley = (float)(scale) / 100.f; // this probably needs a little tweaking
            }
            else {
                roundedScale = (scale - 100 * (scale / 100));
                if (roundedScale < 50) {
                    fscalex = fscaley = (float)(((scale + 50) * 100) / 10000L);
                }
                else if (roundedScale > 50 && scale % 100 != 0) {
                    fscalex = (scale / 100L) + 1.f;
                    fscaley = (float)(scale - scale % 50) / 100.f;
                }
            }

            prevScale = scale;
        }
#endif

#endif

		inFocus = GetForegroundWindow() == GetConsoleWindow(); // true; //GetForegroundWindow() == GetConsoleWindow();

		if (inFocus) {
			ENV("getinput_click", itoa_(mouseclick));

			//if (!inTextInput) {
				process_keys();
			//}

#ifndef NEW_MOUSE
        mousx = my_ceil((float)pt.x / ((float)fontSz->X * fscalex));
		mousy = my_ceil((float)pt.y / ((float)fontSz->Y * fscaley));

		ENV("getinput_mousexpos", (!bLimitMouse || (bLimitMouse && mousx <= lmx)) ? itoa_(mousx) : NULL);
		ENV("getinput_mouseypos", (!bLimitMouse || (bLimitMouse && mousy <= lmy)) ? itoa_(mousy) : NULL);
#endif

#ifdef ENABLE_CONTROLLER
			PROCESS_CONTROLLER(deadzone);
#endif
		}

		took = GetTickCount64() - begin;
		Sleep(_max(0, sleep_time - took));
	}
}
