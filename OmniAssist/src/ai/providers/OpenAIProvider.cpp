#include "OpenAIProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

OpenAIProvider::OpenAIProvider(QObject* parent) 
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_timeoutMs(30000) {
    m_apiUrl = "https://api.openai.com/v1/chat/completions";
    m_model = "gpt-4o-mini";
}

OpenAIProvider::~OpenAIProvider() {
}

void OpenAIProvider::setApiKey(const QString& key) {
    m_apiKey = key;
}

void OpenAIProvider::setApiUrl(const QString& url) {
    m_apiUrl = url;
}

void OpenAIProvider::setModel(const QString& model) {
    m_model = model;
}

QString OpenAIProvider::callApi(const QString& systemPrompt, const QString& userPrompt) {
    if (m_apiKey.isEmpty()) {
        qWarning() << "API key not set";
        return QString();
    }

    QJsonObject requestJson;
    requestJson["model"] = m_model;

    QJsonArray messages;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = systemPrompt;
    messages.append(systemMessage);

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userPrompt;
    messages.append(userMessage);

    requestJson["messages"] = messages;

    QJsonDocument doc(requestJson);
    QByteArray requestData = doc.toJson();

    QNetworkRequest request(QUrl(m_apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QNetworkReply* reply = m_networkManager->post(request, requestData);

    // 设置超时定时器
    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(m_timeoutMs);
    loop.exec();

    // 检查是否超时
    if (!timer.isActive()) {
        // 请求超时，中止请求
        reply->abort();
        qWarning() << "API request timed out after" << m_timeoutMs << "ms";
        reply->deleteLater();
        return QString();
    }
    timer.stop();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        if (responseDoc.isNull()) {
            qWarning() << "Invalid JSON response from API";
            reply->deleteLater();
            return QString();
        }

        QJsonObject responseObj = responseDoc.object();

        // 检查 API 错误响应
        if (responseObj.contains("error")) {
            QJsonObject errorObj = responseObj["error"].toObject();
            qWarning() << "API error:" << errorObj["message"].toString();
            reply->deleteLater();
            return QString();
        }

        if (responseObj.contains("choices")) {
            QJsonArray choices = responseObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                QJsonObject messageObj = choice["message"].toObject();
                result = messageObj["content"].toString();
            }
        }
    } else {
        QByteArray errorData = reply->readAll();
        // 处理 429 速率限制
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429) {
            qWarning() << "API rate limit exceeded. Please wait before retrying.";
        } else {
            qWarning() << "API request failed:" << reply->errorString();
            qWarning() << "Response:" << QString::fromUtf8(errorData);
        }
    }

    reply->deleteLater();
    return result;
}

QString OpenAIProvider::generateReply(const QList<ChatMessage>& context) {
    QString contextStr;
    for (const auto& msg : context) {
        QString role = msg.isSelf ? "我" : "对方";
        contextStr += QString("[%1] %2: %3\n")
            .arg(QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm"))
            .arg(role)
            .arg(msg.content);
    }

    return callApi(Prompts::AUTO_REPLY_SYSTEM, "聊天历史:\n" + contextStr + "\n请生成回复:");
}

QJsonObject OpenAIProvider::generateSummary(const QList<ChatMessage>& context) {
    QString contextStr;
    for (const auto& msg : context) {
        QString role = msg.isSelf ? "我" : "对方";
        contextStr += QString("[%1] %2: %3\n")
            .arg(QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm"))
            .arg(role)
            .arg(msg.content);
    }

    QString jsonResponse = callApi(Prompts::SUMMARY_SYSTEM, "聊天历史:\n" + contextStr + "\n请生成总结 (JSON格式):");

    QJsonDocument doc = QJsonDocument::fromJson(jsonResponse.toUtf8());
    if (doc.isObject()) {
        return doc.object();
    }

    QJsonObject fallback;
    fallback["summary"] = "生成总结失败";
    fallback["key_points"] = QJsonArray();
    fallback["todos"] = QJsonArray();
    return fallback;
}

QString OpenAIProvider::analyzePersona(const QList<ChatMessage>& history) {
    QString historyStr;
    for (const auto& msg : history) {
        QString role = msg.isSelf ? "我" : "对方";
        historyStr += QString("[%1] %2: %3\n")
            .arg(QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("MM-dd HH:mm"))
            .arg(role)
            .arg(msg.content);
    }

    return callApi(Prompts::PERSONA_ANALYSIS_SYSTEM, "聊天历史:\n" + historyStr + "\n请分析对方的人物画像:");
}

QString OpenAIProvider::safetyFilter(const QString& input) {
    if (input.isEmpty()) return input;

    QString filtered = callApi(Prompts::SAFETY_FILTER_SYSTEM, "待检查内容:\n" + input);
    return filtered.isEmpty() ? input : filtered;
}
