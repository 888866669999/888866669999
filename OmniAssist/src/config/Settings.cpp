#include "Settings.h"
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// Windows 默认路径
#ifdef Q_OS_WIN
static const QString DEFAULT_WECHAT_INSTALL = "C:/Program Files/Tencent/WeChat";
static const QString DEFAULT_QQ_INSTALL = "C:/Program Files/Tencent/QQ";
#else
static const QString DEFAULT_WECHAT_INSTALL = "/Applications/WeChat.app";
static const QString DEFAULT_QQ_INSTALL = "/Applications/QQ.app";
#endif

Settings::Settings() : m_autoReply(false) {
    m_openaiApiUrl = "https://api.openai.com/v1/chat/completions";
    m_openaiModel = "gpt-4o-mini";
    load();
}

Settings& Settings::instance() {
    static Settings instance;
    return instance;
}

QString Settings::configPath() const {
    return ensureConfigDir() + "/config.json";
}

QString Settings::ensureConfigDir() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}

// === 微信配置 ===
QString Settings::wechatInstallPath() const {
    return m_wechatInstallPath.isEmpty() ? DEFAULT_WECHAT_INSTALL : m_wechatInstallPath;
}

void Settings::setWechatInstallPath(const QString& path) {
    m_wechatInstallPath = path;
}

QString Settings::wechatDataPath() const {
    return m_wechatDataPath;
}

void Settings::setWechatDataPath(const QString& path) {
    m_wechatDataPath = path;
}

// === QQ 配置 ===
QString Settings::qqInstallPath() const {
    return m_qqInstallPath.isEmpty() ? DEFAULT_QQ_INSTALL : m_qqInstallPath;
}

void Settings::setQqInstallPath(const QString& path) {
    m_qqInstallPath = path;
}

QString Settings::qqDataPath() const {
    return m_qqDataPath;
}

void Settings::setQqDataPath(const QString& path) {
    m_qqDataPath = path;
}

// === AI 配置 ===
QString Settings::openaiApiKey() const {
    return m_openaiApiKey;
}

void Settings::setOpenaiApiKey(const QString& key) {
    m_openaiApiKey = key;
}

QString Settings::openaiApiUrl() const {
    return m_openaiApiUrl;
}

void Settings::setOpenaiApiUrl(const QString& url) {
    m_openaiApiUrl = url;
}

QString Settings::openaiModel() const {
    return m_openaiModel;
}

void Settings::setOpenaiModel(const QString& model) {
    m_openaiModel = model;
}

// === 通用设置 ===
bool Settings::autoReply() const {
    return m_autoReply;
}

void Settings::setAutoReply(bool enabled) {
    m_autoReply = enabled;
}

// === 持久化 ===
void Settings::save() {
    QJsonObject obj;
    obj["wechatInstallPath"] = m_wechatInstallPath;
    obj["wechatDataPath"] = m_wechatDataPath;
    obj["qqInstallPath"] = m_qqInstallPath;
    obj["qqDataPath"] = m_qqDataPath;
    obj["openaiApiKey"] = m_openaiApiKey;
    obj["openaiApiUrl"] = m_openaiApiUrl;
    obj["openaiModel"] = m_openaiModel;
    obj["autoReply"] = m_autoReply;

    QJsonDocument doc(obj);
    QFile file(configPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    } else {
        qWarning() << "Failed to save config to" << configPath();
    }
}

void Settings::load() {
    QFile file(configPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();

    m_wechatInstallPath = obj["wechatInstallPath"].toString();
    m_wechatDataPath = obj["wechatDataPath"].toString();
    m_qqInstallPath = obj["qqInstallPath"].toString();
    m_qqDataPath = obj["qqDataPath"].toString();
    m_openaiApiKey = obj["openaiApiKey"].toString();
    m_openaiApiUrl = obj["openaiApiUrl"].toString();
    m_openaiModel = obj["openaiModel"].toString();
    m_autoReply = obj["autoReply"].toBool(false);
}

void Settings::reset() {
    m_wechatInstallPath.clear();
    m_wechatDataPath.clear();
    m_qqInstallPath.clear();
    m_qqDataPath.clear();
    m_openaiApiKey.clear();
    m_openaiApiUrl = "https://api.openai.com/v1/chat/completions";
    m_openaiModel = "gpt-4o-mini";
    m_autoReply = false;
    save();
}
