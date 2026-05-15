#include "image_render.h"
#include "image_loader.h"
#include "image_list.h"
#include "globals.h"

void FitImageToWindow(HWND hwnd)
{
    if (g_userScaled)
        return;

    if (!hwnd)
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    float clientWidth = static_cast<float>(rc.right - rc.left);
    float clientHeight = static_cast<float>(rc.bottom - rc.top);

    if (clientWidth <= 0 || clientHeight <= 0)
        return;

    if (g_isSvg && g_svgDocument)
    {
        D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
        
        float scaleX = clientWidth / svgSize.width;
        float scaleY = clientHeight / svgSize.height;
        g_scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        if (g_scale > 1.0f)
            g_scale = 1.0f;
        
        g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
        g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
    }
    else if (g_imageBitmap)
    {
        float scaleX = clientWidth / g_imageSize.width;
        float scaleY = clientHeight / g_imageSize.height;
        g_scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        if (g_scale > 1.0f)
            g_scale = 1.0f;
        
        g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
        g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
    }
}

bool LoadImageAndScanDir(const wchar_t* filePath)
{
    if (!filePath)
        return false;

    if (!ScanDirectoryForImages(filePath))
        return false;

    if (!LoadImageFromFile(filePath))
        return false;

    g_userScaled = false;
    
    RECT rc;
    GetClientRect(g_hWndMain, &rc);
    float clientWidth = static_cast<float>(rc.right - rc.left);
    float clientHeight = static_cast<float>(rc.bottom - rc.top);
    
    if (g_isSvg && g_svgDocument)
    {
        D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
        float scaleX = clientWidth / svgSize.width;
        float scaleY = clientHeight / svgSize.height;
        g_scale = (scaleX < scaleY) ? scaleX : scaleY;
        if (g_scale > 1.0f) g_scale = 1.0f;
        g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
        g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
    }
    else if (g_imageBitmap)
    {
        float scaleX = clientWidth / g_imageSize.width;
        float scaleY = clientHeight / g_imageSize.height;
        g_scale = (scaleX < scaleY) ? scaleX : scaleY;
        if (g_scale > 1.0f) g_scale = 1.0f;
        g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
        g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
    }
    
    UpdateWindowTitle();
    Repaint();

    return true;
}
