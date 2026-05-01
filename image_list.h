#pragma once
#include <vector>
#include <string>

// 获取当前图像列表
const std::vector<std::wstring>& GetImageList();

// 获取当前图像索引
int GetCurrentImageIndex();

// 设置当前图像索引
void SetCurrentImageIndex(int index);

// 扫描目录并加载图像列表
// filePath: 当前打开的图像文件路径
// 返回: 是否成功
bool ScanDirectoryForImages(const wchar_t* filePath);

// 导航函数
bool NavigatePrev();
bool NavigateNext();
bool NavigateFirst();
bool NavigateLast();
