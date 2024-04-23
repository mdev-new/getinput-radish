#include "Controller.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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

		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, sizeof buffer);

			vec2i left_stick = process_stick({ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			vec2i right_stick = process_stick({ state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));

			ENV(ControllerEnvNames[i * 6 + 0], itoa_(state.Gamepad.bLeftTrigger));
			ENV(ControllerEnvNames[i * 6 + 1], itoa_(state.Gamepad.bRightTrigger));
			ENV(ControllerEnvNames[i * 6 + 2], itoa_(left_stick.x));
			ENV(ControllerEnvNames[i * 6 + 3], itoa_(left_stick.y));
			ENV(ControllerEnvNames[i * 6 + 4], itoa_(right_stick.x));
			ENV(ControllerEnvNames[i * 6 + 5], itoa_(right_stick.y));

			int result;
			for (WORD var = 0; var < 14; var++) {
				if (result = (state.Gamepad.wButtons & ctrl_values[var].bitmask)) {
					if (result) {
						buffer[size++] = ',';
					}

    				sprintf(buffer + size, "%s", ctrl_values[var].str);
				}
			}

			SetEnvironmentVariable(ControllerBtnEnvNames[i], buffer);

		} else {
			SetEnvironmentVariable(varName, NULL);
		}
	}
}
