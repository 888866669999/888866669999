#include "QQAdapter.h"
#include "../config/Settings.h"
#include <QIcon>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "../core/QQDecoder.h"

QQAdapter::QQAdapter(QObject* parent) 
    : QObject(parent), m_decoder(new QQDecoder()), m_initialized(false), m_useAPI(false) {
    m_platformIcon.addFile(":/icons/qq.png");
}

QQAdapter::~QQAdapter() {
    delete m_decoder;
}

Platform QQAdapter::platform() const {
    return Platform::QQ;
}

QString QQAdapter::name() const {
    return "QQ";
}

QIcon QQAdapter::icon() const {
    return m_platformIcon;
}

bool QQAdapter::isAvailable() const {
    return !findQQDataDirs().isEmpty();
}

QStringList QQAdapter::findQQDataDirs() {
    QStringList dirs;
    
    // 优先使用 Settings 中配置的数据目录
    Settings& settings = Settings::instance();
    QString customDataPath = settings.qqDataPath();
    if (!customDataPath.isEmpty()) {
        QDir dir(customDataPath);
        if (dir.exists()) {
            QDir msgDir(customDataPath + "/Msg2.0");
            if (msgDir.exists()) {
                dirs << customDataPath;
                return dirs;
            }
            QDir msgDirOld(customDataPath + "/Msg");
            if (msgDirOld.exists()) {
                dirs << customDataPath;
                return dirs;
            }
            // 如果配置的是 Tencent Files 根目录，扫描子目录
            QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries) {
                QString qqDir = entry.absoluteFilePath();
                QDir msgDir2(qqDir + "/Msg2.0");
                if (msgDir2.exists()) {
                    dirs << qqDir;
                } else {
                    QDir msgDirOld2(qqDir + "/Msg");
                    if (msgDirOld2.exists()) {
                        dirs << qqDir;
                    }
                }
            }
            if (!dirs.isEmpty()) return dirs;
        }
    }
    
    QString userName = QString::fromUtf8(qgetenv("USERNAME"));
    
    QString tencentFilesPath = "C:/Users/" + userName + "/Documents/Tencent Files";
    QDir tencentDir(tencentFilesPath);
    
    if (tencentDir.exists()) {
        QFileInfoList entries = tencentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            QString qqDir = entry.absoluteFilePath();
            QDir msgDir(qqDir + "/Msg2.0");
            if (msgDir.exists()) {
                dirs << qqDir;
            } else {
                QDir msgDirOld(qqDir + "/Msg");
                if (msgDirOld.exists()) {
                    dirs << qqDir;
                }
            }
        }
    }
    
    return dirs;
}

bool QQAdapter::initialize() {
    if (m_initialized) return true;

    QStringList dataDirs = findQQDataDirs();
    if (dataDirs.isEmpty()) {
        emit errorOccurred("找不到QQ数据目录，请确保QQ已登录过");
        return false;
    }
    
    m_dataDir = dataDirs.first();
    qDebug() << "QQ data dir:" << m_dataDir;

    QStringList keys;
    if (!m_decoder->extractKeysFromMemory(&keys)) {
        qWarning() << "无法从QQ进程提取密钥，尝试直接打开数据库";
    } else {
        m_keys = keys;
        qDebug() << "提取到" << m_keys.size() << "个密钥";
    }

    m_initialized = true;
    return true;
}

void QQAdapter::setUseAPI(bool useAPI) {
    m_useAPI = useAPI;
}

bool QQAdapter::connectToNapCat(const QString& host, int port) {
    return m_decoder->connectToNapCat(host, port);
}

QList<Contact> QQAdapter::getContacts() {
    QList<Contact> contacts;

    if (!m_initialized) {
        initialize();
    }

    if (m_useAPI) {
        QList<QQContact> qqContacts = m_decoder->getContactsFromAPI();
        for (const QQContact& qc : qqContacts) {
            Contact contact;
            contact.id = qc.id;
            contact.name = qc.name;
            contact.remark = qc.remark.isEmpty() ? qc.name : qc.remark;
            contact.platform = Platform::QQ;
            contacts.append(contact);
        }
        return contacts;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<QQContact> qqContacts = m_decoder->getAllContacts(m_dataDir, key);

    for (const QQContact& qc : qqContacts) {
        Contact contact;
        contact.id = qc.id;
        contact.name = qc.name;
        contact.remark = qc.remark.isEmpty() ? qc.name : qc.remark;
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

    if (m_useAPI) {
        QList<QQMessage> qqMessages = m_decoder->getMessagesFromAPI(contactId, limit);
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

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<QQMessage> qqMessages = m_decoder->getChatHistory(m_dataDir, key, contactId, limit);

    for (const QQMessage& qm : qqMessages) {
        ChatMessage msg;
        msg.id = qm.id;
        msg.timestamp = qm.createTime * 1000;
        msg.senderId = qm.senderId;
        msg.senderName = qm.senderName;
        msg.content = qm.content;
        
        if (qm.type == 3) {
            msg.type = MessageType::Image;
        } else if (qm.type == 34) {
            msg.type = MessageType::Audio;
        } else if (qm.type == 43) {
            msg.type = MessageType::Video;
        } else if (qm.type == 49) {
            msg.type = MessageType::File;
        } else {
            msg.type = MessageType::Text;
        }
        
        msg.isSelf = qm.isSelf;
        messages.append(msg);
    }

    return messages;
}

QList<ChatMessage> QQAdapter::getRecentMessages(int limit) {
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    return messages;
}

bool QQAdapter::startMonitoring() {
    qDebug() << "startMonitoring not implemented yet";
    return false;
}

void QQAdapter::stopMonitoring() {
    qDebug() << "stopMonitoring not implemented yet";
}

bool QQAdapter::supportsRichMedia() const {
    return true;
}

QList<MediaFile> QQAdapter::getMediaFilesForMessage(const QString& messageId) {
    Q_UNUSED(messageId);
    return QList<MediaFile>();
}

QPixmap QQAdapter::decryptImage(const MediaFile& mediaFile) {
    Q_UNUSED(mediaFile);
    return QPixmap();
}

QPixmap QQAdapter::decryptImage(const QString& datPath) {
    Q_UNUSED(datPath);
    return QPixmap();
}

void QQAdapter::refreshProcessList(QListWidget* listWidget) {
    refreshProcessListInner(listWidget);
}

QString QQAdapter::captureProcessInstallPath() {
    return captureProcessInstallPathInner();
}

void QQAdapter::refreshProcessListInner(QListWidget* listWidget) {
    if (!listWidget) return;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            // 支持旧版 QQ.exe 和新版 QQNT.exe
            if (processName.compare("QQ.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("QQNT.exe", Qt::CaseInsensitive) == 0) {
                QString pidStr = QString::number(pe32.th32ProcessID);
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                QString fullPath;
                if (hProcess) {
                    wchar_t pathBuf[MAX_PATH];
                    DWORD pathLen = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, pathBuf, &pathLen)) {
                        fullPath = QString::fromWCharArray(pathBuf);
                    }
                    CloseHandle(hProcess);
                }
                
                QString displayText = QString("%1 (PID: %2)").arg(processName).arg(pidStr);
                if (!fullPath.isEmpty()) {
                    displayText += " - " + fullPath;
                }
                listWidget->addItem(displayText);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
}

QString QQAdapter::captureProcessInstallPathInner() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return QString();
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            // 支持旧版 QQ.exe 和新版 QQNT.exe
            if (processName.compare("QQ.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("QQNT.exe", Qt::CaseInsensitive) == 0) {
                
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    wchar_t pathBuf[MAX_PATH];
                    DWORD pathLen = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, pathBuf, &pathLen)) {
                        QString fullPath = QString::fromWCharArray(pathBuf);
                        CloseHandle(hProcess);
                        CloseHandle(hSnapshot);
                        return QFileInfo(fullPath).absolutePath();
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return QString();
}
