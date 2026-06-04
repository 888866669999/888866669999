#ifndef WECHATADAPTER_H
#define WECHATADAPTER_H

#include <QObject>
#include <QIcon>
#include <QListWidget>
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
    QList<MediaFile> getMediaFilesForMessage(const QString& msgId) override;
    QPixmap decryptImage(const MediaFile& mediaFile) override;
    QPixmap decryptImage(const QString& datPath);

signals:
    void newMessageReceived(const ChatMessage& msg);
    void errorOccurred(const QString& error);

public:
    // 进程捕捉和列表刷新（供 MainWindow 调用）
    void refreshProcessList(QListWidget* listWidget);
    QString captureProcessInstallPath();

private:
    QStringList findWeChatDataDirs();
    void refreshProcessListInner(QListWidget* listWidget);
    QString captureProcessInstallPathInner();

    WeChatDecoder* m_decoder;
    QStringList m_keys;
    QString m_dataDir;
    bool m_initialized;
    QIcon m_platformIcon;
    bool m_decoderInitialized;  // 防止重复初始化
};

#endif
