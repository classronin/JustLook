#pragma once
#include <Windows.h>

int  GetCtrlZoomMultiplier();
void SetCtrlZoomMultiplier(int mul);
int  GetThemeMode();
void SetThemeMode(int mode);
void LoadSettings();
void SaveSettings();
void LoadWindowGeometry(HWND hwnd);
void SaveWindowGeometry(HWND hwnd);