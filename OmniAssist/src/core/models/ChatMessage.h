#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QVariant>
#include <QList>
#include "Platform.h"
#include "MediaFile.h"

enum class MessageType {
    Text,
    Image,
    File,
    Audio,
    Video,
    System,
    Unknown
};

struct ChatMessage {
    QString id;
    qint64 timestamp;
    QString senderId;
    QString senderName;
    QString content;
    MessageType type;
    bool isSelf;
    QList<MediaFile> mediaFiles;
    QVariant extra;
};

Q_DECLARE_METATYPE(ChatMessage)

#endif
