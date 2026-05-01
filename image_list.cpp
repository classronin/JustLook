#include "image_list.h"
#include "image_loader.h"
#include "globals.h"
#include "image_render.h"
#include <windows.h>
#include <shlwapi.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "shlwapi.lib")

static std::vector<std::wstring> g_imageList;
static int g_currentIndex = -1;

const std::vector<std::wstring>& GetImageList()
{
    return g_imageList;
}

int GetCurrentImageIndex()
{
    return g_currentIndex;
}

void SetCurrentImageIndex(int index)
{
    if (index >= 0 && index < static_cast<int>(g_imageList.size()))
        g_currentIndex = index;
}

bool ScanDirectoryForImages(const wchar_t* filePath)
{
    if (!filePath) return false;

    // 使用 std::wstring 避免长度限制
    std::wstring fullFilePath(filePath);
    std::wstring dirPath;
    size_t pos = fullFilePath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return false;
    dirPath = fullFilePath.substr(0, pos);  // 去掉文件名，保留目录部分

    g_imageList.clear();
    g_currentIndex = -1;

    std::wstring searchPath = dirPath + L"\\*.*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;

        if (IsSupportedImageFormat(findData.cFileName))
        {
            std::wstring fullPath = dirPath + L"\\" + findData.cFileName;
            g_imageList.push_back(fullPath);
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    std::sort(g_imageList.begin(), g_imageList.end());

    for (size_t i = 0; i < g_imageList.size(); i++)
    {
        if (_wcsicmp(g_imageList[i].c_str(), fullFilePath.c_str()) == 0)
        {
            g_currentIndex = static_cast<int>(i);
            break;
        }
    }
    return !g_imageList.empty();
}

bool NavigatePrev()
{
    if (g_imageList.empty()) return false;
    g_currentIndex--;
    if (g_currentIndex < 0)
        g_currentIndex = static_cast<int>(g_imageList.size()) - 1;
    if (!LoadImageFromFile(g_imageList[g_currentIndex].c_str()))
        return false;
    FitImageToWindow(g_hWndMain);
    Repaint();
    UpdateWindowTitle();
    return true;
}

bool NavigateNext()
{
    if (g_imageList.empty()) return false;
    g_currentIndex++;
    if (g_currentIndex >= static_cast<int>(g_imageList.size()))
        g_currentIndex = 0;
    if (!LoadImageFromFile(g_imageList[g_currentIndex].c_str()))
        return false;
    FitImageToWindow(g_hWndMain);
    Repaint();
    UpdateWindowTitle();
    return true;
}

bool NavigateFirst()
{
    if (g_imageList.empty()) return false;
    g_currentIndex = 0;
    if (!LoadImageFromFile(g_imageList[0].c_str())) return false;
    FitImageToWindow(g_hWndMain);
    Repaint();
    UpdateWindowTitle();
    return true;
}

bool NavigateLast()
{
    if (g_imageList.empty()) return false;
    g_currentIndex = static_cast<int>(g_imageList.size()) - 1;
    if (!LoadImageFromFile(g_imageList.back().c_str())) return false;
    FitImageToWindow(g_hWndMain);
    Repaint();
    UpdateWindowTitle();
    return true;
}