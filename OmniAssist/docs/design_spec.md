# OmniAssist 设计规范

## 项目概述

**项目名称**: OmniAssist (全平台即时通讯智能中枢)

**技术栈**: Qt 6.5+, C++17, CMake, Windows API

**目标平台**: Windows 10/11

---

## 1. 系统架构

### 1.1 整体架构图

```
┌─────────────────────────────────────────────────────────┐
│                    UI Layer (Qt Widgets)                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  MainWindow  │  │ ContactList  │  │   ChatView   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Core Services Layer                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ DatabaseMgr  │  │  AIService   │  │   Monitor    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                Platform Adapter Layer                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │WeChatAdapter │  │  QQAdapter   │  │DingTalkAdapter│ │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ↑ 统一接口  ↑ 统一接口  ↑ 统一接口                       │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│              Low-Level Layer (Windows)                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │MemoryScanner │  │ SQLCipher    │  │   UIAutomation │ │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 1.2 核心设计原则

1. **平台适配器接口统一**: 所有平台适配器实现相同的抽象基类接口
2. **只读数据库操作**: 所有数据库访问均为只读，避免修改原数据
3. **模块化设计**: 各模块低耦合，通过明确的接口通信
4. **错误恢复机制**: 数据库锁定自动重试，内存扫描失败有 fallback 方案

---

## 2. 核心数据模型

### 2.1 平台枚举

```cpp
enum class Platform {
    WeChat,    // 微信
    QQ,        // QQ
    DingTalk,  // 钉钉
    Unknown
};
```

### 2.2 消息类型枚举

```cpp
enum class MessageType {
    Text,     // 文本
    Image,    // 图片
    File,     // 文件
    Audio,    // 语音
    Video,    // 视频
    System,   // 系统消息
    Unknown
};
```

### 2.3 联系人结构

```cpp
struct Contact {
    QString id;              // 联系人唯一ID
    QString name;            // 联系人名称
    QString remark;          // 备注名
    Platform platform;       // 所属平台
    QIcon platformIcon;      // 平台图标
    QPixmap avatar;          // 联系人头像
    QVariant extra;          // 平台特定的额外数据
};
```

### 2.4 聊天消息结构

```cpp
struct ChatMessage {
    QString id;              // 消息唯一ID
    qint64 timestamp;        // 时间戳（毫秒）
    QString senderId;        // 发送者ID
    QString senderName;      // 发送者名称
    QString content;         // 消息内容
    MessageType type;        // 消息类型
    bool isSelf;             // 是否是自己发送的
    QVariant extra;          // 平台特定的额外数据
};
```

---

## 3. 平台适配器接口规范

### 3.1 适配器基类 (IPlatformAdapter)

```cpp
class IPlatformAdapter {
public:
    virtual ~IPlatformAdapter() = default;

    // ========== 基本信息 ==========
    
    /**
     * @brief 获取平台类型
     */
    virtual Platform platform() const = 0;
    
    /**
     * @brief 获取平台名称（用于显示）
     */
    virtual QString name() const = 0;
    
    /**
     * @brief 获取平台图标
     */
    virtual QIcon icon() const = 0;
    
    /**
     * @brief 检测该平台是否安装并正在运行
     * @return true 表示可用
     */
    virtual bool isAvailable() const = 0;

    // ========== 数据库操作 ==========
    
    /**
     * @brief 初始化适配器（包括密钥提取、数据库解密等）
     * @return 初始化是否成功
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 获取所有联系人列表
     * @return 联系人列表
     */
    virtual QList<Contact> getContacts() = 0;
    
    /**
     * @brief 获取指定联系人的聊天历史
     * @param contactId 联系人ID
     * @param limit 最大消息数量
     * @return 聊天消息列表（按时间从新到旧排序）
     */
    virtual QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) = 0;
    
    /**
     * @brief 获取所有联系人的最近消息
     * @param limit 每个联系人的最大消息数
     * @return 最近消息列表
     */
    virtual QList<ChatMessage> getRecentMessages(int limit = 50) = 0;

    // ========== 实时监控 ==========
    
    /**
     * @brief 启动实时消息监控
     * @return 启动是否成功
     */
    virtual bool startMonitoring() = 0;
    
    /**
     * @brief 停止实时消息监控
     */
    virtual void stopMonitoring() = 0;

    // ========== 信号（需在实现中定义） ==========
    // void newMessageReceived(const ChatMessage& msg);
    // void errorOccurred(const QString& error);
};
```

### 3.2 适配器实现要求

每个平台适配器必须：
1. 继承自 `IPlatformAdapter`
2. 实现所有纯虚函数
3. 提供线程安全的实现
4. 所有数据库操作为只读
5. 包含详细的中文注释

---

## 4. 目录结构

```
OmniAssist/
├── CMakeLists.txt
├── docs/
│   ├── design_spec.md                # 本文档
│   ├── adapter_interface.md          # 适配器接口详细文档
│   └── platform_specific_notes/      # 各平台技术细节
│       ├── wechat_notes.md
│       ├── qq_notes.md
│       └── dingtalk_notes.md
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── DatabaseManager.h/cpp     # 数据库路由管理器
│   │   ├── WeChatDecoder.h/cpp       # 微信解密核心
│   │   ├── Monitor.h/cpp             # 文件监听引擎
│   │   └── models/
│   │       ├── ChatMessage.h
│   │       ├── Contact.h
│   │       └── Platform.h
│   ├── adapters/
│   │   ├── IPlatformAdapter.h        # 适配器基类
│   │   ├── WeChatAdapter.h/cpp
│   │   ├── QQAdapter.h/cpp
│   │   └── DingTalkAdapter.h/cpp
│   ├── automation/
│   │   ├── WindowFinder.h/cpp        # 窗口查找
│   │   └── MessageSender.h/cpp       # 消息发送
│   ├── ai/
│   │   ├── AIService.h/cpp           # AI 服务
│   │   ├── Prompts.h                 # 提示词常量
│   │   └── providers/
│   │       ├── IAIServiceProvider.h
│   │       ├── OpenAIProvider.h/cpp
│   │       ├── QwenProvider.h/cpp    # 通义千问
│   │       └── WenxinProvider.h/cpp  # 文心一言
│   └── ui/
│       ├── MainWindow.h/cpp
│       ├── ContactListWidget.h/cpp
│       └── ChatViewWidget.h/cpp
├── third_party/                      # 第三方库
│   └── sqlcipher/
└── resources/
    ├── icons/
    │   ├── wechat.png
    │   ├── qq.png
    │   └── dingtalk.png
    └── resources.qrc
```

---

## 5. 核心模块设计

### 5.1 WeChatDecoder（微信解密模块）

```cpp
class WeChatDecoder {
public:
    /**
     * @brief 从微信进程内存中提取 SQLCipher 密钥
     * @param keys 输出参数，提取到的密钥列表
     * @return 是否成功提取到至少一个密钥
     */
    bool extractKeysFromMemory(QStringList* keys);
    
    /**
     * @brief 解密 SQLCipher 4 加密的数据库
     * @param dbPath 原始数据库路径
     * @param outputPath 解密后输出路径
     * @param key 加密密钥
     * @return 解密是否成功
     */
    bool decryptDatabase(const QString& dbPath, 
                         const QString& outputPath,
                         const QString& key);
    
    /**
     * @brief 自动查找微信数据目录
     * @return 数据目录路径，失败返回空字符串
     */
    QString findWeChatDataDir();
    
    /**
     * @brief 验证密钥是否正确
     */
    bool verifyKey(const QString& dbPath, const QString& key);

private:
    bool scanProcessMemory(HANDLE hProcess);
    QByteArray readProcessMemory(HANDLE hProcess, LPVOID address, SIZE_T size);
};
```

### 5.2 AIService（AI 服务模块）

```cpp
class IAIServiceProvider {
public:
    virtual ~IAIServiceProvider() = default;
    
    /**
     * @brief 基于上下文生成自动回复
     */
    virtual QString generateReply(const QList<ChatMessage>& context) = 0;
    
    /**
     * @brief 生成会议纪要和待办事项（JSON 格式）
     */
    virtual QJsonObject generateSummary(const QList<ChatMessage>& context) = 0;
    
    /**
     * @brief 分析对方性格、沟通风格和关系亲密度
     */
    virtual QString analyzePersona(const QList<ChatMessage>& history) = 0;
    
    /**
     * @brief 安全过滤（所有输出必须经过过滤）
     */
    virtual QString safetyFilter(const QString& input) = 0;
};

class AIService {
public:
    void setProvider(IAIServiceProvider* provider);
    
    QString autoReply(const QList<ChatMessage>& context);
    QJsonObject generateMeetingSummary(const QList<ChatMessage>& context);
    QString analyzeContactPersona(const QString& contactId);
};
```

### 5.3 MessageSender（自动化发送模块）

```cpp
class MessageSender {
public:
    /**
     * @brief 发送文本消息
     * @param platform 目标平台
     * @param contactId 目标联系人ID
     * @param text 消息内容
     * @return 发送是否成功
     */
    bool sendText(Platform platform, const QString& contactId, const QString& text);
    
    /**
     * @brief 发送文件或图片
     */
    bool sendFile(Platform platform, const QString& contactId, const QString& filePath);

private:
    /**
     * @brief 添加随机延迟（1-3秒），用于风控规避
     */
    void addRandomDelay();
    
    /**
     * @brief 模拟鼠标移动
     */
    void simulateMouseMovement();
    
    /**
     * @brief 通过剪贴板注入文本
     */
    bool sendTextViaClipboard(HWND hwnd, const QString& text);
};
```

---

## 6. UI 设计

### 6.1 主窗口布局

```
┌──────────────────────────────────────────────────────────┐
│ OmniAssist - 全平台即时通讯智能中枢                   [X] │
├──────────────┬───────────────────────────────────────────┤
│  联系人列表   │           聊天视图                       │
│              │                                           │
│ [图标] 张三  │  10:30                                    │
│ [图标] 李四  │  张三: 你好                               │
│ [图标] 王五  │  我: 你好呀！                             │
│ [图标] 群聊  │  张三: 最近怎么样？                       │
│              │  ...                                      │
│              │                                           │
│              ├───────────────────────────────────────────┤
│              │  [AI 回复] [生成摘要] [人物分析]           │
├──────────────┴───────────────────────────────────────────┤
│  API 配置: [OpenAI ▼] [key: ___________] [自动回复: ✓]    │
└──────────────────────────────────────────────────────────┘
```

---

## 7. 实现步骤

### 第一步：项目骨架搭建
- CMakeLists.txt
- 目录结构和基础头文件
- 数据模型定义
- 适配器基类

### 第二步：微信解密核心实现
- WeChatDecoder 完整实现
- Windows 进程内存扫描
- SQLCipher 数据库解密

### 第三步：基础 UI 框架
- MainWindow 实现
- 联系人列表（带图标）
- 聊天视图
- 配置面板

### 第四步：AI 服务集成
- AIService 实现
- 多 AI 提供商支持
- Prompt 模板

### 第五步：自动化发送模块
- WindowFinder 和 MessageSender
- UI Automation 集成
- 风控规避机制

---

## 8. 安全规范

1. **内存安全**: 所有数据库操作为只读
2. **密钥安全**: 密钥不写入磁盘，仅在内存中临时存储
3. **安全过滤**: AI 所有输出必须经过安全过滤
4. **风控规避**: 发送消息前添加随机延迟和模拟鼠标操作
5. **错误处理**: 数据库锁定时自动等待并重试

---

## 附录：参考文档

- [wechat-decrypt.md](../../wechat-decrypt.md) - 微信解密技术细节
- [napcatqq.md](../../napcatqq.md) - QQ NTQQ 协议参考
- [qq-chat-exporter.md](../../qq-chat-exporter.md) - QQ 消息导出参考
