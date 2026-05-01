#include "theme.h"
#include "globals.h"
#include "settings.h"
#include <d2d1_3.h>

void DrawBackground(ID2D1DeviceContext5* dc)
{
    int mode = GetThemeMode();
    RECT rc;
    GetClientRect(g_hWndMain, &rc);
    float w = (float)(rc.right - rc.left);
    float h = (float)(rc.bottom - rc.top);

    if (mode == 0) // 透明棋盘格
    {
        // 创建一个半透明的棋盘格背景，而不是完全透明
        D2D1_COLOR_F bgColor = D2D1::ColorF(0.95f, 0.95f, 0.95f, 0.8f);
        dc->Clear(bgColor);
        
        // 绘制棋盘格
        const int cellSize = 16;
        for (int y = 0; y < h; y += cellSize)
        {
            for (int x = 0; x < w; x += cellSize)
            {
                D2D1_COLOR_F color = ((x/cellSize + y/cellSize) % 2 == 0) ?
                    D2D1::ColorF(0.85f, 0.85f, 0.85f, 0.9f) : D2D1::ColorF(0.95f, 0.95f, 0.95f, 0.9f);
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
                dc->CreateSolidColorBrush(color, &brush);
                dc->FillRectangle(D2D1::RectF((float)x, (float)y, (float)(x+cellSize), (float)(y+cellSize)),
                                 brush.Get());
            }
        }
    }
    else
    {
        D2D1_COLOR_F bg = (mode == 1) ? D2D1::ColorF(D2D1::ColorF::White) :
                                        D2D1::ColorF(0.15f, 0.15f, 0.15f);
        dc->Clear(bg);
    }
}