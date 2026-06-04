#include "WeChatAdapter.h"
#include <QIcon>
#include <QDebug>

WeChatAdapter::WeChatAdapter(QObject* parent) 
    : QObject(parent), m_decoder(new WeChatDecoder()), m_initialized(false) {
}

WeChatAdapter::~WeChatAdapter() {
    delete m_decoder;
}

QIcon WeChatAdapter::icon() const {
    return QIcon();
}

bool WeChatAdapter::isAvailable() const {
    return !m_decoder->findWeChatDataDir().isEmpty();
}

bool WeChatAdapter::initialize() {
    if (m_initialized) return true;

    m_dataDir = m_decoder->findWeChatDataDir();
    if (m_dataDir.isEmpty()) {
        emit errorOccurred("找不到微信数据目录");
        return false;
    }

    qDebug() << "WeChat data dir:" << m_dataDir;

    if (!m_decoder->extractKeysFromMemory(&m_keys)) {
        emit errorOccurred("无法从微信进程提取密钥，请确保微信正在运行");
        return false;
    }

    qDebug() << "Extracted" << m_keys.size() << "keys";

    m_initialized = true;
    return true;
}

QList<Contact> WeChatAdapter::getContacts() {
    QList<Contact> contacts;
    qDebug() << "getContacts not implemented yet";
    return contacts;
}

QList<ChatMessage> WeChatAdapter::getChatHistory(const QString& contactId, int limit) {
    Q_UNUSED(contactId);
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    qDebug() << "getChatHistory not implemented yet";
    return messages;
}

QList<ChatMessage> WeChatAdapter::getRecentMessages(int limit) {
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    qDebug() << "getRecentMessages not implemented yet";
    return messages;
}

bool WeChatAdapter::startMonitoring() {
    qDebug() << "startMonitoring not implemented yet";
    return false;
}

void WeChatAdapter::stopMonitoring() {
    qDebug() << "stopMonitoring not implemented yet";
}
