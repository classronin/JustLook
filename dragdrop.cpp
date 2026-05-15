#include "dragdrop.h"
#include "globals.h"
#include "image_render.h"
#include <shellapi.h>
#include <string>

void HandleDragDrop(HWND hwnd, HDROP hDrop)
{
    UINT pathLen = DragQueryFileW(hDrop, 0, nullptr, 0);
    if (pathLen == 0)
    {
        DragFinish(hDrop);
        return;
    }
    std::wstring filePath(pathLen, L'\0');
    UINT copied = DragQueryFileW(hDrop, 0, &filePath[0], pathLen + 1);
    DragFinish(hDrop);
    if (copied == 0 || filePath.empty())
        return;

    if (LoadImageAndScanDir(filePath.c_str()))
    {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
}