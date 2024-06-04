#include "Mouse.h"
#include "Utilities.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
	short wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

	if (wheelDelta != 0) {
		wheelDelta /= WHEEL_DELTA;
		int oldWheelDelta = getenvnum("getinput_wheeldelta");
		SetEnvironmentVariable("getinput_wheeldelta", itoa_(oldWheelDelta + wheelDelta));
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD CALLBACK MouseMessageThread(void* data) {
	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void MouseInit() {
	SetEnvironmentVariable("getinput_wheeldelta", "0");
	SetEnvironmentVariable("getinput_mousexpos", "0");
	SetEnvironmentVariable("getinput_mouseypos", "0");
	SetEnvironmentVariable("getinput_click", "0");

	HANDLE hMouseMsgThread = CreateThread(
		NULL,
		0,
		MouseMessageThread,
		NULL,
		0,
		NULL
	);
}

void MouseUpdate() {

#ifdef VERY_MODERN_WINDOWS
	static HMONITOR monitor;
#endif

	static POINT pt;
	static BYTE buttons;
	static int mouseX, mouseY;
	static int scale = 100, prevScale = scale;
	static float fscalex = (float)(scale) / 100.f, fscaley = fscalex;
	static CONSOLE_FONT_INFO info = { 0 };
	static const COORD* fontSz = &info.dwFontSize;

	static const HANDLE hCon = GetConsoleWindow();
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Mouse buttons
	buttons =
		(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
		(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
		(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

	// Handle "invert mouse buttons" setting
	buttons |= (buttons & 0b11) *
						 (buttons && GetSystemMetrics(SM_SWAPBUTTON));

	SetEnvironmentVariable("getinput_click", itoa_(buttons));

	// Cursor position
	GetPhysicalCursorPos(&pt);
	ScreenToClient(hCon, &pt);

	GetCurrentConsoleFont(hOut, FALSE, &info);

#ifdef VERY_MODERN_WINDOWS

	monitor = MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST);
	GetScaleFactorForMonitorProc(monitor, &scale);

	if (prevScale != scale) {
		// this somehow (mostly) works, !!DO NOT TOUCH!!
		if (!isRaster) {
			// this probably needs a little tweaking
			fscalex = fscaley = (float)(scale) / 100.f;
		}
		else {
			int roundedScale = (scale - 100 * (scale / 100));

			// what if scale == _50????
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

	mouseX = int_ceil((float)pt.x / ((float)fontSz->X * fscalex));
	mouseY = int_ceil((float)pt.y / ((float)fontSz->Y * fscaley));

	// Todo lower limit in addition to this current upper
	int lmx = getenvnum("getinput_limitMouseX");
	int lmy = getenvnum("getinput_limitMouseY");
	bool bLimitMouse = lmx && lmy;

	SetEnvironmentVariable(
		"getinput_mousexpos",
		(bLimitMouse && mouseX > lmx) ? NULL : itoa_(mouseX)
	);

	SetEnvironmentVariable(
		"getinput_mouseypos",
		(bLimitMouse && mouseY > lmy) ? NULL : itoa_(mouseY)
	);

}
