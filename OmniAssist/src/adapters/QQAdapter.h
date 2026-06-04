#ifndef QQADAPTER_H
#define QQADAPTER_H

#include <QObject>
#include <QIcon>
#include "IPlatformAdapter.h"
#include "../core/QQDecoder.h"

class QQAdapter : public QObject, public IPlatformAdapter {
    Q_OBJECT
    Q_INTERFACES(IPlatformAdapter)

public:
    explicit QQAdapter(QObject* parent = nullptr);
    ~QQAdapter();

    Platform platform() const override { return Platform::QQ; }
    QString name() const override { return "QQ"; }
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
    
    void setUseAPI(bool useAPI);
    bool connectToNapCat(const QString& host, int port);

signals:
    void newMessageReceived(const ChatMessage& msg);
    void errorOccurred(const QString& error);

private:
    QStringList findQQDataDirs();

    QQDecoder* m_decoder;
    QStringList m_keys;
    QString m_dataDir;
    bool m_initialized;
    bool m_useAPI;
    QIcon m_platformIcon;
};

#endif
