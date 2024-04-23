#include "Utilities.h"

inline int _max(int a, int b) {
    return ((a > b) ? a : b);
}

char* itoa_(int i) {
	static char buffer[21] = { 0 };

	char* c = buffer + 19; // buffer[20] must be \0
	int x = abs(i);

	do {
		*--c = 48 + x % 10;
	} while (x && (x /= 10));

	if (i < 0) *--c = 45;
	return c;
}

long getenvnum(const char* name) {
	static char buffer[32] = { 0 };
	return
		GetEnvironmentVariable(name, buffer, sizeof(buffer))
		? atol(buffer)
		: 0;
}

long getenvnum_ex(const char* name, int default_val) {
	static char buffer[32] = { 0 };
	return
		GetEnvironmentVariable(name, buffer, sizeof(buffer))
		? atol(buffer)
		: default_val;
}

char* readenv(const char* name) {
	static TCHAR buffer[127];
	GetEnvironmentVariable(name, buffer, sizeof buffer);
	return _strdup(buffer); // memory leaks go brr
}

void usleep(__int64 usec) {
#ifndef WIN2K_BUILD
	static HANDLE timer = NULL;
	static LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	if (timer == NULL)
        timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
#else
    Sleep(usec / 1000);
#endif
}

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

