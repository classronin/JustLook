@echo off
set "VS_DIR=C:\Program Files\Microsoft Visual Studio\18\Community"
set "MSVC_DIR=%VS_DIR%\VC\Tools\MSVC\14.50.35717"
set "SDK_DIR=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"

set "PATH=%MSVC_DIR%\bin\Hostx64\x64;%SDK_DIR%\bin\10.0.26100.0\x64;%PATH%"
set "INCLUDE=%VS_DIR%\VC\Auxiliary\VS\Include;%MSVC_DIR%\ATLMFC\Include;%MSVC_DIR%\Include;%MSVC_DIR%\crt\src;%SDK_DIR%\ucrt;%SDK_DIR%\um;%SDK_DIR%\shared;%SDK_DIR%\winrt"
set "LIB=%MSVC_DIR%\ATLMFC\Lib\x64;%MSVC_DIR%\Lib\x64;%MSVC_DIR%\crt\lib\x64;%SDK_DIR%\ucrt\x64;%SDK_DIR%\um\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\um\x64"

cd /d E:\GitHub\C\JustLook
set CFLAGS=/std:c++17 /EHsc /O2 /D UNICODE /D _UNICODE /D _WIN32_WINNT=0x0A00

echo Compiling JustLook.cpp...
cl.exe %CFLAGS% /c JustLook.cpp
if errorlevel 1 goto error

echo Compiling image_loader.cpp...
cl.exe %CFLAGS% /c image_loader.cpp
if errorlevel 1 goto error

echo Compiling image_render.cpp...
cl.exe %CFLAGS% /c image_render.cpp
if errorlevel 1 goto error

echo Compiling svg_render.cpp...
cl.exe %CFLAGS% /c svg_render.cpp
if errorlevel 1 goto error

echo Compiling renderer.cpp...
cl.exe %CFLAGS% /c renderer.cpp
if errorlevel 1 goto error

echo Compiling image_list.cpp...
cl.exe %CFLAGS% /c image_list.cpp
if errorlevel 1 goto error

echo Compiling dragdrop.cpp...
cl.exe %CFLAGS% /c dragdrop.cpp
if errorlevel 1 goto error

echo Compiling keyboard.cpp...
cl.exe %CFLAGS% /c keyboard.cpp
if errorlevel 1 goto error

echo Compiling settings.cpp...
cl.exe %CFLAGS% /c settings.cpp
if errorlevel 1 goto error

echo Compiling theme.cpp...
cl.exe %CFLAGS% /c theme.cpp
if errorlevel 1 goto error

echo Compiling file_association.cpp...
cl.exe %CFLAGS% /c file_association.cpp
if errorlevel 1 goto error

echo Compiling resources...
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" JustLook.rc
if errorlevel 1 goto error

echo Linking...
link.exe /OUT:JustLook.exe *.obj JustLook.res user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib d2d1.lib d3d11.lib dxgi.lib windowscodecs.lib shlwapi.lib comctl32.lib advapi32.lib /SUBSYSTEM:WINDOWS
if errorlevel 1 goto error

echo Build successful!
goto end

:error
echo Build failed!
pause

:end
pause