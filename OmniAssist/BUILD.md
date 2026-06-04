# OmniAssist 构建指南

## 环境配置

- **Qt 版本**: 6.11.1
- **Qt 路径**: D:\Qt\6.11.1
- **操作系统**: Windows 10/11

## 功能概述

已实现的功能：
- ✅ 微信数据库读取（联系人列表、聊天记录）
- ✅ AI 自动回复
- ✅ 会议纪要生成
- ✅ 人物画像分析
- ✅ API Key 配置
- ✅ 深色主题界面

## 快速开始

### 方法 1：使用 MinGW 编译（推荐）

```bash
# 在 OmniAssist 目录双击
build-windows.bat
```

### 方法 2：使用 Visual Studio 2022 编译

```bash
# 在 OmniAssist 目录双击
build-vs2022.bat
```

### 方法 3：使用 Qt Creator

1. 打开 Qt Creator
2. File → Open File or Project
3. 选择 `OmniAssist/CMakeLists.txt`
4. 选择合适的 Kit（MinGW 64-bit 或 MSVC 2022）
5. 点击 Configure
6. 点击 Run

## 手动构建步骤

如果你想手动构建，步骤如下：

### 1. 打开命令提示符

```powershell
# 进入项目目录
cd OmniAssist
```

### 2. 设置环境变量（根据你的编译器选择）

#### MinGW:
```powershell
set QT_DIR=D:\Qt\6.11.1\mingw_64
set PATH=%QT_DIR%\bin;%PATH%
```

#### MSVC:
```powershell
set QT_DIR=D:\Qt\6.11.1\msvc2019_64
set PATH=%QT_DIR%\bin;%PATH%
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### 3. 配置项目

#### MinGW:
```powershell
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:\Qt\6.11.1\mingw_64" ..
```

#### MSVC:
```powershell
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="D:\Qt\6.11.1\msvc2019_64" ..
```

### 4. 编译

```powershell
cmake --build . --config Release
```

### 5. 运行

```powershell
# MinGW 编译结果
./OmniAssist.exe

# MSVC 编译结果
./Release/OmniAssist.exe
```

## 目录结构

```
OmniAssist/
├── CMakeLists.txt          # CMake 构建配置
├── build-windows.bat       # MinGW 快速构建
├── build-vs2022.bat        # Visual Studio 快速构建
├── BUILD.md                # 本文件
├── src/
│   ├── main.cpp
│   ├── core/               # 核心模块
│   ├── adapters/           # 平台适配
│   ├── ai/                 # AI 服务
│   ├── automation/         # 自动化模块
│   └── ui/                 # 界面
└── docs/                   # 文档
```

## 常见问题

### Q: 报错 "找不到 Qt"？

A: 检查 `D:\Qt\6.11.1` 目录是否存在，路径中确保有 `mingw_64` 或 `msvc2019_64` 子目录。

### Q: 编译时缺少 DLL？

A: 确保已将 `D:\Qt\6.11.1\mingw_64\bin`（或 msvc 路径）添加到 PATH 环境变量中。

### Q: 我可以用其他 Qt 版本吗？

A: 可以。只需要修改脚本中的 `QT_DIR` 变量即可，建议使用 Qt 6.5+。

### Q: 如何调试？

A: 使用 Qt Creator 可以获得最佳调试体验。或者在 CMake 配置时使用 `Debug` 模式：
```powershell
cmake -DCMAKE_BUILD_TYPE=Debug ..
```
