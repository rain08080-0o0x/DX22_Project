#ifndef __INPUT_H__
#define __INPUT_H__

#include <Windows.h>
#undef max
#undef min

HRESULT InitInput();
void UninitInput();
void UpdateInput();

bool IsKeyPress(BYTE key);
bool IsKeyTrigger(BYTE key);
bool IsKeyRelease(BYTE key);
bool IsKeyRepeat(BYTE key);
bool IsPadConnected();
float GetPadLeftStickX();
float GetPadLeftStickY();
bool IsPadLeftShoulderTrigger();
bool IsPadRightShoulderTrigger();
bool IsMouseLeftPress();
bool IsMouseLeftTrigger();
bool IsMouseLeftRelease();
POINT GetMousePosition();

#endif // __INPUT_H__
