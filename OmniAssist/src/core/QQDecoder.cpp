#include "QQDecoder.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

QQDecoder::QQDecoder() : m_foundKeys(nullptr) {
    QSqlDatabase::removeDatabase("qq_connection");
}

QQDecoder::~QQDecoder() {
}

QString QQDecoder::findQQDataDir() {
    QStringList possiblePaths;

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    
    possiblePaths << appDataPath + "/Tencent/QQ";
    possiblePaths << docPath + "/Tencent Files";
    possiblePaths << "C:/Program Files (x86)/Tencent/QQ";
    possiblePaths << "C:/Users/" + qgetenv("USERNAME") + "/AppData/Roaming/Tencent/QQ";

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            QDir msgDir(path + "/Msg");
            if (msgDir.exists()) {
                return path;
            }
        }
    }

    return QString();
}

bool QQDecoder::extractKeysFromMemory(QStringList* keys) {
    if (!keys) return false;
    m_foundKeys = keys;
    keys->clear();

    QStringList pids = findQQProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No QQ process found";
        return false;
    }

    qDebug() << "Found QQ processes:" << pids;

    for (const QString& pidStr : pids) {
        DWORD pid = pidStr.toULong();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        
        if (hProcess) {
            scanProcessMemory(hProcess);
            CloseHandle(hProcess);
        }
    }

    return !keys->isEmpty();
}

QStringList QQDecoder::findQQProcesses() {
    QStringList processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            QString processName = QString::fromWCharArray(pe32.szExeFile);
            if (processName.compare("QQ.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("QQNT.exe", Qt::CaseInsensitive) == 0) {
                processes << QString::number(pe32.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
}

bool QQDecoder::scanProcessMemory(void* processHandle) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* address = reinterpret_cast<unsigned char*>(sysInfo.lpMinimumApplicationAddress);
    
    while (address < reinterpret_cast<unsigned char*>(sysInfo.lpMaximumApplicationAddress)) {
        if (VirtualQueryEx(processHandle, address, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                QByteArray memory = readProcessMemory(processHandle, mbi.BaseAddress, mbi.RegionSize);
                
                if (!memory.isEmpty()) {
                    QByteArray pattern1 = "x'";
                    int pos = 0;
                    while ((pos = memory.indexOf(pattern1, pos)) != -1) {
                        if (pos + 64 + 32 + 2 < memory.size()) {
                            QByteArray candidate = memory.mid(pos + 2, 64 + 32);
                            bool isHex = true;
                            for (int i = 0; i < candidate.size(); ++i) {
                                char c = candidate[i];
                                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                                    isHex = false;
                                    break;
                                }
                            }
                            if (isHex && candidate.size() == 96) {
                                QString key = "x'" + candidate + "'";
                                if (m_foundKeys && !m_foundKeys->contains(key)) {
                                    m_foundKeys->append(key);
                                    qDebug() << "Found potential QQ key:" << key.left(20) << "...";
                                }
                            }
                        }
                        pos += 2;
                    }
                }
            }
            address = static_cast<unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
        } else {
            address += 0x1000;
        }
    }

    return m_foundKeys && !m_foundKeys->isEmpty();
}

QByteArray QQDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    QByteArray buffer(size, 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    
    return QByteArray();
}

QList<QQContact> QQDecoder::getAllContacts(const QString& dbPath, const QString& key) {
    QList<QQContact> contacts;

    QString contactDbPath = dbPath + "/Msg/Contact.db";
    if (!QFile::exists(contactDbPath)) {
        contactDbPath = dbPath + "/Msg/Msg3.0.db";
    }

    if (!QFile::exists(contactDbPath)) {
        qWarning() << "QQ contact database not found:" << contactDbPath;
        return contacts;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "qq_connection");
    db.setDatabaseName(contactDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open QQ database:" << db.lastError().text();
        QSqlDatabase::removeDatabase("qq_connection");
        return contacts;
    }

    QString sql = QString("SELECT uin, nickname, remark, face FROM Friend;");
    QSqlQuery query(db);
    
    if (!query.exec(sql)) {
        sql = QString("SELECT UserName, NickName, RemarkName, HeadImgUrl FROM Contact;");
        if (!query.exec(sql)) {
            qWarning() << "Query failed:" << query.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase("qq_connection");
            return contacts;
        }
    }

    while (query.next()) {
        QQContact contact;
        contact.id = query.value(0).toString();
        contact.name = query.value(1).toString();
        contact.remark = query.value(2).toString();
        contact.avatarPath = query.value(3).toString();
        
        if (contact.remark.isEmpty()) {
            contact.remark = contact.name;
        }
        
        contacts.append(contact);
    }

    db.close();
    QSqlDatabase::removeDatabase("qq_connection");
    
    qDebug() << "Loaded" << contacts.size() << "QQ contacts";
    return contacts;
}

QList<QQMessage> QQDecoder::getChatHistory(const QString& dbPath, const QString& key, 
                                           const QString& talkerId, int limit) {
    QList<QQMessage> messages;

    QString msgDbPath = dbPath + "/Msg/Msg3.0.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "QQ MSG database not found:" << msgDbPath;
        return messages;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "qq_connection");
    db.setDatabaseName(msgDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open QQ database:" << db.lastError().text();
        QSqlDatabase::removeDatabase("qq_connection");
        return messages;
    }

    QString sql = QString("SELECT msgId, talkerId, content, time, type, isSend, fromUin, nick "
                          "FROM Message WHERE talkerId = ? ORDER BY time DESC LIMIT ?;");
    QSqlQuery query(db);
    query.bindValue(0, talkerId);
    query.bindValue(1, limit);
    
    if (!query.exec()) {
        sql = QString("SELECT MsgId, TalkerId, Content, CreateTime, Type, IsSelf, FromUserName, NickName "
                      "FROM MSG WHERE TalkerId = ? ORDER BY CreateTime DESC LIMIT ?;");
        QSqlQuery query2(db);
        query2.bindValue(0, talkerId);
        query2.bindValue(1, limit);
        if (!query2.exec()) {
            qWarning() << "Query failed:" << query2.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase("qq_connection");
            return messages;
        }
        query = query2;
    }

    while (query.next()) {
        QQMessage msg;
        msg.id = query.value(0).toString();
        msg.talkerId = query.value(1).toString();
        msg.content = query.value(2).toString();
        msg.createTime = query.value(3).toLongLong();
        msg.type = query.value(4).toInt();
        msg.isSelf = query.value(5).toInt() == 1;
        msg.senderId = query.value(6).toString();
        msg.senderName = query.value(7).toString();
        
        messages.append(msg);
    }

    std::reverse(messages.begin(), messages.end());

    db.close();
    QSqlDatabase::removeDatabase("qq_connection");
    
    qDebug() << "Loaded" << messages.size() << "QQ messages for" << talkerId;
    return messages;
}
