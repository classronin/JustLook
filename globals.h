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

extern float   g_scale;
extern float   g_offsetX;
extern float   g_offsetY;
extern bool    g_userScaled;
extern bool    g_dragging;
extern int     g_lastX, g_lastY;
extern bool    g_inSizeMove;

extern Microsoft::WRL::ComPtr<ID2D1Factory6>       g_factory;
extern Microsoft::WRL::ComPtr<ID3D11Device>        g_d3dDevice;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3dContext;
extern Microsoft::WRL::ComPtr<IDXGISwapChain1>     g_swapChain;
extern Microsoft::WRL::ComPtr<ID2D1Device5>        g_d2dDevice;
extern Microsoft::WRL::ComPtr<ID2D1DeviceContext5> g_d2dContext;

extern Microsoft::WRL::ComPtr<ID2D1Bitmap1>     g_imageBitmap;
extern D2D1_SIZE_F                              g_imageSize;
extern Microsoft::WRL::ComPtr<ID2D1SvgDocument> g_svgDocument;
extern bool                                    g_isSvg;

struct AnimationFrame
{
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    UINT delay;
};

extern bool     g_isAnimated;
extern int      g_currentFrame;
extern int      g_totalFrames;
extern UINT     g_frameDelay;
extern DWORD   g_lastFrameTime;
extern std::vector<AnimationFrame> g_animationFrames;

void Repaint();
void UpdateWindowTitle();
void UpdateAnimation();

extern std::wstring g_currentFilePath;