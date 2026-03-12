#include "Input.h"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <Xinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Xinput.lib")

// Global state
BYTE g_keyTable[256]{};
BYTE g_oldTable[256]{};
XINPUT_STATE g_padState{};
XINPUT_STATE g_oldPadState{};
bool g_padConnected = false;
LPDIRECTINPUT8 g_pDirectInput = nullptr;
LPDIRECTINPUTDEVICE8 g_pDirectInputPad = nullptr;
DIJOYSTATE2 g_diPadState{};
DIJOYSTATE2 g_oldDiPadState{};
bool g_diPadConnected = false;
POINT g_mousePos{};
POINT g_oldMousePos{};
bool g_mouseLeftDown = false;
bool g_oldMouseLeftDown = false;

namespace
{
	const SHORT kStickDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	const float kStickPressThreshold = 0.35f;
	const LONG kDirectInputAxisRange = 1000;
	const LONG kDirectInputDeadZone = 250;

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

	float NormalizeDirectInputAxis(LONG value)
	{
		if (value > kDirectInputDeadZone)
		{
			return static_cast<float>(value - kDirectInputDeadZone) /
				static_cast<float>(kDirectInputAxisRange - kDirectInputDeadZone);
		}
		if (value < -kDirectInputDeadZone)
		{
			return static_cast<float>(value + kDirectInputDeadZone) /
				static_cast<float>(kDirectInputAxisRange - kDirectInputDeadZone);
		}
		return 0.0f;
	}

	bool IsPadButtonDown(const XINPUT_STATE& state, WORD button)
	{
		return (state.Gamepad.wButtons & button) != 0;
	}

	bool IsDirectInputButtonDown(const DIJOYSTATE2& state, int buttonIndex)
	{
		if (buttonIndex < 0 || buttonIndex >= 128)
		{
			return false;
		}
		return (state.rgbButtons[buttonIndex] & 0x80) != 0;
	}

	bool IsDirectInputPovDirection(const DIJOYSTATE2& state, int direction)
	{
		const DWORD pov = state.rgdwPOV[0];
		if (LOWORD(pov) == 0xFFFF)
		{
			return false;
		}

		switch (direction)
		{
		case 0: return (pov == 0 || pov == 4500 || pov == 31500);
		case 1: return (pov == 18000 || pov == 13500 || pov == 22500);
		case 2: return (pov == 27000 || pov == 22500 || pov == 31500);
		case 3: return (pov == 9000 || pov == 4500 || pov == 13500);
		default: return false;
		}
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

	bool IsMappedXInputPress(BYTE key, const XINPUT_STATE& state)
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
			return IsPadButtonDown(state, XINPUT_GAMEPAD_B);
		case 'Q':
		case 'O':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_Y);
		case 'E':
		case 'P':
			return IsPadButtonDown(state, XINPUT_GAMEPAD_X);
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

	bool IsMappedDirectInputPress(BYTE key, const DIJOYSTATE2& state)
	{
		const float lx = NormalizeDirectInputAxis(state.lX);
		const float ly = NormalizeDirectInputAxis(state.lY);

		switch (key)
		{
		case 'W':
		case VK_UP:
			return IsDirectInputPovDirection(state, 0) || (ly < -kStickPressThreshold);
		case 'S':
		case VK_DOWN:
			return IsDirectInputPovDirection(state, 1) || (ly > kStickPressThreshold);
		case 'A':
		case VK_LEFT:
			return IsDirectInputPovDirection(state, 2) || (lx < -kStickPressThreshold);
		case 'D':
		case VK_RIGHT:
			return IsDirectInputPovDirection(state, 3) || (lx > kStickPressThreshold);
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			return IsDirectInputButtonDown(state, 0);
		case VK_RETURN:
			return IsDirectInputButtonDown(state, 7) || IsDirectInputButtonDown(state, 0);
		case VK_TAB:
			return IsDirectInputButtonDown(state, 6);
		case 'F':
			return IsDirectInputButtonDown(state, 1);
		case 'Q':
		case 'O':
			return IsDirectInputButtonDown(state, 3);
		case 'E':
		case 'P':
			return IsDirectInputButtonDown(state, 2);
		case 'R':
			return IsDirectInputButtonDown(state, 3);
		case 'T':
			return IsDirectInputButtonDown(state, 6);
		case '1':
			return IsDirectInputButtonDown(state, 2);
		case '2':
			return IsDirectInputButtonDown(state, 0);
		case '3':
			return IsDirectInputButtonDown(state, 1);
		default:
			return false;
		}
	}

	bool SetDirectInputAxisRange(LPDIRECTINPUTDEVICE8 device, DWORD objectOffset)
	{
		if (!device)
		{
			return false;
		}

		DIPROPRANGE range{};
		range.diph.dwSize = sizeof(range);
		range.diph.dwHeaderSize = sizeof(range.diph);
		range.diph.dwHow = DIPH_BYOFFSET;
		range.diph.dwObj = objectOffset;
		range.lMin = -kDirectInputAxisRange;
		range.lMax = kDirectInputAxisRange;
		return SUCCEEDED(device->SetProperty(DIPROP_RANGE, &range.diph));
	}

	BOOL CALLBACK EnumGameControllerCallback(const DIDEVICEINSTANCE* instance, VOID* context)
	{
		if (!g_pDirectInput || g_pDirectInputPad)
		{
			return DIENUM_STOP;
		}

		HWND hWnd = static_cast<HWND>(context);
		LPDIRECTINPUTDEVICE8 device = nullptr;
		if (FAILED(g_pDirectInput->CreateDevice(instance->guidInstance, &device, nullptr)) || !device)
		{
			return DIENUM_CONTINUE;
		}

		if (FAILED(device->SetDataFormat(&c_dfDIJoystick2)))
		{
			device->Release();
			return DIENUM_CONTINUE;
		}

		if (FAILED(device->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		{
			device->Release();
			return DIENUM_CONTINUE;
		}

		SetDirectInputAxisRange(device, DIJOFS_X);
		SetDirectInputAxisRange(device, DIJOFS_Y);
		device->Acquire();

		g_pDirectInputPad = device;
		return DIENUM_STOP;
	}

	void UpdateDirectInputState()
	{
		g_oldDiPadState = g_diPadState;
		if (!g_pDirectInputPad)
		{
			g_diPadConnected = false;
			ZeroMemory(&g_diPadState, sizeof(g_diPadState));
			return;
		}

		HRESULT hr = g_pDirectInputPad->Poll();
		if (FAILED(hr))
		{
			hr = g_pDirectInputPad->Acquire();
			while (hr == DIERR_INPUTLOST)
			{
				hr = g_pDirectInputPad->Acquire();
			}
			hr = g_pDirectInputPad->Poll();
		}

		DIJOYSTATE2 state{};
		hr = g_pDirectInputPad->GetDeviceState(sizeof(state), &state);
		if (SUCCEEDED(hr))
		{
			g_diPadConnected = true;
			g_diPadState = state;
		}
		else
		{
			g_diPadConnected = false;
			ZeroMemory(&g_diPadState, sizeof(g_diPadState));
		}
	}

	float SelectStrongerAxis(float xinputValue, float directInputValue)
	{
		const float absXInput = (xinputValue < 0.0f) ? -xinputValue : xinputValue;
		const float absDirectInput = (directInputValue < 0.0f) ? -directInputValue : directInputValue;
		return (absDirectInput > absXInput) ? directInputValue : xinputValue;
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

	HWND hWnd = GetInputWindow();
	if (hWnd &&
		SUCCEEDED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&g_pDirectInput), nullptr)) &&
		g_pDirectInput)
	{
		g_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumGameControllerCallback, hWnd, DIEDFL_ATTACHEDONLY);
	}
	UpdateDirectInputState();
	g_oldDiPadState = g_diPadState;

	g_mousePos = QueryMousePosition();
	g_oldMousePos = g_mousePos;
	g_mouseLeftDown = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	g_oldMouseLeftDown = g_mouseLeftDown;

	return S_OK;
}

void UninitInput()
{
	if (g_pDirectInputPad)
	{
		g_pDirectInputPad->Unacquire();
		g_pDirectInputPad->Release();
		g_pDirectInputPad = nullptr;
	}
	if (g_pDirectInput)
	{
		g_pDirectInput->Release();
		g_pDirectInput = nullptr;
	}
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

	UpdateDirectInputState();
}

bool IsKeyPress(BYTE key)
{
	const bool keyPress = (g_keyTable[key] & 0x80) != 0;
	const bool xinputPress = g_padConnected && IsMappedXInputPress(key, g_padState);
	const bool directInputPress = g_diPadConnected && IsMappedDirectInputPress(key, g_diPadState);
	const bool padPress = xinputPress || directInputPress;
	return keyPress || padPress;
}

bool IsKeyTrigger(BYTE key)
{
	const bool keyTrigger = ((g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80) != 0;
	const bool xinputNow = g_padConnected && IsMappedXInputPress(key, g_padState);
	const bool xinputOld = IsMappedXInputPress(key, g_oldPadState);
	const bool directInputNow = g_diPadConnected && IsMappedDirectInputPress(key, g_diPadState);
	const bool directInputOld = IsMappedDirectInputPress(key, g_oldDiPadState);
	const bool padTrigger = (xinputNow && !xinputOld) || (directInputNow && !directInputOld);
	return keyTrigger || padTrigger;
}

bool IsKeyRelease(BYTE key)
{
	const bool keyRelease = ((g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80) != 0;
	const bool xinputNow = g_padConnected && IsMappedXInputPress(key, g_padState);
	const bool xinputOld = IsMappedXInputPress(key, g_oldPadState);
	const bool directInputNow = g_diPadConnected && IsMappedDirectInputPress(key, g_diPadState);
	const bool directInputOld = IsMappedDirectInputPress(key, g_oldDiPadState);
	const bool padRelease = (!xinputNow && xinputOld) || (!directInputNow && directInputOld);
	return keyRelease || padRelease;
}

bool IsKeyRepeat(BYTE key)
{
	return false;
}

bool IsPadConnected()
{
	return g_padConnected || g_diPadConnected;
}

float GetPadLeftStickX()
{
	const float xinputValue = g_padConnected ? NormalizeThumbAxis(g_padState.Gamepad.sThumbLX, kStickDeadZone) : 0.0f;
	const float directInputValue = g_diPadConnected ? NormalizeDirectInputAxis(g_diPadState.lX) : 0.0f;
	return SelectStrongerAxis(xinputValue, directInputValue);
}

float GetPadLeftStickY()
{
	const float xinputValue = g_padConnected ? NormalizeThumbAxis(g_padState.Gamepad.sThumbLY, kStickDeadZone) : 0.0f;
	const float directInputValue = g_diPadConnected ? -NormalizeDirectInputAxis(g_diPadState.lY) : 0.0f;
	return SelectStrongerAxis(xinputValue, directInputValue);
}

bool IsPadLeftShoulderTrigger()
{
	const bool xinputNow = g_padConnected && IsPadButtonDown(g_padState, XINPUT_GAMEPAD_LEFT_SHOULDER);
	const bool xinputOld = IsPadButtonDown(g_oldPadState, XINPUT_GAMEPAD_LEFT_SHOULDER);
	const bool directInputNow = g_diPadConnected && IsDirectInputButtonDown(g_diPadState, 4);
	const bool directInputOld = IsDirectInputButtonDown(g_oldDiPadState, 4);
	return (xinputNow && !xinputOld) || (directInputNow && !directInputOld);
}

bool IsPadRightShoulderTrigger()
{
	const bool xinputNow = g_padConnected && IsPadButtonDown(g_padState, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	const bool xinputOld = IsPadButtonDown(g_oldPadState, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	const bool directInputNow = g_diPadConnected && IsDirectInputButtonDown(g_diPadState, 5);
	const bool directInputOld = IsDirectInputButtonDown(g_oldDiPadState, 5);
	return (xinputNow && !xinputOld) || (directInputNow && !directInputOld);
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
