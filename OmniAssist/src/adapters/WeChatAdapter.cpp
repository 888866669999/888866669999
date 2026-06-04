#include "WeChatAdapter.h"
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
#include "../core/WeChatDecoder.h"

WeChatAdapter::WeChatAdapter(QObject* parent) 
    : QObject(parent), m_decoder(new WeChatDecoder()), m_initialized(false) {
    m_platformIcon.addFile(":/icons/wechat.png");
}

WeChatAdapter::~WeChatAdapter() {
    delete m_decoder;
}

Platform WeChatAdapter::platform() const {
    return Platform::WeChat;
}

QString WeChatAdapter::name() const {
    return "微信";
}

QIcon WeChatAdapter::icon() const {
    return m_platformIcon;
}

bool WeChatAdapter::isAvailable() const {
    return !findWeChatDataDirs().isEmpty();
}

QStringList WeChatAdapter::findWeChatDataDirs() {
    QStringList dirs;
    
    // 优先使用 Settings 中配置的数据目录
    Settings& settings = Settings::instance();
    QString customDataPath = settings.wechatDataPath();
    if (!customDataPath.isEmpty()) {
        QDir dir(customDataPath);
        if (dir.exists()) {
            QDir msgDir(customDataPath + "/Msg");
            if (msgDir.exists()) {
                dirs << customDataPath;
                return dirs;
            }
            // 如果配置的是 WeChat Files 根目录，扫描子目录
            QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries) {
                QString wxidDir = entry.absoluteFilePath();
                QDir msgDir2(wxidDir + "/Msg");
                if (msgDir2.exists()) {
                    dirs << wxidDir;
                }
            }
            if (!dirs.isEmpty()) return dirs;
        }
    }
    
    // 使用 Settings 中配置的安装目录获取用户名（部分微信版本将数据放在安装目录下）
    QString userName = QString::fromUtf8(qgetenv("USERNAME"));
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    
    QString wechatFilesPath = docPath + "/WeChat Files";
    QDir wechatDir(wechatFilesPath);
    
    if (wechatDir.exists()) {
        QFileInfoList entries = wechatDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            QString wxidDir = entry.absoluteFilePath();
            QDir msgDir(wxidDir + "/Msg");
            if (msgDir.exists()) {
                dirs << wxidDir;
            }
        }
    }
    
    return dirs;
}

bool WeChatAdapter::initialize() {
    if (m_initialized) return true;

    QStringList dataDirs = findWeChatDataDirs();
    if (dataDirs.isEmpty()) {
        emit errorOccurred("找不到微信数据目录，请确保微信已登录过");
        return false;
    }
    
    m_dataDir = dataDirs.first();
    qDebug() << "WeChat data dir:" << m_dataDir;

    QStringList keys;
    if (!m_decoder->extractKeysFromMemory(&keys)) {
        qWarning() << "无法从微信进程提取密钥，尝试直接打开数据库";
    } else {
        m_keys = keys;
        qDebug() << "提取到" << m_keys.size() << "个密钥";
    }

    m_initialized = true;
    return true;
}

QList<Contact> WeChatAdapter::getContacts() {
    QList<Contact> contacts;

    if (!m_initialized) {
        initialize();
    }

    QString msgDbPath = m_dataDir + "/Msg/MSG.db";
    if (!QFile::exists(msgDbPath)) {
        msgDbPath = m_dataDir + "/Msg/Msg.db";
    }
    
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "MSG.db not found:" << msgDbPath;
        return contacts;
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<WeChatContact> weChatContacts = m_decoder->getAllContacts(m_dataDir, m_keys.isEmpty() ? QString() : m_keys.first());
    
    for (const WeChatContact& wc : weChatContacts) {
        Contact contact;
        contact.id = wc.id;
        contact.name = wc.name;
        contact.remark = wc.remark.isEmpty() ? wc.name : wc.remark;
        contact.platform = Platform::WeChat;
        contact.platformIcon = m_platformIcon;
        contact.avatar = QPixmap();  // TODO: 加载头像
        contact.extra = QVariant::fromValue(wc);
        
        contacts.append(contact);
    }

    return contacts;
}

QList<ChatMessage> WeChatAdapter::getChatHistory(const QString& contactId, int limit) {
    QList<ChatMessage> messages;

    if (!m_initialized) {
        initialize();
    }

    QString key = m_keys.isEmpty() ? QString() : m_keys.first();
    QList<WeChatMessage> wechatMessages = m_decoder->getChatHistory(m_dataDir, key, contactId, limit);

    for (const WeChatMessage& wm : wechatMessages) {
        ChatMessage msg;
        msg.id = wm.id;
        msg.timestamp = wm.createTime * 1000;
        msg.senderId = wm.senderId;
        msg.senderName = wm.senderName;
        msg.content = wm.content;
        
        if (wm.type == 3) {
            msg.type = MessageType::Image;
        } else if (wm.type == 34) {
            msg.type = MessageType::Audio;
        } else if (wm.type == 43) {
            msg.type = MessageType::Video;
        } else if (wm.type == 49) {
            msg.type = MessageType::File;
        } else {
            msg.type = MessageType::Text;
        }
        
        msg.isSelf = wm.isSelf;
        messages.append(msg);
    }

    return messages;
}

QList<ChatMessage> WeChatAdapter::getRecentMessages(int limit) {
    Q_UNUSED(limit);
    QList<ChatMessage> messages;
    return messages;
}

bool WeChatAdapter::startMonitoring() {
    qDebug() << "startMonitoring not implemented yet";
    return false;
}

void WeChatAdapter::stopMonitoring() {
    qDebug() << "stopMonitoring not implemented yet";
}

bool WeChatAdapter::supportsRichMedia() const {
    return true;
}

QList<MediaFile> WeChatAdapter::getMediaFilesForMessage(const QString& messageId) {
    Q_UNUSED(messageId);
    return QList<MediaFile>();
}

QPixmap WeChatAdapter::decryptImage(const MediaFile& mediaFile) {
    Q_UNUSED(mediaFile);
    return QPixmap();
}

QPixmap WeChatAdapter::decryptImage(const QString& datPath) {
    return m_decoder->decryptImage(datPath);
}

void WeChatAdapter::refreshProcessList(QListWidget* listWidget) {
    refreshProcessListInner(listWidget);
}

QString WeChatAdapter::captureProcessInstallPath() {
    return captureProcessInstallPathInner();
}

void WeChatAdapter::refreshProcessListInner(QListWidget* listWidget) {
    if (!listWidget) return;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            if (processName.compare("WeChat.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("Weixin.exe", Qt::CaseInsensitive) == 0) {
                // 获取进程可执行文件路径
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

QString WeChatAdapter::captureProcessInstallPathInner() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return QString();
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            if (processName.compare("WeChat.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("Weixin.exe", Qt::CaseInsensitive) == 0) {
                
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    wchar_t pathBuf[MAX_PATH];
                    DWORD pathLen = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, pathBuf, &pathLen)) {
                        QString fullPath = QString::fromWCharArray(pathBuf);
                        CloseHandle(hProcess);
                        CloseHandle(hSnapshot);
                        // 返回安装目录（可执行文件所在目录）
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
