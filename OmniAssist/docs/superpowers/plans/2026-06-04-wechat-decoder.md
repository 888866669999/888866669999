# OmniAssist 微信解密模块 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 WeChatDecoder 模块，包括从微信进程内存中提取 SQLCipher 密钥和数据库解密功能。

**Architecture:** 独立的 WeChatDecoder 类，提供静态方法供 WeChatAdapter 调用。

**Tech Stack:** Qt 6.5+, C++17, Windows API (进程内存读取), SQLCipher

---

## File Structure

```
OmniAssist/
├── src/
│   └── core/
│       ├── WeChatDecoder.h
│       └── WeChatDecoder.cpp
└── third_party/
    └── sqlcipher/        (预留)
```

---

### Task 1: WeChatDecoder 基本结构

**Files:**
- Create: `OmniAssist/src/core/WeChatDecoder.h`
- Create: `OmniAssist/src/core/WeChatDecoder.cpp`
- Modify: `OmniAssist/CMakeLists.txt`

- [ ] **Step 1: 编写 WeChatDecoder.h**

```cpp
#ifndef WECHATDECODER_H
#define WECHATDECODER_H

#include <QString>
#include <QStringList>
#include <QPixmap>

class WeChatDecoder {
public:
    WeChatDecoder();
    ~WeChatDecoder();

    bool extractKeysFromMemory(QStringList* keys);
    bool decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key);
    QString findWeChatDataDir();
    bool verifyKey(const QString& dbPath, const QString& key);

    QPixmap decryptImage(const QString& datPath);

private:
    bool scanProcessMemory(void* processHandle);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);
    bool isValidKey(const QString& key, const QString& dbPath);
    QStringList findWeChatProcesses();
};

#endif
```

- [ ] **Step 2: 编写 WeChatDecoder.cpp 基础框架**

```cpp
#include "WeChatDecoder.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

WeChatDecoder::WeChatDecoder() {
}

WeChatDecoder::~WeChatDecoder() {
}

QString WeChatDecoder::findWeChatDataDir() {
    QStringList possiblePaths;

    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    possiblePaths << docPath + "/WeChat Files";
    possiblePaths << docPath + "/Tencent Files/WeChat";

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            return path;
        }
    }

    return QString();
}

bool WeChatDecoder::extractKeysFromMemory(QStringList* keys) {
    Q_UNUSED(keys);
    qWarning() << "extractKeysFromMemory not implemented yet";
    return false;
}

bool WeChatDecoder::decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key) {
    Q_UNUSED(dbPath);
    Q_UNUSED(outputPath);
    Q_UNUSED(key);
    qWarning() << "decryptDatabase not implemented yet";
    return false;
}

bool WeChatDecoder::verifyKey(const QString& dbPath, const QString& key) {
    Q_UNUSED(dbPath);
    Q_UNUSED(key);
    qWarning() << "verifyKey not implemented yet";
    return false;
}

QPixmap WeChatDecoder::decryptImage(const QString& datPath) {
    Q_UNUSED(datPath);
    qWarning() << "decryptImage not implemented yet";
    return QPixmap();
}
```

- [ ] **Step 3: 更新 CMakeLists.txt**

修改 `OmniAssist/CMakeLists.txt`，添加新文件到 SOURCES 和 HEADERS：

```cmake
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/core/WeChatDecoder.cpp
)

set(HEADERS
    src/core/models/Platform.h
    src/core/models/Contact.h
    src/core/models/MediaFile.h
    src/core/models/ChatMessage.h
    src/adapters/IPlatformAdapter.h
    src/ui/MainWindow.h
    src/core/WeChatDecoder.h
)
```

- [ ] **Step 4: 编译验证**

```bash
cd /workspace/OmniAssist/build && cmake .. && cmake --build .
```
Expected: 编译成功

- [ ] **Step 5: Commit**

```bash
cd /workspace/OmniAssist
git add src/core/WeChatDecoder.h src/core/WeChatDecoder.cpp CMakeLists.txt
git commit -m "feat: add WeChatDecoder skeleton"
```

---

### Task 2: 实现微信进程查找

**Files:**
- Modify: `OmniAssist/src/core/WeChatDecoder.cpp`

- [ ] **Step 1: 添加 Windows 头文件**

在文件顶部添加：

```cpp
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
```

- [ ] **Step 2: 实现 findWeChatProcesses() 私有方法**

```cpp
QStringList WeChatDecoder::findWeChatProcesses() {
    QStringList processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            if (processName.compare("WeChat.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("WeChatWin.dll", Qt::CaseInsensitive) == 0) {
                processes << QString::number(pe32.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
}
```

- [ ] **Step 3: 更新 extractKeysFromMemory 以调用进程查找**

```cpp
bool WeChatDecoder::extractKeysFromMemory(QStringList* keys) {
    if (!keys) return false;

    QStringList pids = findWeChatProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No WeChat process found";
        return false;
    }

    qDebug() << "Found WeChat processes:" << pids;

    for (const QString& pidStr : pids) {
        DWORD pid = pidStr.toULong();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        
        if (hProcess) {
            if (scanProcessMemory(hProcess)) {
            }
            CloseHandle(hProcess);
        }
    }

    return !keys->isEmpty();
}
```

- [ ] **Step 4: 编译验证**

```bash
cd /workspace/OmniAssist/build && cmake --build .
```
Expected: 编译成功

- [ ] **Step 5: Commit**

```bash
cd /workspace/OmniAssist
git add src/core/WeChatDecoder.cpp
git commit -m "feat: add WeChat process detection"
```

---

### Task 3: 实现进程内存扫描

**Files:**
- Modify: `OmniAssist/src/core/WeChatDecoder.cpp`
- Modify: `OmniAssist/src/core/WeChatDecoder.h`

- [ ] **Step 1: 添加成员变量存储找到的密钥**

在 `WeChatDecoder.h` 的 private 部分添加：

```cpp
private:
    QStringList* m_foundKeys;
    bool scanProcessMemory(void* processHandle);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);
    bool isValidKey(const QString& key, const QString& dbPath);
    QStringList findWeChatProcesses();
```

- [ ] **Step 2: 实现 readProcessMemory**

```cpp
QByteArray WeChatDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    QByteArray buffer(size, 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    
    return QByteArray();
}
```

- [ ] **Step 3: 实现 scanProcessMemory 扫描密钥特征**

```cpp
bool WeChatDecoder::scanProcessMemory(void* processHandle) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* address = reinterpret_cast<unsigned char*>(sysInfo.lpMinimumApplicationAddress);
    
    while (address < reinterpret_cast<unsigned char*>(sysInfo.lpMaximumApplicationAddress)) {
        if (VirtualQueryEx(processHandle, address, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                QByteArray memory = readProcessMemory(processHandle, mbi.BaseAddress, mbi.RegionSize);
                
                if (!memory.isEmpty()) {
                    QByteArray pattern1 = "x'";
                    int pos = 0;
                    while ((pos = memory.indexOf(pattern1, pos)) != -1) {
                        if (pos + 64 + 32 + 2 < memory.size()) {
                            QByteArray candidate = memory.mid(pos + 2, 64 + 32);
                            bool isHex = true;
                            for (int i = 0; i < candidate.size(); ++i) {
                                char c = candidate[i];
                                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                                    isHex = false;
                                    break;
                                }
                            }
                            if (isHex && candidate.size() == 96) {
                                QString key = "x'" + candidate + "'";
                                if (m_foundKeys && !m_foundKeys->contains(key)) {
                                    m_foundKeys->append(key);
                                    qDebug() << "Found potential key:" << key.left(20) << "...";
                                }
                            }
                        }
                        pos += 2;
                    }
                }
            }
            address = static_cast<unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
        } else {
            address += 0x1000;
        }
    }

    return m_foundKeys && !m_foundKeys->isEmpty();
}
```

- [ ] **Step 4: 更新 extractKeysFromMemory 传递指针**

```cpp
bool WeChatDecoder::extractKeysFromMemory(QStringList* keys) {
    if (!keys) return false;
    m_foundKeys = keys;
    keys->clear();

    QStringList pids = findWeChatProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No WeChat process found";
        return false;
    }

    qDebug() << "Found WeChat processes:" << pids;

    for (const QString& pidStr : pids) {
        DWORD pid = pidStr.toULong();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        
        if (hProcess) {
            scanProcessMemory(hProcess);
            CloseHandle(hProcess);
        }
    }

    return !keys->isEmpty();
}
```

- [ ] **Step 5: 编译验证**

```bash
cd /workspace/OmniAssist/build && cmake --build .
```
Expected: 编译成功

- [ ] **Step 6: Commit**

```bash
cd /workspace/OmniAssist
git add src/core/WeChatDecoder.h src/core/WeChatDecoder.cpp
git commit -m "feat: implement process memory scanning for SQLCipher keys"
```

---

### Task 4: 实现 WeChatAdapter 框架

**Files:**
- Create: `OmniAssist/src/adapters/WeChatAdapter.h`
- Create: `OmniAssist/src/adapters/WeChatAdapter.cpp`
- Modify: `OmniAssist/CMakeLists.txt`

- [ ] **Step 1: 编写 WeChatAdapter.h**

```cpp
#ifndef WECHATADAPTER_H
#define WECHATADAPTER_H

#include <QObject>
#include "IPlatformAdapter.h"
#include "../core/WeChatDecoder.h"

class WeChatAdapter : public QObject, public IPlatformAdapter {
    Q_OBJECT
    Q_INTERFACES(IPlatformAdapter)

public:
    explicit WeChatAdapter(QObject* parent = nullptr);
    ~WeChatAdapter();

    Platform platform() const override { return Platform::WeChat; }
    QString name() const override { return "微信"; }
    QIcon icon() const override;
    bool isAvailable() const override;

    bool initialize() override;
    QList<Contact> getContacts() override;
    QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) override;
    QList<ChatMessage> getRecentMessages(int limit = 50) override;

    bool startMonitoring() override;
    void stopMonitoring() override;

    bool supportsRichMedia() const override { return true; }

signals:
    void newMessageReceived(const ChatMessage& msg);
    void errorOccurred(const QString& error);

private:
    WeChatDecoder* m_decoder;
    QStringList m_keys;
    QString m_dataDir;
    bool m_initialized;
};

#endif
```

- [ ] **Step 2: 编写 WeChatAdapter.cpp**

```cpp
#include "WeChatAdapter.h"
#include <QIcon>
#include <QDebug>

WeChatAdapter::WeChatAdapter(QObject* parent) 
    : QObject(parent), m_decoder(new WeChatDecoder()), m_initialized(false) {
}

WeChatAdapter::~WeChatAdapter() {
    delete m_decoder;
}

QIcon WeChatAdapter::icon() const {
    return QIcon();
}

bool WeChatAdapter::isAvailable() const {
    return !m_decoder->findWeChatDataDir().isEmpty();
}

bool WeChatAdapter::initialize() {
    if (m_initialized) return true;

    m_dataDir = m_decoder->findWeChatDataDir();
    if (m_dataDir.isEmpty()) {
        emit errorOccurred("找不到微信数据目录");
        return false;
    }

    qDebug() << "WeChat data dir:" << m_dataDir;

    if (!m_decoder->extractKeysFromMemory(&m_keys)) {
        emit errorOccurred("无法从微信进程提取密钥，请确保微信正在运行");
        return false;
    }

    qDebug() << "Extracted" << m_keys.size() << "keys";

    m_initialized = true;
    return true;
}

QList<Contact> WeChatAdapter::getContacts() {
    QList<Contact> contacts;
    qDebug() << "getContacts not implemented yet";
    return contacts;
}

QList<ChatMessage> WeChatAdapter::getChatHistory(const QString& contactId, int limit) {
    Q_UNUSED(contactId);
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    qDebug() << "getChatHistory not implemented yet";
    return messages;
}

QList<ChatMessage> WeChatAdapter::getRecentMessages(int limit) {
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    qDebug() << "getRecentMessages not implemented yet";
    return messages;
}

bool WeChatAdapter::startMonitoring() {
    qDebug() << "startMonitoring not implemented yet";
    return false;
}

void WeChatAdapter::stopMonitoring() {
    qDebug() << "stopMonitoring not implemented yet";
}
```

- [ ] **Step 3: 更新 CMakeLists.txt**

添加 WeChatAdapter 到 SOURCES 和 HEADERS。

- [ ] **Step 4: 编译验证**

```bash
cd /workspace/OmniAssist/build && cmake --build .
```
Expected: 编译成功

- [ ] **Step 5: Commit**

```bash
cd /workspace/OmniAssist
git add src/adapters/WeChatAdapter.h src/adapters/WeChatAdapter.cpp CMakeLists.txt
git commit -m "feat: add WeChatAdapter skeleton"
```

---

## 第二阶段阶段性完成！

微信解密模块的核心框架已完成。下一个阶段将实现 SQLCipher 数据库解密和数据解析功能。
