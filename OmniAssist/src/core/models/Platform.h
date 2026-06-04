#ifndef PLATFORM_H
#define PLATFORM_H

#include <QString>

enum class Platform {
    WeChat,
    QQ,
    DingTalk,
    Unknown
};

inline QString platformToString(Platform platform) {
    switch (platform) {
        case Platform::WeChat: return "微信";
        case Platform::QQ: return "QQ";
        case Platform::DingTalk: return "钉钉";
        default: return "未知";
    }
}

#endif
