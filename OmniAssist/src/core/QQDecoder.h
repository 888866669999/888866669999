#ifndef QQDECODER_H
#define QQDECODER_H

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QList>
#include <QDateTime>
#include <QVariant>

struct QQContact {
    QString id;
    QString name;
    QString remark;
    QString avatarPath;
    QDateTime createTime;
    QVariant extra;
    bool isGroup = false;
};

struct QQMessage {
    QString id;
    QString talkerId;
    QString content;
    qint64 createTime;
    int type;
    bool isSelf;
    QString senderId;
    QString senderName;
    QVariant extra;
};

class QQDecoder {
public:
    QQDecoder();
    ~QQDecoder();

    QStringList extractKeysFromMemory();
    QString findQQDataDir();
    bool decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key);

    QList<QQContact> getAllContacts(const QString& dbPath, const QString& key);
    QList<QQMessage> getChatHistory(const QString& dbPath, const QString& key, 
                                    const QString& talkerId, int limit = 100);
    
    bool connectToNapCat(const QString& host, int port);
    QList<QQContact> getContactsFromAPI();
    QList<QQMessage> getMessagesFromAPI(const QString& talkerId, int limit);

private:
    QStringList findQQProcesses();
    bool scanProcessMemory(void* processHandle, QStringList* foundKeys);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);
};

#endif
