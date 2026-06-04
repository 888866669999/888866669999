#ifndef QQADAPTER_H
#define QQADAPTER_H

#include <QObject>
#include <QIcon>
#include <QListWidget>
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

public:
    // 进程捕捉和列表刷新（供 MainWindow 调用）
    void refreshProcessList(QListWidget* listWidget);
    QString captureProcessInstallPath();

private:
    QStringList findQQDataDirs();
    void refreshProcessListInner(QListWidget* listWidget);
    QString captureProcessInstallPathInner();

    QQDecoder* m_decoder;
    QStringList m_keys;
    QString m_dataDir;
    bool m_initialized;
    bool m_useAPI;
    QIcon m_platformIcon;
};

#endif
