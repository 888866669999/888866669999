# OmniAssist 核心框架 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 搭建 OmniAssist 项目核心框架，包括 CMake 配置、数据模型、适配器接口和基础 UI。

**Architecture:** 采用分层架构设计：数据模型层、适配器接口层、核心服务层、UI 层。

**Tech Stack:** Qt 6.5+, C++17, CMake, Windows API

---

## File Structure

```
OmniAssist/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── core/
│   │   └── models/
│   │       ├── Platform.h
│   │       ├── Contact.h
│   │       ├── MediaFile.h
│   │       └── ChatMessage.h
│   ├── adapters/
│   │   └── IPlatformAdapter.h
│   └── ui/
│       ├── MainWindow.h
│       └── MainWindow.cpp
└── resources/
    └── resources.qrc
```

---

### Task 1: CMakeLists.txt 项目配置

**Files:**
- Create: `OmniAssist/CMakeLists.txt`

- [ ] **Step 1: 编写 CMake 配置文件**

```cmake
cmake_minimum_required(VERSION 3.16)
project(OmniAssist VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
)

set(HEADERS
    src/core/models/Platform.h
    src/core/models/Contact.h
    src/core/models/MediaFile.h
    src/core/models/ChatMessage.h
    src/adapters/IPlatformAdapter.h
    src/ui/MainWindow.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
    )
endif()
```

- [ ] **Step 2: 创建空的资源文件**

```xml
<RCC>
    <qresource prefix="/">
    </qresource>
</RCC>
```

保存到: `OmniAssist/resources/resources.qrc`

- [ ] **Step 3: 验证 CMake 配置**

Run: 
```bash
cd /workspace/OmniAssist && mkdir -p build && cd build && cmake ..
```
Expected: CMake 配置成功，无错误

- [ ] **Step 4: Commit**

```bash
cd /workspace/OmniAssist
git init
git add CMakeLists.txt resources/resources.qrc
git commit -m "feat: initial project structure and CMake config"
```

---

### Task 2: 核心数据模型定义

**Files:**
- Create: `OmniAssist/src/core/models/Platform.h`
- Create: `OmniAssist/src/core/models/Contact.h`
- Create: `OmniAssist/src/core/models/MediaFile.h`
- Create: `OmniAssist/src/core/models/ChatMessage.h`

- [ ] **Step 1: 编写 Platform.h**

```cpp
#ifndef PLATFORM_H
#define PLATFORM_H

#include <QString>

enum class Platform {
    WeChat,
    QQ,
    DingTalk,
    Unknown
};

inline QString platformToString(Platform platform) {
    switch (platform) {
        case Platform::WeChat: return "微信";
        case Platform::QQ: return "QQ";
        case Platform::DingTalk: return "钉钉";
        default: return "未知";
    }
}

#endif
```

- [ ] **Step 2: 编写 MediaFile.h**

```cpp
#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <QString>
#include <QPixmap>

struct MediaFile {
    QString id;
    QString localPath;
    QString fileName;
    qint64 fileSize;
    QString fileType;
    QPixmap thumbnail;
};

#endif
```

- [ ] **Step 3: 编写 Contact.h**

```cpp
#ifndef CONTACT_H
#define CONTACT_H

#include <QString>
#include <QIcon>
#include <QPixmap>
#include <QVariant>
#include "Platform.h"

struct Contact {
    QString id;
    QString name;
    QString remark;
    Platform platform;
    QIcon platformIcon;
    QPixmap avatar;
    QVariant extra;
};

#endif
```

- [ ] **Step 4: 编写 ChatMessage.h**

```cpp
#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QVariant>
#include <QList>
#include "Platform.h"
#include "MediaFile.h"

enum class MessageType {
    Text,
    Image,
    File,
    Audio,
    Video,
    System,
    Unknown
};

struct ChatMessage {
    QString id;
    qint64 timestamp;
    QString senderId;
    QString senderName;
    QString content;
    MessageType type;
    bool isSelf;
    QList<MediaFile> mediaFiles;
    QVariant extra;
};

#endif
```

- [ ] **Step 5: Commit**

```bash
cd /workspace/OmniAssist
git add src/core/models/Platform.h src/core/models/Contact.h src/core/models/MediaFile.h src/core/models/ChatMessage.h
git commit -m "feat: add core data models"
```

---

### Task 3: IPlatformAdapter 适配器接口

**Files:**
- Create: `OmniAssist/src/adapters/IPlatformAdapter.h`

- [ ] **Step 1: 编写适配器基类接口**

```cpp
#ifndef IPLATFORMADAPTER_H
#define IPLATFORMADAPTER_H

#include <QString>
#include <QList>
#include <QIcon>
#include <QPixmap>
#include <QObject>
#include "../core/models/Platform.h"
#include "../core/models/Contact.h"
#include "../core/models/ChatMessage.h"
#include "../core/models/MediaFile.h"

class IPlatformAdapter {
public:
    virtual ~IPlatformAdapter() = default;

    virtual Platform platform() const = 0;
    virtual QString name() const = 0;
    virtual QIcon icon() const = 0;
    virtual bool isAvailable() const = 0;

    virtual bool initialize() = 0;
    virtual QList<Contact> getContacts() = 0;
    virtual QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) = 0;
    virtual QList<ChatMessage> getRecentMessages(int limit = 50) = 0;

    virtual bool startMonitoring() = 0;
    virtual void stopMonitoring() = 0;

    virtual bool supportsRichMedia() const { return false; }
    virtual QList<MediaFile> getMediaFilesForMessage(const QString& msgId) {
        Q_UNUSED(msgId);
        return {};
    }
    virtual QPixmap decryptImage(const MediaFile& mediaFile) {
        Q_UNUSED(mediaFile);
        return QPixmap();
    }
};

#define IPlatformAdapter_iid "com.omniassist.IPlatformAdapter"
Q_DECLARE_INTERFACE(IPlatformAdapter, IPlatformAdapter_iid)

#endif
```

- [ ] **Step 2: Commit**

```bash
cd /workspace/OmniAssist
git add src/adapters/IPlatformAdapter.h
git commit -m "feat: add platform adapter interface"
```

---

### Task 4: 基础 UI MainWindow

**Files:**
- Create: `OmniAssist/src/ui/MainWindow.h`
- Create: `OmniAssist/src/ui/MainWindow.cpp`

- [ ] **Step 1: 编写 MainWindow.h**

```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void setupConnections();

    QSplitter* m_splitter;
    QListWidget* m_contactList;
    QTextEdit* m_chatView;
    QTextEdit* m_inputEdit;

    QComboBox* m_providerCombo;
    QLineEdit* m_apiKeyEdit;
    QCheckBox* m_autoReplyCheck;
    QPushButton* m_aiReplyBtn;
    QPushButton* m_summaryBtn;
    QPushButton* m_personaBtn;
    QPushButton* m_sendBtn;
};

#endif
```

- [ ] **Step 2: 编写 MainWindow.cpp**

```cpp
#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();
    resize(1000, 700);
    setWindowTitle("OmniAssist - 全平台即时通讯智能中枢");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_contactList = new QListWidget(this);
    m_contactList->setMaximumWidth(300);

    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);

    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);

    auto* buttonLayout = new QHBoxLayout();
    m_aiReplyBtn = new QPushButton("AI 回复", this);
    m_summaryBtn = new QPushButton("生成摘要", this);
    m_personaBtn = new QPushButton("人物分析", this);
    buttonLayout->addWidget(m_aiReplyBtn);
    buttonLayout->addWidget(m_summaryBtn);
    buttonLayout->addWidget(m_personaBtn);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(100);
    m_sendBtn = new QPushButton("发送", this);

    rightLayout->addWidget(m_chatView);
    rightLayout->addLayout(buttonLayout);
    rightLayout->addWidget(m_inputEdit);
    rightLayout->addWidget(m_sendBtn);

    m_splitter->addWidget(m_contactList);
    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 3);

    auto* configWidget = new QWidget(this);
    auto* configLayout = new QHBoxLayout(configWidget);

    configLayout->addWidget(new QLabel("API 配置:", this));
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"OpenAI", "通义千问", "文心一言"});
    configLayout->addWidget(m_providerCombo);

    configLayout->addWidget(new QLabel("Key:", this));
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    configLayout->addWidget(m_apiKeyEdit);

    m_autoReplyCheck = new QCheckBox("自动回复", this);
    configLayout->addWidget(m_autoReplyCheck);

    mainLayout->addWidget(m_splitter);
    mainLayout->addWidget(configWidget);
}

void MainWindow::setupConnections() {
}
```

- [ ] **Step 3: Commit**

```bash
cd /workspace/OmniAssist
git add src/ui/MainWindow.h src/ui/MainWindow.cpp
git commit -m "feat: add main window UI skeleton"
```

---

### Task 5: main.cpp 入口文件

**Files:**
- Create: `OmniAssist/src/main.cpp`

- [ ] **Step 1: 编写 main.cpp**

```cpp
#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OmniAssist");
    app.setOrganizationName("OmniAssist");

    MainWindow window;
    window.show();

    return app.exec();
}
```

- [ ] **Step 2: 更新 CMakeLists.txt 添加 HEADERS 和 SOURCES**

编辑 `OmniAssist/CMakeLists.txt`，确保已包含所有文件。

- [ ] **Step 3: 编译验证**

Run:
```bash
cd /workspace/OmniAssist/build && cmake .. && cmake --build .
```
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
cd /workspace/OmniAssist
git add src/main.cpp CMakeLists.txt
git commit -m "feat: add main.cpp and complete basic build"
```

---

## 第一阶段完成！

核心框架已搭建完成。下一个阶段将实现微信解密模块 (WeChatDecoder)。
