#include "settings.h"
#include "globals.h"
#include <Windows.h>

static int g_ctrlZoomMul = 2;
static int g_themeMode = 0;

int GetCtrlZoomMultiplier() { return g_ctrlZoomMul; }
void SetCtrlZoomMultiplier(int mul) { if (mul >= 2 && mul <= 10) g_ctrlZoomMul = mul; }
int GetThemeMode() { return g_themeMode; }
void SetThemeMode(int mode) { if (mode >= 0 && mode <= 2) g_themeMode = mode; }

void LoadSettings()
{
    g_ctrlZoomMul = GetPrivateProfileIntW(L"Zoom", L"CtrlMultiplier", -1, g_iniPath);
    if (g_ctrlZoomMul < 2 || g_ctrlZoomMul > 10)
    {
        g_ctrlZoomMul = 2;
        WritePrivateProfileStringW(L"Zoom", L"CtrlMultiplier", L"2", g_iniPath);
    }
    
    g_themeMode = GetPrivateProfileIntW(L"Theme", L"Mode", 0, g_iniPath);
    if (g_themeMode < 0 || g_themeMode > 2)
    {
        g_themeMode = 0;
        WritePrivateProfileStringW(L"Theme", L"Mode", L"0", g_iniPath);
    }
}

void SaveSettings()
{
    // 注意：CtrlMultiplier 不再保存，让用户手动编辑ini文件
    wchar_t buf[32];
    _itow_s(g_themeMode, buf, 32, 10);
    WritePrivateProfileStringW(L"Theme", L"Mode", buf, g_iniPath);
}

// 使用 WINDOWPLACEMENT 获取正常窗口位置（官方文档推荐）
void SaveWindowGeometry(HWND hwnd)
{
    if (hwnd == NULL) return;
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(hwnd, &wp)) return;

    RECT rc = wp.rcNormalPosition;
    wchar_t buf[32];
    _itow_s(rc.left, buf, 32, 10);
    WritePrivateProfileStringW(L"Window", L"X", buf, g_iniPath);
    _itow_s(rc.top, buf, 32, 10);
    WritePrivateProfileStringW(L"Window", L"Y", buf, g_iniPath);
    _itow_s(rc.right - rc.left, buf, 32, 10);
    WritePrivateProfileStringW(L"Window", L"Width", buf, g_iniPath);
    _itow_s(rc.bottom - rc.top, buf, 32, 10);
    WritePrivateProfileStringW(L"Window", L"Height", buf, g_iniPath);

    _itow_s((wp.showCmd == SW_SHOWMAXIMIZED) ? 1 : 0, buf, 32, 10);
    WritePrivateProfileStringW(L"Window", L"Maximized", buf, g_iniPath);
}

void LoadWindowGeometry(HWND hwnd)
{
    int x = GetPrivateProfileIntW(L"Window", L"X", CW_USEDEFAULT, g_iniPath);
    int y = GetPrivateProfileIntW(L"Window", L"Y", CW_USEDEFAULT, g_iniPath);
    int w = GetPrivateProfileIntW(L"Window", L"Width", 800, g_iniPath);
    int h = GetPrivateProfileIntW(L"Window", L"Height", 600, g_iniPath);
    int max = GetPrivateProfileIntW(L"Window", L"Maximized", 0, g_iniPath);

    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);
    
    // 如果窗口之前是最大化状态，设置最大化标志
    if (max) {
        wp.showCmd = SW_SHOWMAXIMIZED;
    } else {
        // 否则设置正常位置
        wp.rcNormalPosition = { x, y, x + w, y + h };
        wp.showCmd = SW_SHOWNORMAL;
    }
    
    SetWindowPlacement(hwnd, &wp);
}