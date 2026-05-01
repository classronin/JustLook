#include "keyboard.h"
#include "globals.h"
#include "settings.h"
#include "image_list.h"
#include "file_association.h"

void HandleKeyboard(WPARAM wParam)
{
    // 检查 Ctrl 键状态
    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    switch (wParam)
    {
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
    case 'A':  // Ctrl+A - 注册文件关联
        if (ctrlPressed)
        {
            if (RegisterFileAssociations())
            {
                MessageBoxW(g_hWndMain, L"文件关联注册成功！", L"成功", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(g_hWndMain, L"文件关联注册失败，请以管理员身份运行。", L"错误", MB_OK | MB_ICONERROR);
            }
        }
        break;
    case 'U':  // Ctrl+U - 取消文件关联
        if (ctrlPressed)
        {
            if (UnregisterFileAssociations())
            {
                MessageBoxW(g_hWndMain, L"文件关联已取消！", L"成功", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(g_hWndMain, L"取消文件关联失败。", L"错误", MB_OK | MB_ICONERROR);
            }
        }
        break;
    }
}
