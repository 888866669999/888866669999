# 平台适配器接口详细文档

## 概述

本文档详细描述了 OmniAssist 项目中平台适配器的接口规范，供开发者参考实现新的平台适配器。

---

## IPlatformAdapter 接口详解

### 基本信息接口

#### platform()
```cpp
virtual Platform platform() const = 0;
```
- **功能**: 返回适配器对应的平台类型
- **返回值**: Platform 枚举值（WeChat/QQ/DingTalk）
- **实现要求**: 必须是线程安全的，返回值恒定不变

#### name()
```cpp
virtual QString name() const = 0;
```
- **功能**: 返回平台名称，用于 UI 显示
- **返回值**: 例如 "微信"、"QQ"、"钉钉"
- **实现要求**: 建议使用本地化字符串

#### icon()
```cpp
virtual QIcon icon() const = 0;
```
- **功能**: 返回平台图标
- **返回值**: QIcon 对象，建议从资源文件加载
- **实现要求**: 图标尺寸建议 32x32 或 64x64

#### isAvailable()
```cpp
virtual bool isAvailable() const = 0;
```
- **功能**: 检测该平台是否已安装并正在运行
- **返回值**: true 表示可以使用
- **实现要求**: 
  - 检查进程是否存在
  - 检查数据目录是否存在
  - 必须是线程安全的

---

### 数据库操作接口

#### initialize()
```cpp
virtual bool initialize() = 0;
```
- **功能**: 初始化适配器，包括密钥提取、数据库解密等
- **返回值**: 初始化是否成功
- **实现要求**:
  - 幂等设计，重复调用应安全
  - 可能需要管理员权限
  - 超时处理建议
  - 错误信息通过信号 errorOccurred() 发送

#### getContacts()
```cpp
virtual QList<Contact> getContacts() = 0;
```
- **功能**: 获取所有联系人列表
- **返回值**: 联系人列表
- **实现要求**:
  - 按名称排序
  - 包含平台图标
  - 只读操作
  - 缓存机制建议

#### getChatHistory()
```cpp
virtual QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) = 0;
```
- **功能**: 获取指定联系人的聊天历史
- **参数**:
  - contactId: 联系人唯一标识
  - limit: 最大返回消息数
- **返回值**: 聊天消息列表，按时间从新到旧排序
- **实现要求**:
  - 支持分页（可选扩展）
  - 消息类型正确解析
  - 标记是否是自己发送的

#### getRecentMessages()
```cpp
virtual QList<ChatMessage> getRecentMessages(int limit = 50) = 0;
```
- **功能**: 获取所有联系人的最近消息
- **参数**: limit 每个联系人的最大消息数
- **返回值**: 最近消息列表
- **实现要求**: 按时间戳排序

---

### 实时监控接口

#### startMonitoring()
```cpp
virtual bool startMonitoring() = 0;
```
- **功能**: 启动实时消息监控
- **返回值**: 启动是否成功
- **实现要求**:
  - 使用 QFileSystemWatcher 监听 WAL 文件
  - 检测到新消息时发出 newMessageReceived() 信号
  - 避免重复启动

#### stopMonitoring()
```cpp
virtual void stopMonitoring() = 0;
```
- **功能**: 停止实时消息监控
- **实现要求**:
  - 清理资源
  - 可以重复调用

---

### 富媒体支持接口（可选功能）

#### supportsRichMedia()
```cpp
virtual bool supportsRichMedia() const { return false; }
```
- **功能**: 指示该平台适配器是否支持富媒体文件解析
- **返回值**: true 表示支持，false 表示不支持
- **实现要求**:
  - 默认返回 false
  - 如果支持富媒体，重写此方法返回 true

#### getMediaFilesForMessage()
```cpp
virtual QList<MediaFile> getMediaFilesForMessage(const QString& msgId) {
    Q_UNUSED(msgId);
    return {};
}
```
- **功能**: 获取指定消息的富媒体文件信息
- **参数**: msgId 消息唯一标识
- **返回值**: 富媒体文件列表
- **实现要求**:
  - 仅在 supportsRichMedia() 返回 true 时调用
  - 包含图片、表情、文件等
  - 只读操作

#### decryptImage()
```cpp
virtual QPixmap decryptImage(const MediaFile& mediaFile) {
    Q_UNUSED(mediaFile);
    return QPixmap();
}
```
- **功能**: 解密并获取图片数据
- **参数**: mediaFile 媒体文件信息
- **返回值**: 解密后的图片，失败返回空 QPixmap
- **实现要求**:
  - 处理平台特定的图片加密格式
  - 对于微信 4.x，需要处理 .dat 图片解密

---

### 信号（必须在实现中定义）

#### newMessageReceived
```cpp
void newMessageReceived(const ChatMessage& msg);
```
- **触发时机**: 检测到新消息时
- **参数**: 新消息对象

#### errorOccurred
```cpp
void errorOccurred(const QString& error);
```
- **触发时机**: 发生错误时
- **参数**: 错误描述字符串

---

## 实现检查清单

创建新适配器时，请确保以下各项：

### 基本功能（必须）
- [ ] 继承 IPlatformAdapter
- [ ] 实现所有纯虚函数
- [ ] 使用 Q_OBJECT 宏（如果需要信号）
- [ ] 所有数据库操作为只读
- [ ] 包含详细的中文注释
- [ ] 提供线程安全的实现
- [ ] 处理数据库锁定情况（自动重试）
- [ ] 实现错误处理和报告
- [ ] 包含平台图标
- [ ] 更新 docs/platform_specific_notes/ 下的对应文档

### 富媒体支持（可选）
- [ ] 重写 supportsRichMedia() 返回 true
- [ ] 实现 getMediaFilesForMessage() 解析富媒体文件
- [ ] 实现 decryptImage() 解密图片数据
- [ ] 处理表情、图片、文件等不同类型的富媒体

---

## 示例实现框架

```cpp
class WeChatAdapter : public QObject, public IPlatformAdapter {
    Q_OBJECT
    Q_INTERFACES(IPlatformAdapter)

public:
    explicit WeChatAdapter(QObject* parent = nullptr);
    ~WeChatAdapter() override;

    // 基本信息
    Platform platform() const override { return Platform::WeChat; }
    QString name() const override { return "微信"; }
    QIcon icon() const override;
    bool isAvailable() const override;

    // 数据库操作
    bool initialize() override;
    QList<Contact> getContacts() override;
    QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) override;
    QList<ChatMessage> getRecentMessages(int limit = 50) override;

    // 实时监控
    bool startMonitoring() override;
    void stopMonitoring() override;

signals:
    void newMessageReceived(const ChatMessage& msg) override;
    void errorOccurred(const QString& error) override;

private:
    WeChatDecoder* m_decoder;
    QFileSystemWatcher* m_watcher;
    // ...
};
```
