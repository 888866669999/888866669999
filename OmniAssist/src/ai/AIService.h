#ifndef AISERVICE_H
#define AISERVICE_H

#include <QString>
#include <QJsonObject>
#include <QList>
#include <memory>
#include "providers/IAIServiceProvider.h"
#include "../core/models/ChatMessage.h"

class AIService {
public:
    AIService();
    ~AIService();

    void setProvider(IAIServiceProvider* provider);

    QString autoReply(const QList<ChatMessage>& context);
    QJsonObject generateMeetingSummary(const QList<ChatMessage>& context);
    QString analyzeContactPersona(const QList<ChatMessage>& history);

private:
    IAIServiceProvider* m_provider;          // 不拥有所有权，由外部管理
    std::unique_ptr<IAIServiceProvider> m_ownedProvider;  // 默认 provider 的所有权
};

#endif
