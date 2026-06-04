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
