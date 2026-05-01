#include "file_association.h"
#include "globals.h"
#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib, "shlwapi.lib")

// Supported image file extensions
static const wchar_t* g_supportedExtensions[] = {
    L".png", L".jpg", L".jpeg", L".bmp", L".gif", L".tiff", L".tif", L".webp", L".ico"
};
static const int g_extensionCount = sizeof(g_supportedExtensions) / sizeof(g_supportedExtensions[0]);

// Get full executable path
static std::wstring GetExecutablePath()
{
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::wstring(path);
}

// Write registry value
static bool WriteRegistryValue(HKEY hRoot, const wchar_t* subKey, const wchar_t* valueName, const wchar_t* data)
{
    HKEY hKey;
    LONG result = RegCreateKeyExW(hRoot, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS)
        return false;

    result = RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)data, (DWORD)(wcslen(data) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

// Delete registry key
static bool DeleteRegistryKey(HKEY hRoot, const wchar_t* subKey)
{
    return RegDeleteKeyW(hRoot, subKey) == ERROR_SUCCESS || GetLastError() == ERROR_FILE_NOT_FOUND;
}

// Register single file type
static bool RegisterFileType(const wchar_t* extension)
{
    std::wstring exePath = GetExecutablePath();
    std::wstring progId = L"JustLook" + std::wstring(extension);
    std::wstring progIdKey = L"Software\\Classes\\" + progId;

    // 1. Set extension to ProgID mapping
    std::wstring extKey = L"Software\\Classes\\" + std::wstring(extension);
    if (!WriteRegistryValue(HKEY_CURRENT_USER, extKey.c_str(), L"", progId.c_str()))
        return false;

    // 2. Set ProgID default value (display name)
    std::wstring displayName = std::wstring(extension + 1) + L" Image";
    if (!WriteRegistryValue(HKEY_CURRENT_USER, progIdKey.c_str(), L"", displayName.c_str()))
        return false;

    // 3. Set default icon (use exe icon)
    std::wstring iconPath = exePath + L",0";
    if (!WriteRegistryValue(HKEY_CURRENT_USER, (progIdKey + L"\\DefaultIcon").c_str(), L"", iconPath.c_str()))
        return false;

    // 4. Register Shell open command
    std::wstring command = L"\"" + exePath + L"\" \"%1\"";
    if (!WriteRegistryValue(HKEY_CURRENT_USER, (progIdKey + L"\\shell\\open\\command").c_str(), L"", command.c_str()))
        return false;

    return true;
}

// Unregister single file type
static bool UnregisterFileType(const wchar_t* extension)
{
    std::wstring progId = L"JustLook" + std::wstring(extension);

    // Delete extension association
    std::wstring extKey = L"Software\\Classes\\" + std::wstring(extension);
    DeleteRegistryKey(HKEY_CURRENT_USER, extKey.c_str());

    // Delete ProgID
    std::wstring progIdKey = L"Software\\Classes\\" + progId;
    DeleteRegistryKey(HKEY_CURRENT_USER, progIdKey.c_str());

    return true;
}

// Register all supported file types
bool RegisterFileAssociations()
{
    for (int i = 0; i < g_extensionCount; i++)
    {
        if (!RegisterFileType(g_supportedExtensions[i]))
            return false;
    }

    // Notify system that file associations have changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, nullptr, nullptr);

    return true;
}

// Unregister all supported file types
bool UnregisterFileAssociations()
{
    for (int i = 0; i < g_extensionCount; i++)
    {
        UnregisterFileType(g_supportedExtensions[i]);
    }

    // Notify system that file associations have changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, nullptr, nullptr);

    return true;
}

// Check if already registered
bool IsFileAssociationsRegistered()
{
    std::wstring progId = L"JustLook" + std::wstring(g_supportedExtensions[0]);
    std::wstring progIdKey = L"Software\\Classes\\" + progId;

    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, progIdKey.c_str(), 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}
