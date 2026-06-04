#include "WeChatAdapter.h"
#include <QIcon>
#include <QDebug>
#include <QDir>
#include <QFile>
#include "../core/WeChatDecoder.h"

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

    if (!m_initialized) {
        initialize();
    }

    QString msgDbPath = m_dataDir + "/Msg/Msg.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "MSG.db not found:" << msgDbPath;
        return contacts;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<WeChatContact> wechatContacts = m_decoder->getAllContacts(msgDbPath, key);

    for (const WeChatContact& wc : wechatContacts) {
        Contact contact;
        contact.id = wc.id;
        contact.name = wc.name;
        contact.remark = wc.remark;
        contact.platform = Platform::WeChat;
        contacts.append(contact);
    }

    return contacts;
}

QList<ChatMessage> WeChatAdapter::getChatHistory(const QString& contactId, int limit) {
    QList<ChatMessage> messages;

    if (!m_initialized) {
        initialize();
    }

    QString msgDbPath = m_dataDir + "/Msg/Msg.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "MSG.db not found:" << msgDbPath;
        return messages;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<WeChatMessage> wechatMessages = m_decoder->getChatHistory(msgDbPath, key, contactId, limit);

    for (const WeChatMessage& wm : wechatMessages) {
        ChatMessage msg;
        msg.id = wm.id;
        msg.timestamp = wm.createTime * 1000;
        msg.senderId = wm.senderId;
        msg.senderName = wm.senderName;
        msg.content = wm.content;
        msg.type = MessageType::Text;
        msg.isSelf = wm.isSelf;
        messages.append(msg);
    }

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
