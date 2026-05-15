#include "globals.h"
#include "renderer.h"
#include "settings.h"
#include "theme.h"
#include "keyboard.h"
#include "dragdrop.h"
#include "image_loader.h"
#include "image_list.h"
#include "image_render.h"
#include "file_association.h"
#include "resource.h"
#include <windowsx.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

// 全局变量定义
HINSTANCE g_hInst = nullptr;
HWND      g_hWndMain = nullptr;
wchar_t   g_iniPath[MAX_PATH] = L"";
std::wstring g_currentFilePath;

float g_scale = 1.0f;
float g_offsetX = 0.0f;
float g_offsetY = 0.0f;
bool  g_userScaled = false;
bool  g_dragging = false;
int   g_lastX = 0, g_lastY = 0;
bool  g_inSizeMove = false;

// 通用图像数据
Microsoft::WRL::ComPtr<ID2D1Bitmap1> g_imageBitmap;
D2D1_SIZE_F g_imageSize = { 0.0f, 0.0f };

// GIF 动画支持
bool     g_isAnimated = false;
int      g_currentFrame = 0;
int      g_totalFrames = 0;
UINT     g_frameDelay = 100;
DWORD   g_lastFrameTime = 0;

void Repaint()
{
    InvalidateRect(g_hWndMain, nullptr, TRUE);
}

void UpdateWindowTitle()
{
    if (!g_hWndMain) return;
    std::wstring title = L"JustLook";
    if (!g_currentFilePath.empty()) {
        size_t lastSlash = g_currentFilePath.find_last_of(L"\\/");
        std::wstring fileName = (lastSlash != std::wstring::npos)
                                ? g_currentFilePath.substr(lastSlash + 1)
                                : g_currentFilePath;
        title = fileName + L" - JustLook";
    }
    SetWindowTextW(g_hWndMain, title.c_str());
}

void UpdateAnimation()
{
    if (!g_isAnimated || g_totalFrames <= 1)
        return;

    DWORD currentTime = GetTickCount();
    if (currentTime - g_lastFrameTime >= g_frameDelay)
    {
        g_currentFrame = (g_currentFrame + 1) % g_totalFrames;
        g_lastFrameTime = currentTime;

        if (g_currentFrame < static_cast<int>(g_animationFrames.size()))
        {
            g_imageBitmap = g_animationFrames[g_currentFrame].bitmap;
            g_frameDelay = g_animationFrames[g_currentFrame].delay;

            SetTimer(g_hWndMain, 2, g_frameDelay, NULL);
        }

        Repaint();
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        DragAcceptFiles(hwnd, TRUE);
        break;

    case WM_SIZE:
        if (g_swapChain)
        {
            UINT w = LOWORD(lParam), h = HIWORD(lParam);
            
            static UINT lastW = 0, lastH = 0;
            if (w == lastW && h == lastH)
                return 0;
            lastW = w; lastH = h;
            
            OnResize(w, h);
            
            // 窗口调整大小期间，清除渲染目标以避免重影
            if (g_inSizeMove && g_d2dContext)
            {
                g_d2dContext->SetTarget(nullptr);
            }
            
            if (!g_inSizeMove)
            {
                if (!g_userScaled)
                {
                    FitImageToWindow(hwnd);
                }
                else
                {
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    float clientWidth = static_cast<float>(rc.right - rc.left);
                    float clientHeight = static_cast<float>(rc.bottom - rc.top);
                    
                    if (g_isSvg && g_svgDocument)
                    {
                        D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
                        g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
                        g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
                    }
                    else if (g_imageBitmap)
                    {
                        g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
                        g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
                    }
                }
                Repaint();
            }

            SaveWindowGeometry(hwnd);
        }
        return 0;
        
    case WM_ENTERSIZEMOVE:
        g_inSizeMove = true;
        return 0;
        
    case WM_EXITSIZEMOVE:
        {
            g_inSizeMove = false;
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            float clientWidth = static_cast<float>(rc.right - rc.left);
            float clientHeight = static_cast<float>(rc.bottom - rc.top);
            
            if (!g_userScaled)
            {
                FitImageToWindow(hwnd);
            }
            else
            {
                if (g_isSvg && g_svgDocument)
                {
                    D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
                    g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
                    g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
                }
                else if (g_imageBitmap)
                {
                    g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
                    g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
                }
            }
            
            Repaint();
        }
        return 0;

    case WM_PAINT:
        Render();
        return 0;

    case WM_KEYDOWN:
        HandleKeyboard(wParam);
        return 0;

    case WM_DROPFILES:
        HandleDragDrop(hwnd, (HDROP)wParam);
        return 0;

    case WM_MOUSEWHEEL:
        {
            float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam);
            float factor = (delta > 0) ? 1.1f : 1.0f/1.1f;
            if (wParam & MK_CONTROL)
                factor = (delta > 0) ? (float)GetCtrlZoomMultiplier() : 1.0f / (float)GetCtrlZoomMultiplier();

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            float newScale = g_scale * factor;
            if (newScale >= 0.1f && newScale <= 10.0f)
            {
                float mx = (pt.x - g_offsetX) / g_scale;
                float my = (pt.y - g_offsetY) / g_scale;
                g_scale = newScale;
                g_offsetX = pt.x - mx * g_scale;
                g_offsetY = pt.y - my * g_scale;
                g_userScaled = true;
                Repaint();
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        g_dragging = true;
        g_lastX = GET_X_LPARAM(lParam);
        g_lastY = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        return 0;

    case WM_LBUTTONUP:
        g_dragging = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (g_dragging)
        {
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
            g_offsetX += (x - g_lastX);
            g_offsetY += (y - g_lastY);
            g_lastX = x; g_lastY = y;
            Repaint();
        }
        return 0;

    case WM_RBUTTONDOWN:
        g_userScaled = false;
        g_scale = 1.0f;
        
        if (g_isSvg && g_svgDocument)
        {
            D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_offsetX = (rc.right - svgSize.width) * 0.5f;
            g_offsetY = (rc.bottom - svgSize.height) * 0.5f;
        }
        else if (g_imageBitmap)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_offsetX = (rc.right - g_imageSize.width) * 0.5f;
            g_offsetY = (rc.bottom - g_imageSize.height) * 0.5f;
        }
        
        Repaint();
        return 0;

    case WM_SYSCOMMAND:
        // 处理窗口最大化、还原等状态变化
        if ((wParam & 0xFFF0) == SC_MAXIMIZE || (wParam & 0xFFF0) == SC_RESTORE || (wParam & 0xFFF0) == SC_MINIMIZE)
        {
            // 延迟保存窗口几何信息，确保状态变化完成
            SetTimer(hwnd, 1, 100, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == 1)
        {
            KillTimer(hwnd, 1);
            SaveWindowGeometry(hwnd);
        }
        else if (wParam == 2) // 动画定时器
        {
            UpdateAnimation();
        }
        else if (wParam == 3 && g_userScaled)
        {
            KillTimer(hwnd, 3);
            RECT rc;
            GetClientRect(hwnd, &rc);
            float clientWidth = static_cast<float>(rc.right - rc.left);
            float clientHeight = static_cast<float>(rc.bottom - rc.top);
            
            if (g_isSvg && g_svgDocument)
            {
                D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
                g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
                g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
            }
            else if (g_imageBitmap)
            {
                g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
                g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
            }
            Repaint();
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        CleanupRenderer();
        SaveWindowGeometry(hwnd);
        SaveSettings();
        WritePrivateProfileStringW(NULL, NULL, NULL, g_iniPath); // 强制刷新
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInst = hInstance;

    // 设置 ini 路径
    GetModuleFileNameW(nullptr, g_iniPath, MAX_PATH);
    wchar_t* p = wcsrchr(g_iniPath, L'\\');
    if (p) wcscpy_s(p + 1, MAX_PATH - (p - g_iniPath) - 1, L"config.ini");

    LoadSettings();

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"JustLookClass";
    RegisterClassW(&wc);

    // 先读取保存的窗口几何信息
    int x = GetPrivateProfileIntW(L"Window", L"X", CW_USEDEFAULT, g_iniPath);
    int y = GetPrivateProfileIntW(L"Window", L"Y", CW_USEDEFAULT, g_iniPath);
    int w = GetPrivateProfileIntW(L"Window", L"Width", 800, g_iniPath);
    int h = GetPrivateProfileIntW(L"Window", L"Height", 600, g_iniPath);
    int max = GetPrivateProfileIntW(L"Window", L"Maximized", 0, g_iniPath);

    g_hWndMain = CreateWindowExW(
        0,
        L"JustLookClass", L"",
        WS_OVERLAPPEDWINDOW, x, y,
        w, h, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWndMain) return 1;

    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
    DragAcceptFiles(g_hWndMain, TRUE);

    if (!InitRenderer(g_hWndMain)) return 1;
    UpdateWindowTitle();            // 显示初始标题

    // 如果窗口之前是最大化状态，现在设置为最大化（提前显示）
    if (max) {
        ShowWindow(g_hWndMain, SW_HIDE);
        ShowWindow(g_hWndMain, SW_SHOWMAXIMIZED);
    } else {
        ShowWindow(g_hWndMain, nCmdShow);
    }
    UpdateWindow(g_hWndMain);

    // 处理等待中的消息，确保窗口几何信息生效
    MSG dummyMsg;
    while (PeekMessage(&dummyMsg, g_hWndMain, 0, 0, PM_REMOVE)) {
        TranslateMessage(&dummyMsg);
        DispatchMessage(&dummyMsg);
    }

    // 命令行参数加载图像
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1)
    {
        LoadImageAndScanDir(argv[1]);
    }
    LocalFree(argv);

    // 确保窗口能够接收键盘消息
    SetFocus(g_hWndMain);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
