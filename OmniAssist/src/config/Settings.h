#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

/**
 * 全局配置管理 - 支持自定义安装目录、数据目录、API 配置等
 * 配置持久化到用户数据目录下的 config.json
 */
class Settings {
public:
    static Settings& instance();

    // 微信配置
    QString wechatInstallPath() const;
    void setWechatInstallPath(const QString& path);
    QString wechatDataPath() const;
    void setWechatDataPath(const QString& path);

    // QQ 配置
    QString qqInstallPath() const;
    void setQqInstallPath(const QString& path);
    QString qqDataPath() const;
    void setQqDataPath(const QString& path);

    // AI 配置
    QString openaiApiKey() const;
    void setOpenaiApiKey(const QString& key);
    QString openaiApiUrl() const;
    void setOpenaiApiUrl(const QString& url);
    QString openaiModel() const;
    void setOpenaiModel(const QString& model);

    // 通用设置
    bool autoReply() const;
    void setAutoReply(bool enabled);

    // 配置持久化
    void save();
    void load();

    // 重置为默认值
    void reset();

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    QString configPath() const;
    QString ensureConfigDir() const;

    // 微信
    QString m_wechatInstallPath;
    QString m_wechatDataPath;

    // QQ
    QString m_qqInstallPath;
    QString m_qqDataPath;

    // AI
    QString m_openaiApiKey;
    QString m_openaiApiUrl;
    QString m_openaiModel;

    // 通用
    bool m_autoReply;
};

#endif
