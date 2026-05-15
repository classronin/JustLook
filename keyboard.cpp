#include "keyboard.h"
#include "globals.h"
#include "settings.h"
#include "image_list.h"
#include "file_association.h"
#include "image_render.h"

void HandleKeyboard(WPARAM wParam)
{
    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    switch (wParam)
    {
    case VK_SPACE:
    {
        RECT rc;
        GetClientRect(g_hWndMain, &rc);
        float clientWidth = static_cast<float>(rc.right - rc.left);
        float clientHeight = static_cast<float>(rc.bottom - rc.top);
        
        g_userScaled = true;
        
        if (g_isSvg && g_svgDocument)
        {
            D2D1_SIZE_F svgSize = g_svgDocument->GetViewportSize();
            float scaleX = clientWidth / svgSize.width;
            float scaleY = clientHeight / svgSize.height;
            g_scale = (scaleX < scaleY) ? scaleX : scaleY;
            
            g_offsetX = (clientWidth - svgSize.width * g_scale) * 0.5f;
            g_offsetY = (clientHeight - svgSize.height * g_scale) * 0.5f;
        }
        else if (g_imageBitmap)
        {
            float scaleX = clientWidth / g_imageSize.width;
            float scaleY = clientHeight / g_imageSize.height;
            g_scale = (scaleX < scaleY) ? scaleX : scaleY;
            
            g_offsetX = (clientWidth - g_imageSize.width * g_scale) * 0.5f;
            g_offsetY = (clientHeight - g_imageSize.height * g_scale) * 0.5f;
        }
        Repaint();
        break;
    }
    case VK_TAB:
    {
        int mode = GetThemeMode();
        mode = (mode + 1) % 3;
        SetThemeMode(mode);
        Repaint();
    }
    break;
    case VK_LEFT:
        NavigatePrev();
        break;
    case VK_RIGHT:
        NavigateNext();
        break;
    case VK_UP:
        NavigateFirst();
        break;
    case VK_DOWN:
        NavigateLast();
        break;
    case 'A':
        if (ctrlPressed)
        {
            if (RegisterFileAssociations())
            {
                MessageBoxW(g_hWndMain, L"File association registered!", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(g_hWndMain, L"Failed to register file association. Please run as administrator.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;
    case 'U':
        if (ctrlPressed)
        {
            if (UnregisterFileAssociations())
            {
                MessageBoxW(g_hWndMain, L"File association unregistered!", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(g_hWndMain, L"Failed to unregister file association.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;
    }
}
