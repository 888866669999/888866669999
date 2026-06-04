@echo off
:: OmniAssist Windows 构建脚本
:: Qt 版本: 6.11.1
:: Qt 路径: D:\Qt

echo ====================================
echo OmniAssist 构建脚本
echo ====================================
echo.

:: 设置环境变量
set QT_DIR=D:\Qt\6.11.1
set QT_BIN=%QT_DIR%\mingw_64\bin
set PATH=%QT_BIN%;%PATH%

echo Qt 目录: %QT_DIR%
echo.

:: 检查 Qt 是否存在
if not exist "%QT_DIR%\mingw_64" (
    echo [错误] 找不到 Qt Mingw 目录！
    echo 请检查: %QT_DIR%\mingw_64
    pause
    exit /b 1
)

:: 检查 MinGW 编译器
if not exist "%QT_DIR%\mingw_64\bin\g++.exe" (
    echo [错误] 找不到 g++ 编译器！
    echo 请确保安装了 MinGW 工具链
    pause
    exit /b 1
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
cmake -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%\mingw_64" ^
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
echo %CD%\OmniAssist.exe
echo.

pause
