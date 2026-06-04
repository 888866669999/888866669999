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

    bool extractKeysFromMemory(QStringList* keys);
    QString findQQDataDir();

    QList<QQContact> getAllContacts(const QString& dbPath, const QString& key);
    QList<QQMessage> getChatHistory(const QString& dbPath, const QString& key, 
                                    const QString& talkerId, int limit = 100);

private:
    QStringList* m_foundKeys;
    QStringList findQQProcesses();
    bool scanProcessMemory(void* processHandle);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);

    QString m_lastDbPath;
    QString m_lastKey;
};

#endif
