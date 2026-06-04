#ifndef OPENAIPROVIDER_H
#define OPENAIPROVIDER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "IAIServiceProvider.h"
#include "../Prompts.h"

class OpenAIProvider : public QObject, public IAIServiceProvider {
    Q_OBJECT

public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    ~OpenAIProvider() override = default;

    QString generateReply(const QList<ChatMessage>& context) override;
    QJsonObject generateSummary(const QList<ChatMessage>& context) override;
    QString analyzePersona(const QList<ChatMessage>& history) override;
    QString safetyFilter(const QString& input) override;

    void setApiKey(const QString& key) override;
    void setApiUrl(const QString& url) override;
    void setModel(const QString& model) override;

private:
    QString callApi(const QString& systemPrompt, const QString& userPrompt);

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
    int m_timeoutMs;  // 请求超时时间（毫秒）
};

#endif
