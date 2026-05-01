#include "svg_render.h"
#include "globals.h"
#include <d2d1_3.h>
#include <ole2.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

extern Microsoft::WRL::ComPtr<ID2D1SvgDocument> g_svgDocument;

static D2D1_SIZE_F ParseViewBox(const char* data, size_t size)
{
    const char* p = strstr(data, "viewBox");
    if (!p || p - data >= (int)size) return D2D1::SizeF(100, 100);
    p += 7;
    while (p < data + size && (*p == ' ' || *p == '\t' || *p == '=' || *p == '"' || *p == '\'')) p++;
    // skip to first number
    while (p < data + size && (*p == ' ' || *p == '\t')) p++;
    // skip x and y
    while (p < data + size && *p >= '0' && *p <= '9') p++;
    while (p < data + size && (*p == ' ' || *p == '.' || *p == '-')) p++;
    while (p < data + size && *p >= '0' && *p <= '9') p++;
    while (p < data + size && (*p == ' ' || *p == '\t')) p++;
    // read width
    char buf[32] = {0};
    int i = 0;
    while (p < data + size && i < 31 && ((*p >= '0' && *p <= '9') || *p == '.')) buf[i++] = *p++;
    float w = (float)atof(buf);
    while (p < data + size && (*p == ' ' || *p == '\t')) p++;
    // read height
    memset(buf, 0, sizeof(buf));
    i = 0;
    while (p < data + size && i < 31 && ((*p >= '0' && *p <= '9') || *p == '.')) buf[i++] = *p++;
    float h = (float)atof(buf);
    if (w > 0 && h > 0) return D2D1::SizeF(w, h);
    return D2D1::SizeF(100, 100);
}

bool LoadSvgFromFile(const wchar_t* filePath)
{
    ReleaseSvg();

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streampos size = file.tellg();
    if (size <= 0)
        return false;

    std::vector<char> buffer(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), size))
        return false;
    file.close();

    D2D1_SIZE_F viewBox = ParseViewBox(buffer.data(), static_cast<size_t>(size));

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, static_cast<DWORD>(size));
    if (!hGlobal)
        return false;

    LPVOID pBuffer = GlobalLock(hGlobal);
    if (!pBuffer)
    {
        GlobalFree(hGlobal);
        return false;
    }

    memcpy(pBuffer, buffer.data(), static_cast<size_t>(size));
    GlobalUnlock(hGlobal);

    ComPtr<IStream> pStream;
    HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
    if (FAILED(hr))
        return false;

    hr = g_d2dContext->CreateSvgDocument(pStream.Get(), viewBox, &g_svgDocument);
    return SUCCEEDED(hr);
}

void ReleaseSvg()
{
    if (g_svgDocument)
    {
        g_svgDocument.Reset();
    }
}

void RenderSvg(float scale, float offsetX, float offsetY, UINT clientWidth, UINT clientHeight)
{
    if (!g_svgDocument || !g_d2dContext)
        return;

    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Scale(scale, scale) *
                                  D2D1::Matrix3x2F::Translation(offsetX, offsetY);
    g_d2dContext->SetTransform(transform);
    
    g_d2dContext->DrawSvgDocument(g_svgDocument.Get());
    
    g_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
}