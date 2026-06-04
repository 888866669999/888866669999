#ifndef PLATFORM_H
#define PLATFORM_H

#include <QString>
#include <QMetaType>

enum class Platform {
    WeChat,
    QQ,
    DingTalk,
    Unknown
};
Q_DECLARE_METATYPE(Platform)

enum class MessageType {
    Text,
    Image,
    Audio,
    Video,
    File,
    Unknown
};
Q_DECLARE_METATYPE(MessageType)

inline QString platformToString(Platform platform) {
    switch (platform) {
        case Platform::WeChat: return "微信";
        case Platform::QQ: return "QQ";
        case Platform::DingTalk: return "钉钉";
        default: return "未知";
    }
}

#endif
