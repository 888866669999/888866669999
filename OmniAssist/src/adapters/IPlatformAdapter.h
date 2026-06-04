#ifndef IPLATFORMADAPTER_H
#define IPLATFORMADAPTER_H

#include <QString>
#include <QList>
#include <QIcon>
#include <QPixmap>
#include <QObject>
#include "../core/models/Platform.h"
#include "../core/models/Contact.h"
#include "../core/models/ChatMessage.h"
#include "../core/models/MediaFile.h"

class IPlatformAdapter {
public:
    virtual ~IPlatformAdapter() = default;

    virtual Platform platform() const = 0;
    virtual QString name() const = 0;
    virtual QIcon icon() const = 0;
    virtual bool isAvailable() const = 0;

    virtual bool initialize() = 0;
    virtual QList<Contact> getContacts() = 0;
    virtual QList<ChatMessage> getChatHistory(const QString& contactId, int limit = 100) = 0;
    virtual QList<ChatMessage> getRecentMessages(int limit = 50) = 0;

    virtual bool startMonitoring() = 0;
    virtual void stopMonitoring() = 0;

    virtual bool supportsRichMedia() const { return false; }
    virtual QList<MediaFile> getMediaFilesForMessage(const QString& msgId) {
        Q_UNUSED(msgId);
        return {};
    }
    virtual QPixmap decryptImage(const MediaFile& mediaFile) {
        Q_UNUSED(mediaFile);
        return QPixmap();
    }
};

#define IPlatformAdapter_iid "com.omniassist.IPlatformAdapter"
Q_DECLARE_INTERFACE(IPlatformAdapter, IPlatformAdapter_iid)

#endif
