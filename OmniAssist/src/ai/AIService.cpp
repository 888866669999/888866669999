#include "AIService.h"
#include "providers/OpenAIProvider.h"

AIService::AIService() : m_provider(nullptr) {
    // 默认使用 OpenAIProvider，由 AIService 拥有所有权
    m_ownedProvider = std::make_unique<OpenAIProvider>();
    m_provider = m_ownedProvider.get();
}

AIService::~AIService() {
    // m_ownedProvider 通过 unique_ptr 自动释放
    // 外部通过 setProvider 设置的 provider 不由 AIService 管理
}

void AIService::setProvider(IAIServiceProvider* provider) {
    m_ownedProvider.reset();  // 释放默认 provider
    m_provider = provider;    // 外部 provider，不由 AIService 管理
}

QString AIService::autoReply(const QList<ChatMessage>& context) {
    if (!m_provider) {
        return QString();
    }
    QString reply = m_provider->generateReply(context);
    return m_provider->safetyFilter(reply);
}

QJsonObject AIService::generateMeetingSummary(const QList<ChatMessage>& context) {
    if (!m_provider) {
        return QJsonObject();
    }
    return m_provider->generateSummary(context);
}

QString AIService::analyzeContactPersona(const QList<ChatMessage>& history) {
    if (!m_provider) {
        return QString();
    }
    QString analysis = m_provider->analyzePersona(history);
    return m_provider->safetyFilter(analysis);
}
