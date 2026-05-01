#pragma once
#include <Windows.h>

bool InitRenderer(HWND hwnd);
void CleanupRenderer();
void Render();
void OnResize(UINT width, UINT height);