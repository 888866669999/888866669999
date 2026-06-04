#ifndef OPENAIPROVIDER_H
#define OPENAIPROVIDER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>
#include "IAIServiceProvider.h"
#include "../Prompts.h"

class OpenAIProvider : public QObject, public IAIServiceProvider {
    Q_OBJECT

public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    ~OpenAIProvider() override = default;

    // 异步 API - 立即返回，结果通过信号返回
    void generateReplyAsync(const QList<ChatMessage>& context);
    void generateSummaryAsync(const QList<ChatMessage>& context);
    void analyzePersonaAsync(const QList<ChatMessage>& history);
    void safetyFilterAsync(const QString& input);

    // 同步版本（保留向后兼容）- 会阻塞调用线程，**不应在 UI 线程使用**
    QString generateReply(const QList<ChatMessage>& context) override;
    QJsonObject generateSummary(const QList<ChatMessage>& context) override;
    QString analyzePersona(const QList<ChatMessage>& history) override;
    QString safetyFilter(const QString& input) override;

    void setApiKey(const QString& key) override;
    void setApiUrl(const QString& url) override;
    void setModel(const QString& model) override;

signals:
    // 异步回调信号 - 包含 requestId 以区分不同的请求
    void replyGenerated(int requestId, const QString& reply);
    void summaryGenerated(int requestId, const QJsonObject& summary);
    void personaGenerated(int requestId, const QString& persona);
    void filterCompleted(int requestId, const QString& filtered);
    void aiError(int requestId, const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    enum class RequestType { Reply, Summary, Persona, Filter };

    struct PendingRequest {
        int requestId;
        QString systemPrompt;
        QString userPrompt;
        QTimer* timeoutTimer;
        RequestType type;
    };

    void sendRequestAsync(int requestId, const QString& systemPrompt, const QString& userPrompt, RequestType type);
    QString callApi(const QString& systemPrompt, const QString& userPrompt);

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
    int m_timeoutMs;
    int m_nextRequestId;
    QHash<QNetworkReply*, PendingRequest> m_pendingRequests;
};

#endif
