#ifndef IAISERVICEPROVIDER_H
#define IAISERVICEPROVIDER_H

#include <QString>
#include <QJsonObject>
#include <QList>
#include "../core/models/ChatMessage.h"

class IAIServiceProvider {
public:
    virtual ~IAIServiceProvider() = default;

    virtual QString generateReply(const QList<ChatMessage>& context) = 0;
    virtual QJsonObject generateSummary(const QList<ChatMessage>& context) = 0;
    virtual QString analyzePersona(const QList<ChatMessage>& history) = 0;
    virtual QString safetyFilter(const QString& input) = 0;

    virtual void setApiKey(const QString& key) = 0;
    virtual void setApiUrl(const QString& url) = 0;
    virtual void setModel(const QString& model) = 0;
};

#endif
