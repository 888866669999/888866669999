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
    QStringList* m_foundKeys;
    bool scanProcessMemory(void* processHandle);
    QByteArray readProcessMemory(void* processHandle, void* address, size_t size);
    bool isValidKey(const QString& key, const QString& dbPath);
    QStringList findWeChatProcesses();
};

#endif
