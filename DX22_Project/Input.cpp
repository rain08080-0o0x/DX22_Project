#include "Input.h"
#include <Xinput.h>

#pragma comment(lib, "Xinput.lib")

// Global state
BYTE g_keyTable[256]{};
BYTE g_oldTable[256]{};
XINPUT_STATE g_padState{};
XINPUT_STATE g_oldPadState{};
bool g_padConnected = false;
POINT g_mousePos{};
POINT g_oldMousePos{};
bool g_mouseLeftDown = false;
bool g_oldMouseLeftDown = false;

namespace
{
	const SHORT kStickDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	const float kStickPressThreshold = 0.35f;

	float NormalizeThumbAxis(SHORT value, SHORT deadZone)
	{
		if (value > deadZone)
		{
			return static_cast<float>(value - deadZone) / static_cast<float>(32767 - deadZone);
		}
		if (value < -deadZone)
		{
			return static_cast<float>(value + deadZone) / static_cast<float>(32768 - deadZone);
		}
		return 0.0f;
	}

	bool IsPadButtonDown(const XINPUT_STATE& state, WORD button)
	{
		return (state.Gamepad.wButtons & button) != 0;
	}

	HWND GetInputWindow()
	{
		HWND hWnd = GetActiveWindow();
		if (!hWnd) hWnd = GetForegroundWindow();
		if (!hWnd) hWnd = GetFocus();
		return hWnd;
	}

	POINT QueryMousePosition()
	{
		POINT pt{};
		::GetCursorPos(&pt);
		HWND hWnd = GetInputWindow();
		if (hWnd)
		{
			::ScreenToClient(hWnd, &pt);
		}
		return pt;
	}

	bool IsMappedPadPress(BYTE key, const XINPUT_STATE& state)
	{
		const float lx = NormalizeThumbAxis(state.Gamepad.sThumbLX, kStickDeadZone);
		const float ly = NormalizeThumbAxis(state.Gamepad.sThumbLY, kStickDeadZone);

		switch (key)
		{
		case 'W':
		case VK_UP:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_DPAD_UP) || (ly > kStickPressThreshold);
		case 'S':
		case VK_DOWN:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_DPAD_DOWN) || (ly < -kStickPressThreshold);
		case 'A':
		case VK_LEFT:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_DPAD_LEFT) || (lx < -kStickPressThreshold);
		case 'D':
		case VK_RIGHT:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_DPAD_RIGHT) || (lx > kStickPressThreshold);
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_A);
		case VK_RETURN:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_START) || IsPadButtonDown(state, XINPUT_GAMEPAD_A);
		case VK_TAB:
			return IsPadButtonDown(state, XINPUT_GAMEPAD_BACK);
		case 'F':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_X);
		case 'Q':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_Y);
		case 'E':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_B);
		case 'R':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_Y);
		case 'T':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_BACK);
		case '1':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_X);
		case '2':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_A);
		case '3':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_B);
		default:
			return false;
		}
	}
}

HRESULT InitInput()
{
	GetKeyboardState(g_keyTable);
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));

	XINPUT_STATE state{};
	if (XInputGetState(0, &state) == ERROR_SUCCESS)
	{
		g_padConnected = true;
		g_padState = state;
		g_oldPadState = state;
	}
	else
	{
		g_padConnected = false;
		ZeroMemory(&g_padState, sizeof(g_padState));
		ZeroMemory(&g_oldPadState, sizeof(g_oldPadState));
	}

	g_mousePos = QueryMousePosition();
	g_oldMousePos = g_mousePos;
	g_mouseLeftDown = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	g_oldMouseLeftDown = g_mouseLeftDown;

	return S_OK;
}

void UninitInput()
{
}

void UpdateInput()
{
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	GetKeyboardState(g_keyTable);

	g_oldMousePos = g_mousePos;
	g_oldMouseLeftDown = g_mouseLeftDown;
	g_mousePos = QueryMousePosition();
	g_mouseLeftDown = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	g_oldPadState = g_padState;
	XINPUT_STATE state{};
	if (XInputGetState(0, &state) == ERROR_SUCCESS)
	{
		g_padConnected = true;
		g_padState = state;
	}
	else
	{
		g_padConnected = false;
		ZeroMemory(&g_padState, sizeof(g_padState));
	}
}

bool IsKeyPress(BYTE key)
{
	const bool keyPress = (g_keyTable[key] & 0x80) != 0;
	const bool padPress = g_padConnected && IsMappedPadPress(key, g_padState);
	return keyPress || padPress;
}

bool IsKeyTrigger(BYTE key)
{
	const bool keyTrigger = ((g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80) != 0;
	const bool padNow = g_padConnected && IsMappedPadPress(key, g_padState);
	const bool padOld = IsMappedPadPress(key, g_oldPadState);
	const bool padTrigger = padNow && !padOld;
	return keyTrigger || padTrigger;
}

bool IsKeyRelease(BYTE key)
{
	const bool keyRelease = ((g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80) != 0;
	const bool padNow = g_padConnected && IsMappedPadPress(key, g_padState);
	const bool padOld = IsMappedPadPress(key, g_oldPadState);
	const bool padRelease = !padNow && padOld;
	return keyRelease || padRelease;
}

bool IsKeyRepeat(BYTE key)
{
	return false;
}

bool IsPadConnected()
{
	return g_padConnected;
}

float GetPadLeftStickX()
{
	if (!g_padConnected) return 0.0f;
	return NormalizeThumbAxis(g_padState.Gamepad.sThumbLX, kStickDeadZone);
}

float GetPadLeftStickY()
{
	if (!g_padConnected) return 0.0f;
	return NormalizeThumbAxis(g_padState.Gamepad.sThumbLY, kStickDeadZone);
}

bool IsMouseLeftPress()
{
	return g_mouseLeftDown;
}

bool IsMouseLeftTrigger()
{
	return g_mouseLeftDown && !g_oldMouseLeftDown;
}

bool IsMouseLeftRelease()
{
	return !g_mouseLeftDown && g_oldMouseLeftDown;
}

POINT GetMousePosition()
{
	return g_mousePos;
}
