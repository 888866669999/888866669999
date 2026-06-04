#include "AIService.h"
#include <QDebug>

AIService::AIService() : m_provider(nullptr), m_ownedProvider(false) {
}

AIService::~AIService() {
    if (m_ownedProvider && m_provider) {
        delete m_provider;
    }
}

void AIService::setProvider(IAIServiceProvider* provider) {
    m_provider = provider;
    m_ownedProvider = false;  // 外部传入的 provider 不由 AIService 管理
}

QString AIService::autoReply(const QList<ChatMessage>& context) {
    if (!m_provider) {
        qWarning() << "AI provider not set";
        return QString();
    }

    QString reply = m_provider->generateReply(context);
    return m_provider->safetyFilter(reply);
}

QJsonObject AIService::generateMeetingSummary(const QList<ChatMessage>& context) {
    if (!m_provider) {
        qWarning() << "AI provider not set";
        return QJsonObject();
    }

    return m_provider->generateSummary(context);
}

QString AIService::analyzeContactPersona(const QList<ChatMessage>& history) {
    if (!m_provider) {
        qWarning() << "AI provider not set";
        return QString();
    }

    QString analysis = m_provider->analyzePersona(history);
    return m_provider->safetyFilter(analysis);
}
