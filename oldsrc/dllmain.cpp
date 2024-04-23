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

#define INJECTOR_EXIT_ROUTINE_EXISTS

HHOOK keyboardLowLevelHook = NULL;
HHOOK noAltF4Hook = NULL;

void DllUnloadRoutine(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	if(keyboardLowLevelHook != NULL) {
		UnhookWindowsHookEx(keyboardLowLevelHook);
	}

	if (noAltF4Hook != NULL) {
		UnhookWindowsHookEx(noAltF4Hook);
	}

    // this is awfully hacky, lets goooo
    SetSystemCursor(LoadCursorA(NULL, IDC_ARROW), OCR_NORMAL);
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
}

#include "Utilities.h"

#define GETINPUT_SUB __declspec(noinline)

int GETINPUT_SUB my_ceil(float num) {
	int a = num;
	return ((float)a != num) ? a + 1 : a;
}

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

inline int _max(int a, int b) {
	return ((a > b) ? a : b);
}

atomic_bool inTextInput = false;

bool inFocus = true;
volatile int sleep_time = 1000;

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

const char *stringifiedVKs[] = {
	"Undefined",
	"Left mouse button",
	"Right mouse button",
	"Control-break processing",
	"Middle mouse button",
	"X1 mouse button",
	"X2 mouse button",
	"Reserved",
	"BACKSPACE",
	"TAB",
	"Reserved",
	"Reserved",
	"CLEAR",
	"ENTER",
	"Unassigned",
	"Unassigned",
	"SHIFT",
	"CTRL",
	"ALT",
	"PAUSE",
	"CAPS LOCK",
	"IME Kana mode",
	"IME Hangul mode",
	"IME On",
	"IME Junja mode",
	"IME final mode",
	"IME Hanja mode",
	"IME Kanji mode",
	"IME Off",
	"ESC",
	"IME convert",
	"IME nonconvert",
	"IME accept",
	"IME mode change request",
	"SPACEBAR",
	"PAGE UP",
	"PAGE DOWN",
	"END",
	"HOME",
	"LEFT ARROW",
	"UP ARROW",
	"RIGHT ARROW",
	"DOWN ARROW",
	"SELECT",
	"PRINT",
	"EXECUTE",
	"PRINT SCREEN",
	"INS",
	"DEL",
	"HELP",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"Left Windows",
	"Right Windows",
	"Applications",
	"Reserved",
	"Sleep",
	"Numericpad 0",
	"Numericpad 1",
	"Numericpad 2",
	"Numericpad 3",
	"Numericpad 4",
	"Numericpad 5",
	"Numericpad 6",
	"Numericpad 7",
	"Numericpad 8",
	"Numericpad 9",
	"Multiply",
	"Add",
	"Separator",
	"Subtract",
	"Decimal",
	"Divide",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21",
	"F22",
	"F23",
	"F24",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"NUM LOCK",
	"SCROLL LOCK",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Left SHIFT",
	"Right SHIFT",
	"Left CONTROL",
	"Right CONTROL",
	"Left ALT",
	"Right ALT",
	"Browser Back",
	"Browser Forward",
	"Browser Refresh",
	"Browser Stop",
	"Browser Search",
	"Browser Favorites",
	"Browser Start and Home",
	"Volume Mute",
	"Volume Down",
	"Volume Up",
	"Next Track",
	"Previous Track",
	"Stop Media",
	"Play/Pause Media",
	"Start Mail",
	"Select Media",
	"Start Application 1",
	"Start Application 2",
	"Reserved",
	"Reserved",
	";:",
	"+",
	",",
	"-",
	".",
	"/?",
	"`~",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"[{",
	"\\|",
	"]}",
	"'\"",
	"Misc.",
	"Reserved",
	"OEM specific",
	"The <>s on the US standardboard, or the \\| on the non-US 102-keyboard",
	"OEM specific",
	"OEM specific",
	"IME PROCESS",
	"OEM specific",
	"Passing unicode ass",
	"Unassigned",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"OEM specific",
	"Attn",
	"CrSel",
	"ExSel",
	"Erase EOF",
	"Play",
	"Zoom",
	"Reserved",
	"PA1",
	"Clear",
};

// i was way too lazy to check for values individually, so this was created
// this technically disallows code 0xFF in some cases but once that becomes a problem
// i'll a) not care or b) not be maintaing this or c) will solve it (last resort)
BYTE conversion_table[256] = {
	//        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/* 0 */  -1, -1,  0,  0, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0, // exclude mouse buttons
	/* 1 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 2 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 3 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 4 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 5 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 6 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 7 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 8 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 9 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* A */  -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // exclude right/left ctrl,shift,etc.
	/* B */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* C */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* D */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* E */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* F */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

VOID GETINPUT_SUB process_keys() {
	static BYTE pressed[0x100] = { 0 };

	// 104 (# ofs on a standardboard) * 4 (max amount of chars emmited per (-123)) + 1 (ending dash)
	static char buffer[104 * 4 + 1];
	static char buffer1[0xff * 0xff + 1];

	int isAnyKeyDown = 0, actionHappened = 0;
	BOOL state = 0;
	WORD idx = 1, idx2 = 1;

	for (int i = 3; i < 0x100; ++i) {
		state = GetAsyncKeyState(i);

		if (!pressed[i] && (state & 0x8000) && conversion_table[i] != (BYTE)(-1)) {
			pressed[i] = TRUE;
			actionHappened = TRUE;
		}

		if (pressed[i] && !state) {
			pressed[i] = FALSE;
			actionHappened = TRUE;
		}

		isAnyKeyDown |= pressed[i];
	}

	if (!actionHappened) return;
	if (!isAnyKeyDown) {
        SetEnvironmentVariable("getinput_keyspressed", NULL);
        SetEnvironmentVariable("getinput_keyspressed_str", NULL);
		return;
	}

	ZeroMemory(buffer, sizeof(buffer));
	ZeroMemory(buffer1, sizeof(buffer1));
	buffer[0] = '-';
	buffer1[0] = '-';

	for (int i = 0; i < 0x100; ++i) {
		if (!pressed[i]) continue;

		int num = (conversion_table[i] != 0) ? conversion_table[i] : i;
		if (num == (BYTE)(-1)) continue;

		idx += sprintf(buffer + idx, "%d-", num);
		idx2 += sprintf(buffer1 + idx2, "%s-", stringifiedVKs[num]);
	}

	SetEnvironmentVariable("getinput_keyspressed", buffer);
	SetEnvironmentVariable("getinput_keyspressed_str", buffer1);
}

#ifdef ENABLE_CONTROLLER

typedef struct _controller_value {
	WORD bitmask;
	const char* str;
} controller_value;

controller_value ctrl_values[] = {
	{ 0x0001, "up" },
	{ 0x0002, "down" },
	{ 0x0004, "left" },
	{ 0x0008, "right" },
	{ 0x0010, "start" },
	{ 0x0020, "back" },
	{ 0x0040, "lthumb" },
	{ 0x0080, "rthumb" },
	{ 0x0100, "lshouldr" },
	{ 0x0200, "rshouldr" },
	{ 0x1000, "a" },
	{ 0x2000, "b" },
	{ 0x4000, "x" },
	{ 0x8000, "y" }
};

#ifndef CONTROLLER_NORMAL_INPUT

// too lazy to concat strings :^)
const char* ControllerEnvNames[] = {
	"getinput_controller1_ltrig",
	"getinput_controller1_rtrig",
	"getinput_controller1_lthumbx",
	"getinput_controller1_lthumby",
	"getinput_controller1_rthumbx",
	"getinput_controller1_rthumby",
	"getinput_controller2_ltrig",
	"getinput_controller2_rtrig",
	"getinput_controller2_lthumbx",
	"getinput_controller2_lthumby",
	"getinput_controller2_rthumbx",
	"getinput_controller2_rthumby",
	"getinput_controller3_ltrig",
	"getinput_controller3_rtrig",
	"getinput_controller3_lthumbx",
	"getinput_controller3_lthumby",
	"getinput_controller3_rthumbx",
	"getinput_controller3_rthumby",
	"getinput_controller4_ltrig",
	"getinput_controller4_rtrig",
	"getinput_controller4_lthumbx",
	"getinput_controller4_lthumby",
	"getinput_controller4_rthumbx",
	"getinput_controller4_rthumby"
};

const char* ControllerBtnEnvNames[] = {
	"getinput_controller1_btns",
	"getinput_controller2_btns",
	"getinput_controller3_btns",
	"getinput_controller4_btns"
};

#endif

typedef struct _vec2i {
	int x, y;
} vec2i;

vec2i process_stick(vec2i axes, short deadzone) {
	const int deadzone_squared = deadzone * deadzone;

	if (axes.x * axes.x < deadzone_squared) {
		axes.x = 0;
	}

	if (axes.y * axes.y < deadzone_squared) {
		axes.y = 0;
	}

	return axes;
}

VOID GETINPUT_SUB PROCESS_CONTROLLER(float deadzone) {
	static char buffer[256] = { 0 };
	static char varName[32] = "getinput_controller";

	XINPUT_STATE state;
	DWORD dwResult, size = 0;

	for (char i = 0; i < 4; i++) {
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		dwResult = XInputGetState(i, &state);

#ifdef CONTROLLER_NORMAL_INPUT
		varName[19] = '0' + (i + 1);
#endif
		 
		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, sizeof buffer);

			vec2i left_stick = process_stick({ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			vec2i right_stick = process_stick({ state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));

#ifdef CONTROLLER_NORMAL_INPUT
			size = sprintf(buffer + size, "ltrig=%d,rtrig=%d,lthumbx=%d,lthumby=%d,rthumbx=%d,rthumby=%d", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger, left_stick.x, left_stick.y, right_stick.x, right_stick.y);

			if (state.Gamepad.wButtons) {
				buffer[size++] = '|';
			}

			int origSize = size;

#else
// YESHIS INSANE, NOT PRETTY FUCKERY BEGINS HERE
// I DONT WANT TO DO THIS
// I DONT WANNA SUPPORT ANOTHER INPUT MODE

			ENV(ControllerEnvNames[i * 6 + 0], itoa_(state.Gamepad.bLeftTrigger));
			ENV(ControllerEnvNames[i * 6 + 1], itoa_(state.Gamepad.bRightTrigger));
			ENV(ControllerEnvNames[i * 6 + 2], itoa_(left_stick.x));
			ENV(ControllerEnvNames[i * 6 + 3], itoa_(left_stick.y));
			ENV(ControllerEnvNames[i * 6 + 4], itoa_(right_stick.x));
			ENV(ControllerEnvNames[i * 6 + 5], itoa_(right_stick.y));

#endif

			int result;
			for (WORD var = 0; var < 14; var++) {
				if (result = (state.Gamepad.wButtons & ctrl_values[var].bitmask)) {
					if (
#ifdef CONTROLLER_NORMAL_INPUT
						(size != origSize) &&
#endif
					result) {
						buffer[size++] = ',';
					}

#ifdef CONTROLLER_NORMAL_INPUT
					size +=
#endif
						sprintf(buffer + size, "%s", ctrl_values[var].str);
				}
			}

#ifdef CONTROLLER_NORMAL_INPUT
			SetEnvironmentVariable(varName, buffer);
#else
			SetEnvironmentVariable(ControllerBtnEnvNames[i], buffer);
#endif

		} else {
			SetEnvironmentVariable(varName, NULL);
		}
	}
}

#endif

#ifdef NEW_MOUSE
DWORD GETINPUT_SUB CALLBACK ModeThread(void* data) {
	// i don't like this function. at all.
	// basically we're just chugging cpu because microsoft
	// decided to make windows utter trash.

	HANDLE hStdIn =  GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD modeIn, modeOut, inputModeRead, inputModeRead2;

#ifndef WIN2K_BUILD
	modeIn = (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE);
	modeOut = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
#else
#	ifndef ENABLE_EXTENDED_FLAGS
#		define ENABLE_EXTENDED_FLAGS 0x0080
#	endif

	modeIn = (ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	modeOut = 0;
#endif

	while (1) {
		SetConsoleMode(hStdIn, modeIn);
		//SetConsoleMode(hStdOut, modeOut);

#ifndef WIN2K_BUILD
		usleep(10);
#else
		Sleep(1);
#endif
	}

	return 0;
}

void MouseEventProc(MOUSE_EVENT_RECORD& record) {
	if (!inFocus) return;

	switch (record.dwEventFlags) {
	case MOUSE_MOVED: {
		int lmx = getenvnum("getinput_limitMouseX"), lmy = getenvnum("getinput_limitMouseY");

		int mouseX = record.dwMousePosition.X + 1;
		int mouseY = record.dwMousePosition.Y;
		if (lmx && mouseX > lmx) mouseX = lmx;
		if (lmy && mouseY > lmy) mouseY = lmy;

		ENV("getinput_mousexpos", itoa_(mouseX));
		ENV("getinput_mouseypos", itoa_(mouseY));
		break;
	}

	case MOUSE_WHEELED:
		ENV("getinput_wheeldelta", itoa_((signed short)(HIWORD(record.dwButtonState)) / WHEEL_DELTA));
		break;

	default: break;
	}
}

void processEvnt(INPUT_RECORD ir) {
	switch (ir.EventType) {
	case MOUSE_EVENT: {
		MouseEventProc(ir.Event.MouseEvent);
		break;
	}

	case KEY_EVENT:
	case FOCUS_EVENT: // handled in main loop, from testing this is quite buggy sadly
	case WINDOW_BUFFER_SIZE_EVENT:
	case MENU_EVENT:
	default:
		break;
	}
}

DWORD GETINPUT_SUB CALLBACK MousePosThread(void* data) {
	INPUT_RECORD ir[64];
	DWORD read = 0;

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	while (1) {
		ReadConsoleInput(hStdIn, ir, 64, &read);

		for (int i = 0; i < read; i++) {
			processEvnt(ir[i]);
		}
	}

	return 0;
}

#else

LRESULT GETINPUT_SUB CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
    short wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

    if (wheelDelta != 0) {
        wheelDelta /= WHEEL_DELTA;
        ENV("getinput_wheeldelta", itoa_(wheelDelta));
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD GETINPUT_SUB CALLBACK MouseMessageThread(void* data) {
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

#endif

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


void audioreplay(Audio::Audio* audio) {
	audio->Rewind();
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

					// audio QUEUE_Id play|stop|pause
					if (strlen(inbuf) > 7 && (inbuf, "audio ") == 0) {
						int audionumber = inbuf[7] - '0';

						if (strcmp(inbuf + 7, "play ") == 0) {
							char* str = inbuf + 12;
							auto audio = new Audio::AudioFile(str);
							streams[audionumber].Play(*audio);
							queues[audionumber].push_back(audio);
						}

						if (strcmp(inbuf + 7, "stop") == 0) {
							queues[audionumber].front()->Stop();
							queues[audionumber].pop_front();
						}

						if (strcmp(inbuf + 7, "rewind") == 0) {
							queues[audionumber].front()->Rewind();
						}

						if (strcmp(inbuf + 7, "pause") == 0) {
						}

						if (strcmp(inbuf + 7, "loop") == 0) {
							queues[audionumber].front()->SetEndCallback(audioreplay);
						}
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


#include "dllmain_conhost.h"
#include "Injector.h"