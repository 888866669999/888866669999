@echo off
:: OmniAssist Windows 构建脚本 - Visual Studio 2022
:: Qt 版本: 6.11.1
:: Qt 路径: D:\Qt

echo ====================================
echo OmniAssist 构建脚本 (Visual Studio)
echo ====================================
echo.

:: 设置环境变量
set QT_DIR=D:\Qt\6.11.1
set QT_BIN=%QT_DIR%\msvc2019_64\bin
set PATH=%QT_BIN%;%PATH%

echo Qt 目录: %QT_DIR%
echo.

:: 检查 Qt 是否存在
if not exist "%QT_DIR%\msvc2019_64" (
    echo [错误] 找不到 Qt MSVC 目录！
    echo 请检查: %QT_DIR%\msvc2019_64
    echo.
    echo 如果使用 MinGW，请运行 build-windows.bat
    pause
    exit /b 1
)

:: 查找 Visual Studio
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo [警告] 找不到 Visual Studio 2022，尝试自动检测...
)

:: 创建构建目录
if not exist "build" (
    mkdir build
    echo [信息] 创建 build 目录
)
cd build

echo.
echo [信息] 配置 CMake 项目...
echo.

:: 配置 CMake
cmake -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%\msvc2019_64" ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 (
    echo.
    echo [错误] CMake 配置失败！
    pause
    exit /b 1
)

echo.
echo [信息] 编译项目...
echo.

:: 编译项目
cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo [错误] 编译失败！
    pause
    exit /b 1
)

echo.
echo ====================================
echo 构建成功！
echo ====================================
echo.
echo 可执行文件位置:
echo %CD%\Release\OmniAssist.exe
echo.

pause
