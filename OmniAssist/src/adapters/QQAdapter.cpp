#include "QQAdapter.h"
#include <QIcon>
#include <QDebug>
#include <QDir>
#include <QFile>
#include "../core/QQDecoder.h"

QQAdapter::QQAdapter(QObject* parent) 
    : QObject(parent), m_decoder(new QQDecoder()), m_initialized(false) {
}

QQAdapter::~QQAdapter() {
    delete m_decoder;
}

QIcon QQAdapter::icon() const {
    return QIcon();
}

bool QQAdapter::isAvailable() const {
    return !m_decoder->findQQDataDir().isEmpty();
}

bool QQAdapter::initialize() {
    if (m_initialized) return true;

    m_dataDir = m_decoder->findQQDataDir();
    if (m_dataDir.isEmpty()) {
        emit errorOccurred("找不到QQ数据目录");
        return false;
    }

    qDebug() << "QQ data dir:" << m_dataDir;

    if (!m_decoder->extractKeysFromMemory(&m_keys)) {
        qDebug() << "QQ key extraction failed, trying without key";
    }

    m_initialized = true;
    return true;
}

QList<Contact> QQAdapter::getContacts() {
    QList<Contact> contacts;

    if (!m_initialized) {
        initialize();
    }

    QString msgDbPath = m_dataDir + "/Msg/Msg3.0.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "QQ Msg3.0.db not found:" << msgDbPath;
        return contacts;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<QQContact> qqContacts = m_decoder->getAllContacts(m_dataDir, key);

    for (const QQContact& qc : qqContacts) {
        Contact contact;
        contact.id = qc.id;
        contact.name = qc.name;
        contact.remark = qc.remark;
        contact.platform = Platform::QQ;
        contacts.append(contact);
    }

    return contacts;
}

QList<ChatMessage> QQAdapter::getChatHistory(const QString& contactId, int limit) {
    QList<ChatMessage> messages;

    if (!m_initialized) {
        initialize();
    }

    QString msgDbPath = m_dataDir + "/Msg/Msg3.0.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "QQ Msg3.0.db not found:" << msgDbPath;
        return messages;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<QQMessage> qqMessages = m_decoder->getChatHistory(m_dataDir, key, contactId, limit);

    for (const QQMessage& qm : qqMessages) {
        ChatMessage msg;
        msg.id = qm.id;
        msg.timestamp = qm.createTime * 1000;
        msg.senderId = qm.senderId;
        msg.senderName = qm.senderName;
        msg.content = qm.content;
        msg.type = MessageType::Text;
        msg.isSelf = qm.isSelf;
        messages.append(msg);
    }

    return messages;
}

QList<ChatMessage> QQAdapter::getRecentMessages(int limit) {
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    qDebug() << "getRecentMessages not implemented yet";
    return messages;
}

bool QQAdapter::startMonitoring() {
    qDebug() << "startMonitoring not implemented yet";
    return false;
}

void QQAdapter::stopMonitoring() {
    qDebug() << "stopMonitoring not implemented yet";
}
