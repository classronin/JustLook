
# JustLook 只看 —— Windows 图像查看器 （无编辑功能）

下载：[JustLook.exe](https://github.com/classronin/JustLook/releases/latest/download/JustLook.exe)

支持格式：
1. WIC 格式（Windows内置）
.png/.jpg/.bmp/.gif/.tiff/.webp/.ico/.wdp
2. stb_image 格式
.tga/.psd/.hdr/.qoi
3. SVG 格式（Direct2D渲染）
.svg


## 配置

彻底移除菜单，窗口上无菜单。

快捷键：
- Tab键：透明，浅色和深色。
- ← 键 → 键： 切换当前目录下图像文件的名称顺序,目录内循环。
- ↑ 键 ↓ 键： 切换当前目录下图像文件的名称开头与末尾
- 鼠标滚轮缩放功能，鼠标右键重置缩放。
- 程序会在同目录下创建 `config.ini` 文件，ini可以编辑CTRL+鼠标滚轮缩放倍数。
```
CtrlMultiplier=2
```
>默认为2倍数，1-10


### 注册关联：
Ctrl+A - 注册关联
Ctrl+U - 注销关联

### 添 exe 图标
使用 rc.exe 编译资源文件：`rc JustLook.rc`
>链接时包含生成的 .res 文件

### 编译命令
```
cl.exe /std:c++17 /EHsc /O2 /D UNICODE /D _UNICODE /D _WIN32_WINNT=0x0A00 *.cpp /link /OUT:JustLook.exe JustLook.res user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib d2d1.lib d3d11.lib dxgi.lib windowscodecs.lib shlwapi.lib comctl32.lib advapi32.lib /SUBSYSTEM:WINDOWS
```

