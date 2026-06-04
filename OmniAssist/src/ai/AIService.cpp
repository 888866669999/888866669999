#include "AIService.h"
#include <QDebug>

AIService::AIService() : m_provider(nullptr) {
}

AIService::~AIService() {
}

void AIService::setProvider(IAIServiceProvider* provider) {
    m_provider = provider;
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
