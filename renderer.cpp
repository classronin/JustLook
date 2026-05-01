#include "renderer.h"
#include "globals.h"
#include "theme.h"
#include "svg_render.h"
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

// 全局变量定义（已在 globals.h 中声明）
ComPtr<ID2D1Factory6>       g_factory;
ComPtr<ID3D11Device>        g_d3dDevice;
ComPtr<ID3D11DeviceContext> g_d3dContext;
ComPtr<IDXGISwapChain1>     g_swapChain;
ComPtr<ID2D1Device5>        g_d2dDevice;
ComPtr<ID2D1DeviceContext5> g_d2dContext;
ComPtr<ID2D1SvgDocument>    g_svgDocument;
bool                        g_isSvg = false;

bool InitRenderer(HWND hwnd)
{
    D2D1_FACTORY_OPTIONS opts = {};
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory6), &opts, (void**)g_factory.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
        D3D11_SDK_VERSION, &g_d3dDevice, &featureLevel, &g_d3dContext);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = g_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return false;
    hr = g_factory->CreateDevice(dxgiDevice.Get(), g_d2dDevice.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        g_d2dContext.GetAddressOf());
    if (FAILED(hr)) return false;

    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) return false;

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width       = 0;
    scDesc.Height      = 0;
    scDesc.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount      = 2;
    scDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    hr = dxgiFactory->CreateSwapChainForHwnd(g_d3dDevice.Get(), hwnd, &scDesc,
        nullptr, nullptr, g_swapChain.GetAddressOf());
    if (FAILED(hr)) return false;

    // 初始绑定（仅用于初始化，Render 时会重新绑定）
    ComPtr<IDXGISurface> surface;
    hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ComPtr<ID2D1Bitmap1> targetBmp;
    hr = g_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), bmpProps, &targetBmp);
    if (FAILED(hr)) return false;
    g_d2dContext->SetTarget(targetBmp.Get());

    return true;
}

void CleanupRenderer()
{
    g_d2dContext.Reset();
    g_d2dDevice.Reset();
    g_swapChain.Reset();
    g_d3dContext.Reset();
    g_d3dDevice.Reset();
    g_factory.Reset();
}

void OnResize(UINT width, UINT height)
{
    if (!g_swapChain || width == 0 || height == 0) return;
    g_d2dContext->SetTarget(nullptr);
    HRESULT hr = g_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return;

    // 重新绑定新的后备缓冲区（并设置渲染目标）
    ComPtr<IDXGISurface> surface;
    hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) return;

    D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ComPtr<ID2D1Bitmap1> targetBmp;
    hr = g_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), bmpProps, &targetBmp);
    if (FAILED(hr)) return;
    g_d2dContext->SetTarget(targetBmp.Get());
}

// ── 每帧必须重新获取 PUFFER ──
void Render()
{
    PAINTSTRUCT ps;
    BeginPaint(g_hWndMain, &ps);
    if (!g_d2dContext || !g_swapChain) {
        EndPaint(g_hWndMain, &ps);
        return;
    }

    // 获取交换链当前后备缓冲区（Flip 模型要求每次绘制前必须获取）
    ComPtr<IDXGISurface> surface;
    HRESULT hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) {
        EndPaint(g_hWndMain, &ps);
        return;
    }

    D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ComPtr<ID2D1Bitmap1> targetBmp;
    hr = g_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), bmpProps, &targetBmp);
    if (FAILED(hr)) {
        EndPaint(g_hWndMain, &ps);
        return;
    }
    g_d2dContext->SetTarget(targetBmp.Get());

    g_d2dContext->BeginDraw();
    DrawBackground(g_d2dContext.Get());

    if (g_isSvg && g_svgDocument)
    {
        RECT clientRect;
        GetClientRect(g_hWndMain, &clientRect);
        UINT clientWidth = clientRect.right - clientRect.left;
        UINT clientHeight = clientRect.bottom - clientRect.top;
        RenderSvg(g_scale, g_offsetX, g_offsetY, clientWidth, clientHeight);
    }
    else if (g_imageBitmap)
    {
        D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Scale(g_scale, g_scale) *
                                     D2D1::Matrix3x2F::Translation(g_offsetX, g_offsetY);
        g_d2dContext->SetTransform(transform);
        g_d2dContext->DrawBitmap(g_imageBitmap.Get());
        g_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    hr = g_d2dContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // 设备丢失，下次 WM_SIZE 会调用 OnResize 重建
    }
    else {
        g_swapChain->Present(1, 0);
    }

    EndPaint(g_hWndMain, &ps);
}