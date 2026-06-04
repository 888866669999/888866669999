#include "OpenAIProvider.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

OpenAIProvider::OpenAIProvider(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_apiUrl("https://api.openai.com/v1/chat/completions"),
      m_model("gpt-4o-mini"), m_timeoutMs(30000), m_nextRequestId(1) {
    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
        onReplyFinished(reply);
    });
}

void OpenAIProvider::setApiKey(const QString& key) { m_apiKey = key; }
void OpenAIProvider::setApiUrl(const QString& url) { m_apiUrl = url; }
void OpenAIProvider::setModel(const QString& model) { m_model = model; }

// ===== 异步 API 实现 =====

void OpenAIProvider::generateReplyAsync(const QList<ChatMessage>& context) {
    int requestId = m_nextRequestId++;
    sendRequestAsync(requestId, Prompts::REPLY_SYSTEM, Prompts::buildReplyPrompt(context), RequestType::Reply);
}

void OpenAIProvider::generateSummaryAsync(const QList<ChatMessage>& context) {
    int requestId = m_nextRequestId++;
    sendRequestAsync(requestId, Prompts::SUMMARY_SYSTEM, Prompts::buildSummaryPrompt(context), RequestType::Summary);
}

void OpenAIProvider::analyzePersonaAsync(const QList<ChatMessage>& history) {
    int requestId = m_nextRequestId++;
    sendRequestAsync(requestId, Prompts::PERSONA_SYSTEM, Prompts::buildPersonaPrompt(history), RequestType::Persona);
}

void OpenAIProvider::safetyFilterAsync(const QString& input) {
    int requestId = m_nextRequestId++;
    sendRequestAsync(requestId, Prompts::SAFETY_FILTER_SYSTEM, input, RequestType::Filter);
}

void OpenAIProvider::sendRequestAsync(int requestId, const QString& systemPrompt, const QString& userPrompt, RequestType type) {
    if (m_apiKey.isEmpty()) {
        emit aiError(requestId, "API Key 未设置");
        return;
    }

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userPrompt;

    QJsonArray messages;
    messages.append(systemMsg);
    messages.append(userMsg);

    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["messages"] = messages;
    requestBody["temperature"] = 0.7;

    QNetworkRequest request(QUrl(m_apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QByteArray requestData = QJsonDocument(requestBody).toJson();
    QNetworkReply* reply = m_networkManager->post(request, requestData);

    // 设置超时定时器
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);

    PendingRequest pending;
    pending.requestId = requestId;
    pending.systemPrompt = systemPrompt;
    pending.userPrompt = userPrompt;
    pending.timeoutTimer = timer;
    pending.type = type;
    m_pendingRequests.insert(reply, pending);

    connect(timer, &QTimer::timeout, this, [this, reply]() {
        auto it = m_pendingRequests.find(reply);
        if (it != m_pendingRequests.end()) {
            int requestId = it->requestId;
            reply->abort();
            emit aiError(requestId, "请求超时");
            it->timeoutTimer->deleteLater();
            m_pendingRequests.erase(it);
        }
    });
    timer->start(m_timeoutMs);
}

void OpenAIProvider::onReplyFinished(QNetworkReply* reply) {
    auto it = m_pendingRequests.find(reply);
    if (it == m_pendingRequests.end()) {
        reply->deleteLater();
        return;
    }

    PendingRequest pending = it.value();
    int requestId = pending.requestId;
    RequestType type = pending.type;

    // 停止并清理定时器
    if (pending.timeoutTimer) {
        pending.timeoutTimer->stop();
        pending.timeoutTimer->deleteLater();
    }

    QString result;
    bool success = true;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        if (responseDoc.isNull()) {
            emit aiError(requestId, "API 返回无效 JSON");
            success = false;
        } else {
            QJsonObject responseObj = responseDoc.object();

            // 检查 API 错误响应
            if (responseObj.contains("error")) {
                QJsonObject errorObj = responseObj["error"].toObject();
                emit aiError(requestId, "API 错误: " + errorObj["message"].toString());
                success = false;
            } else if (responseObj.contains("choices")) {
                QJsonArray choices = responseObj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    QJsonObject messageObj = choice["message"].toObject();
                    result = messageObj["content"].toString();
                }
            }
        }
    } else {
        // 处理 429 速率限制
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429) {
            emit aiError(requestId, "API 速率限制，请稍后重试");
        } else {
            emit aiError(requestId, "请求失败: " + reply->errorString());
        }
        success = false;
    }

    reply->deleteLater();
    m_pendingRequests.erase(it);

    if (success) {
        switch (type) {
            case RequestType::Reply:
                emit replyGenerated(requestId, result);
                break;
            case RequestType::Summary: {
                QJsonObject summaryObj;
                summaryObj["summary"] = result;
                emit summaryGenerated(requestId, summaryObj);
                break;
            }
            case RequestType::Persona:
                emit personaGenerated(requestId, result);
                break;
            case RequestType::Filter:
                emit filterCompleted(requestId, result);
                break;
        }
    }
}

// ===== 同步 API 实现（向后兼容 - 阻塞调用线程） =====

QString OpenAIProvider::generateReply(const QList<ChatMessage>& context) {
    return callApi(Prompts::REPLY_SYSTEM, Prompts::buildReplyPrompt(context));
}

QJsonObject OpenAIProvider::generateSummary(const QList<ChatMessage>& context) {
    QString result = callApi(Prompts::SUMMARY_SYSTEM, Prompts::buildSummaryPrompt(context));
    QJsonObject obj;
    obj["summary"] = result;
    return obj;
}

QString OpenAIProvider::analyzePersona(const QList<ChatMessage>& history) {
    return callApi(Prompts::PERSONA_SYSTEM, Prompts::buildPersonaPrompt(history));
}

QString OpenAIProvider::safetyFilter(const QString& input) {
    return callApi(Prompts::SAFETY_FILTER_SYSTEM, input);
}

QString OpenAIProvider::callApi(const QString& systemPrompt, const QString& userPrompt) {
    if (m_apiKey.isEmpty()) {
        qWarning() << "API Key is empty";
        return QString();
    }

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userPrompt;

    QJsonArray messages;
    messages.append(systemMsg);
    messages.append(userMsg);

    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["messages"] = messages;
    requestBody["temperature"] = 0.7;

    QNetworkRequest request(QUrl(m_apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QByteArray requestData = QJsonDocument(requestBody).toJson();
    QNetworkReply* reply = m_networkManager->post(request, requestData);

    // 使用栈上定时器防止泄漏
    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(m_timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        qWarning() << "API request timed out";
        reply->deleteLater();
        return QString();
    }
    timer.stop();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        if (responseDoc.isNull()) return QString();

        QJsonObject responseObj = responseDoc.object();
        if (responseObj.contains("error")) {
            qWarning() << "API error:" << responseObj["error"].toObject()["message"].toString();
        } else if (responseObj.contains("choices")) {
            QJsonArray choices = responseObj["choices"].toArray();
            if (!choices.isEmpty()) {
                result = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
    } else {
        qWarning() << "API request failed:" << reply->errorString();
    }

    reply->deleteLater();
    return result;
}
