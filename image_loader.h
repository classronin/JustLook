#pragma once
#include <d2d1_3.h>
#include <wrl/client.h>
#include <string>

// WIC 支持的图像扩展名
bool IsWICFormat(const wchar_t* ext);

// stb_image 支持的图像扩展名
bool IsStbFormat(const wchar_t* ext);

// 支持的图像格式（WIC + stb_image）
bool IsSupportedImageFormat(const wchar_t* filePath);

// 加载图像文件为 D2D1Bitmap1
// 成功返回 true，并设置 g_imageBitmap 和 g_imageSize
bool LoadImageFromFile(const wchar_t* filePath);

// 释放当前图像
void ReleaseCurrentImage();
