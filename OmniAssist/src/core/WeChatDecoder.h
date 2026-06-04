#ifndef WECHATDECODER_H
#define WECHATDECODER_H

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QList>
#include <QDateTime>
#include <QVariant>
#include <QByteArray>

struct WeChatContact {
    QString id;
    QString name;
    QString remark;
    QString alias;
    QString avatarPath;
    QDateTime createTime;
    QVariant extra;
};

struct WeChatMessage {
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

class WeChatDecoder {
public:
    WeChatDecoder();
    ~WeChatDecoder();

    QStringList extractKeysFromMemory();
    bool decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key);
    QString findWeChatDataDir();
    bool verifyKey(const QString& dbPath, const QString& key);

    QPixmap decryptImage(const QString& datPath);

    QList<WeChatContact> getAllContacts(const QString& dbPath, const QString& key);
    QList<WeChatMessage> getChatHistory(const QString& dbPath, const QString& key,
                                         const QString& talkerId, int limit = 100);

private:
    bool scanProcessMemory(void* processHandle, QStringList* foundKeys);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);
    bool isValidKey(const QString& key, const QString& dbPath);
    QStringList findWeChatProcesses();

    QByteArray deriveKey(const QByteArray& password, const QByteArray& salt, int iterations, int dklen);
    QByteArray hmacSha1(const QByteArray& key, const QByteArray& data);
    QByteArray hmacSha512(const QByteArray& key, const QByteArray& data);
    QByteArray aes256CbcDecrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& data);
};

#endif
