#pragma once
#include <windows.h>

// 将图像适配到窗口大小
// hwnd: 窗口句柄
void FitImageToWindow(HWND hwnd);

// 加载图像并扫描同目录
// filePath: 要加载的图像文件路径
// 返回: 是否成功
bool LoadImageAndScanDir(const wchar_t* filePath);
