#include "image_loader.h"
#include "globals.h"
#include "svg_render.h"
#include <wincodec.h>
#include <shlwapi.h>
#include <vector>
#include <stdio.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

using Microsoft::WRL::ComPtr;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

std::vector<AnimationFrame> g_animationFrames;

static const wchar_t* g_wicExtensions[] = {
    L".png", L".jpg", L".jpeg", L".bmp", L".gif", L".tiff", L".webp", L".ico", L".wdp", L".cur"
};

static const wchar_t* g_stbExtensions[] = {
    L".tga", L".psd", L".hdr", L".pnm", L".qoi"
};

static const wchar_t* g_svgExtensions[] = {
    L".svg"
};

bool IsWICFormat(const wchar_t* ext)
{
    if (!ext) return false;
    for (size_t i = 0; i < sizeof(g_wicExtensions) / sizeof(g_wicExtensions[0]); i++)
    {
        if (_wcsicmp(ext, g_wicExtensions[i]) == 0)
            return true;
    }
    return false;
}

bool IsStbFormat(const wchar_t* ext)
{
    if (!ext) return false;
    for (size_t i = 0; i < sizeof(g_stbExtensions) / sizeof(g_stbExtensions[0]); i++)
    {
        if (_wcsicmp(ext, g_stbExtensions[i]) == 0)
            return true;
    }
    return false;
}

bool IsSupportedImageFormat(const wchar_t* filePath)
{
    if (!filePath) return false;
    const wchar_t* ext = PathFindExtensionW(filePath);
    if (!ext || *ext == L'\0') return false;

    if (IsWICFormat(ext)) return true;
    if (IsStbFormat(ext)) return true;

    for (size_t i = 0; i < sizeof(g_svgExtensions) / sizeof(g_svgExtensions[0]); i++)
    {
        if (_wcsicmp(ext, g_svgExtensions[i]) == 0)
            return true;
    }
    return false;
}

// Read file to memory (supports Unicode paths)
static std::vector<unsigned char> ReadFileToMemory(const wchar_t* filePath, size_t* outSize = nullptr)
{
    FILE* f = nullptr;
    _wfopen_s(&f, filePath, L"rb");
    if (!f)
    {
        if (outSize) *outSize = 0;
        return {};
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<unsigned char> data((size_t)fsize);
    size_t readSize = fread(data.data(), 1, (size_t)fsize, f);
    fclose(f);

    if (readSize != (size_t)fsize)
    {
        if (outSize) *outSize = 0;
        return {};
    }

    if (outSize) *outSize = (size_t)fsize;
    return data;
}

static UINT GetFrameDelay(IWICBitmapFrameDecode* frame)
{
    UINT delay = 100;
    ComPtr<IWICMetadataQueryReader> metadataReader;
    if (SUCCEEDED(frame->GetMetadataQueryReader(&metadataReader)))
    {
        PROPVARIANT propValue;
        PropVariantInit(&propValue);
        if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue)))
        {
            if (propValue.vt == VT_UI2)
            {
                delay = propValue.uiVal * 10;
                if (delay == 0) delay = 100;
            }
            PropVariantClear(&propValue);
        }
        else if (SUCCEEDED(metadataReader->GetMetadataByName(L"/appext/Application", &propValue)))
        {
            PropVariantClear(&propValue);
        }
        else if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Disposal", &propValue)))
        {
            PropVariantClear(&propValue);
        }
    }
    return delay;
}

static bool LoadImageWithStb(const wchar_t* filePath)
{
    const wchar_t* ext = PathFindExtensionW(filePath);

    // QOI format
    if (ext && _wcsicmp(ext, L".qoi") == 0)
    {
        std::vector<unsigned char> fileData = ReadFileToMemory(filePath);
        if (fileData.empty()) return false;

        qoi_desc desc;
        void* pixels = qoi_decode(fileData.data(), (int)fileData.size(), &desc, 4);
        if (!pixels) return false;

        int width = desc.width;
        int height = desc.height;

        std::vector<unsigned char> bgraData(width * height * 4);
        for (int i = 0; i < width * height; i++)
        {
            bgraData[i * 4] = ((unsigned char*)pixels)[i * 4 + 2];
            bgraData[i * 4 + 1] = ((unsigned char*)pixels)[i * 4 + 1];
            bgraData[i * 4 + 2] = ((unsigned char*)pixels)[i * 4];
            bgraData[i * 4 + 3] = ((unsigned char*)pixels)[i * 4 + 3];
        }

        free(pixels);

        UINT stride = width * 4;
        D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

        HRESULT hr = g_d2dContext->CreateBitmap(
            D2D1::SizeU(width, height),
            bgraData.data(),
            stride,
            bitmapProps,
            &g_imageBitmap);

        if (FAILED(hr))
        {
            g_imageBitmap.Reset();
            return false;
        }

        g_imageSize.width = (float)width;
        g_imageSize.height = (float)height;
        g_isAnimated = false;
        g_totalFrames = 0;
        g_currentFrame = 0;
        KillTimer(g_hWndMain, 2);
        return true;
    }

    // HDR format
    if (ext && _wcsicmp(ext, L".hdr") == 0)
    {
        std::vector<unsigned char> fileData = ReadFileToMemory(filePath);
        if (fileData.empty()) return false;

        int width, height, channels;
        float* fdata = stbi_loadf_from_memory(fileData.data(), (int)fileData.size(), &width, &height, &channels, 0);
        if (!fdata) return false;

        std::vector<unsigned char> bgraData(width * height * 4);
        for (int i = 0; i < width * height; i++)
        {
            float r = fdata[i * channels];
            float g = (channels > 1) ? fdata[i * channels + 1] : r;
            float b = (channels > 2) ? fdata[i * channels + 2] : r;
            float a = (channels > 3) ? fdata[i * channels + 3] : 1.0f;

            r = powf(r * 0.9f, 0.45f);
            g = powf(g * 0.9f, 0.45f);
            b = powf(b * 0.9f, 0.45f);

            bgraData[i * 4] = (unsigned char)(b * 255.0f);
            bgraData[i * 4 + 1] = (unsigned char)(g * 255.0f);
            bgraData[i * 4 + 2] = (unsigned char)(r * 255.0f);
            bgraData[i * 4 + 3] = (unsigned char)(a * 255.0f);
        }

        stbi_image_free(fdata);

        UINT stride = width * 4;
        D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

        HRESULT hr = g_d2dContext->CreateBitmap(
            D2D1::SizeU(width, height),
            bgraData.data(),
            stride,
            bitmapProps,
            &g_imageBitmap);

        if (FAILED(hr))
        {
            g_imageBitmap.Reset();
            return false;
        }

        g_imageSize.width = (float)width;
        g_imageSize.height = (float)height;
        g_isAnimated = false;
        g_totalFrames = 0;
        g_currentFrame = 0;
        KillTimer(g_hWndMain, 2);
        return true;
    }

    // Standard formats (TGA, PSD, HDR, QOI, PNM, PIC)
    std::vector<unsigned char> fileData = ReadFileToMemory(filePath);
    if (fileData.empty()) return false;

    int width, height, channels;
    unsigned char* data = stbi_load_from_memory(fileData.data(), (int)fileData.size(), &width, &height, &channels, 0);
    if (!data)
    {
        return false;
    }

    if (channels < 1 || channels > 4)
    {
        stbi_image_free(data);
        return false;
    }

    std::vector<unsigned char> bgraData(width * height * 4);
    for (int i = 0; i < width * height; i++)
    {
        unsigned char r = 0, g = 0, b = 0, a = 255;

        if (channels == 1)
        {
            r = g = b = data[i];
        }
        else if (channels == 2)
        {
            r = g = b = data[i * 2];
            a = data[i * 2 + 1];
        }
        else if (channels == 3)
        {
            r = data[i * 3];
            g = data[i * 3 + 1];
            b = data[i * 3 + 2];
        }
        else if (channels == 4)
        {
            r = data[i * 4];
            g = data[i * 4 + 1];
            b = data[i * 4 + 2];
            a = data[i * 4 + 3];
        }

        bgraData[i * 4] = b;
        bgraData[i * 4 + 1] = g;
        bgraData[i * 4 + 2] = r;
        bgraData[i * 4 + 3] = a;
    }

    stbi_image_free(data);

    UINT stride = width * 4;
    D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    HRESULT hr = g_d2dContext->CreateBitmap(
        D2D1::SizeU(width, height),
        bgraData.data(),
        stride,
        bitmapProps,
        &g_imageBitmap);

    if (FAILED(hr))
    {
        g_imageBitmap.Reset();
        return false;
    }

    g_imageSize.width = (float)width;
    g_imageSize.height = (float)height;
    g_isAnimated = false;
    g_totalFrames = 0;
    g_currentFrame = 0;
    KillTimer(g_hWndMain, 2);
    return true;
}

bool LoadImageFromFile(const wchar_t* filePath)
{
    if (!filePath || !g_factory || !g_d2dContext)
        return false;

    ReleaseCurrentImage();
    
    // 加载新图像时重置缩放状态
    g_userScaled = false;
    g_scale = 1.0f;
    g_offsetX = 0;
    g_offsetY = 0;

    const wchar_t* ext = PathFindExtensionW(filePath);

    if (ext && _wcsicmp(ext, L".svg") == 0)
    {
        if (LoadSvgFromFile(filePath))
        {
            g_isSvg = true;
            g_currentFilePath = filePath;
            return true;
        }
        return false;
    }

    if (ext && IsWICFormat(ext))
    {
        ComPtr<IWICImagingFactory> wicFactory;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory));
        if (FAILED(hr))
            goto try_stb;

        ComPtr<IWICBitmapDecoder> decoder;
        hr = wicFactory->CreateDecoderFromFilename(
            filePath,
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder);
        if (FAILED(hr))
            goto try_stb;

        GUID containerFormat;
        hr = decoder->GetContainerFormat(&containerFormat);
        if (SUCCEEDED(hr) && containerFormat == GUID_ContainerFormatGif)
        {
            g_isAnimated = true;
            g_animationFrames.clear();

            UINT frameCount = 0;
            decoder->GetFrameCount(&frameCount);
            g_totalFrames = (int)frameCount;
            g_currentFrame = 0;

            for (UINT i = 0; i < frameCount; i++)
            {
                ComPtr<IWICBitmapFrameDecode> frame;
                hr = decoder->GetFrame(i, &frame);
                if (FAILED(hr)) continue;

                UINT delay = GetFrameDelay(frame.Get());

                ComPtr<IWICFormatConverter> converter;
                hr = wicFactory->CreateFormatConverter(&converter);
                if (FAILED(hr)) continue;

                hr = converter->Initialize(
                    frame.Get(),
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeMedianCut);
                if (FAILED(hr)) continue;

                UINT width, height;
                hr = converter->GetSize(&width, &height);
                if (FAILED(hr)) continue;

                UINT stride = width * 4;
                UINT imageSize = stride * height;
                std::vector<BYTE> pixels(imageSize);
                hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
                if (FAILED(hr)) continue;

                ComPtr<ID2D1Bitmap1> bitmap;
                D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_NONE,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

                hr = g_d2dContext->CreateBitmap(
                    D2D1::SizeU(width, height),
                    pixels.data(),
                    stride,
                    bitmapProps,
                    &bitmap);
                if (FAILED(hr)) continue;

                AnimationFrame animFrame;
                animFrame.bitmap = bitmap;
                animFrame.delay = delay;
                g_animationFrames.push_back(animFrame);
            }

            if (!g_animationFrames.empty())
            {
                g_imageBitmap = g_animationFrames[0].bitmap;
                g_frameDelay = g_animationFrames[0].delay;
                g_lastFrameTime = GetTickCount();

                D2D1_SIZE_F size = g_imageBitmap->GetSize();
                g_imageSize.width = size.width;
                g_imageSize.height = size.height;

                SetTimer(g_hWndMain, 2, g_frameDelay, NULL);
            }

            g_currentFilePath = filePath;
            return true;
        }
        else
        {
            g_isAnimated = false;
            g_totalFrames = 0;
            g_currentFrame = 0;
            KillTimer(g_hWndMain, 2);

            ComPtr<IWICBitmapFrameDecode> frame;
            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) goto try_stb;

            ComPtr<IWICFormatConverter> converter;
            hr = wicFactory->CreateFormatConverter(&converter);
            if (FAILED(hr)) goto try_stb;

            hr = converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom);
            if (FAILED(hr)) goto try_stb;

            UINT width, height;
            hr = converter->GetSize(&width, &height);
            if (FAILED(hr)) goto try_stb;

            UINT stride = width * 4;
            UINT imageSize = stride * height;
            std::vector<BYTE> pixels(imageSize);
            hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
            if (FAILED(hr)) goto try_stb;

            D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

            hr = g_d2dContext->CreateBitmap(
                D2D1::SizeU(width, height),
                pixels.data(),
                stride,
                bitmapProps,
                &g_imageBitmap);
            if (FAILED(hr))
            {
                g_imageBitmap.Reset();
                goto try_stb;
            }

            g_imageSize.width = (float)width;
            g_imageSize.height = (float)height;
            g_currentFilePath = filePath;
            return true;
        }
    }

try_stb:
    if (LoadImageWithStb(filePath))
    {
        g_isSvg = false;
        g_currentFilePath = filePath;
        return true;
    }

    return false;
}

void ReleaseCurrentImage()
{
    g_imageBitmap.Reset();
    g_imageSize.width = 0.0f;
    g_imageSize.height = 0.0f;
    ReleaseSvg();
    g_isSvg = false;
}
