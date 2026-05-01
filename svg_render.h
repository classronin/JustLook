#pragma once
#include <d2d1_3.h>
#include <wrl/client.h>

bool LoadSvgFromFile(const wchar_t* filePath);
void ReleaseSvg();
void RenderSvg(float scale, float offsetX, float offsetY, UINT clientWidth, UINT clientHeight);