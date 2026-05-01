#pragma once
#include <Windows.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <vector>
#include <string>

extern HINSTANCE g_hInst;
extern HWND      g_hWndMain;
extern wchar_t   g_iniPath[MAX_PATH];

// 视图状态
extern float   g_scale;
extern float   g_offsetX;
extern float   g_offsetY;
extern bool    g_userScaled;
extern bool    g_dragging;
extern int     g_lastX, g_lastY;
extern bool    g_inSizeMove;  // 窗口正在调整大小中

// 渲染接口（定义在 renderer.cpp 中）
extern Microsoft::WRL::ComPtr<ID2D1Factory6>       g_factory;
extern Microsoft::WRL::ComPtr<ID3D11Device>        g_d3dDevice;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3dContext;
extern Microsoft::WRL::ComPtr<IDXGISwapChain1>     g_swapChain;
extern Microsoft::WRL::ComPtr<ID2D1Device5>        g_d2dDevice;
extern Microsoft::WRL::ComPtr<ID2D1DeviceContext5> g_d2dContext;

extern Microsoft::WRL::ComPtr<ID2D1Bitmap1>     g_imageBitmap;
extern D2D1_SIZE_F                              g_imageSize;
extern Microsoft::WRL::ComPtr<ID2D1SvgDocument> g_svgDocument;
extern bool                                     g_isSvg;

// GIF 动画帧存储结构
struct AnimationFrame
{
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    UINT delay; // 帧延迟（毫秒）
};

// GIF 动画支持
extern bool     g_isAnimated;        // 是否是动画图像
extern int      g_currentFrame;      // 当前帧索引
extern int      g_totalFrames;       // 总帧数
extern UINT     g_frameDelay;        // 当前帧延迟（毫秒）
extern DWORD   g_lastFrameTime;     // 上一帧时间
extern std::vector<AnimationFrame> g_animationFrames; // 动画帧列表

// 工具
void Repaint();
void UpdateWindowTitle();
void UpdateAnimation();  // 更新动画帧

// 当前加载文件的完整路径
extern std::wstring g_currentFilePath;