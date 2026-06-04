#include "QQDecoder.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <regex>
#include <set>
#include <algorithm>

QQDecoder::QQDecoder() : m_foundKeys(nullptr) {
    QSqlDatabase::removeDatabase("qq_connection");
}

QQDecoder::~QQDecoder() {
}

QString QQDecoder::findQQDataDir() {
    QStringList possiblePaths;
    
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString userName = QString::fromUtf8(qgetenv("USERNAME"));
    
    possiblePaths << "C:/Users/" + userName + "/Documents/Tencent Files";
    possiblePaths << "C:/Users/" + userName + "/AppData/Roaming/Tencent/QQ";
    possiblePaths << appData + "/Tencent/QQ";

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries) {
                QString qqDir = entry.absoluteFilePath();
                QDir dbDir(qqDir + "/Msg2.0");
                if (dbDir.exists()) {
                    return qqDir;
                }
                QDir msgDir(qqDir + "/Msg");
                if (msgDir.exists()) {
                    return qqDir;
                }
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
            if (processName.compare("QQ.exe", Qt::CaseInsensitive) == 0) {
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
    unsigned long long maxAddress = reinterpret_cast<unsigned long long>(sysInfo.lpMaximumApplicationAddress);
    
    const DWORD MEM_COMMIT = 0x1000;
    const std::set<DWORD> READABLE = {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

    while (reinterpret_cast<unsigned long long>(address) < maxAddress) {
        if (VirtualQueryEx((HANDLE)processHandle, address, &mbi, sizeof(mbi)) == 0) {
            break;
        }

        if (mbi.State == MEM_COMMIT && READABLE.count(mbi.Protect) && 
            mbi.RegionSize > 0 && mbi.RegionSize < 500 * 1024 * 1024) {
            
            QByteArray memory = readProcessMemory(processHandle, mbi.BaseAddress, mbi.RegionSize);
            
            if (!memory.isEmpty()) {
                std::regex hex_re(R"(x'([0-9a-fA-F]{64,192})')");
                std::string data_str(memory.constData(), memory.size());
                std::smatch match;
                
                std::string::const_iterator search_start(data_str.cbegin());
                while (std::regex_search(search_start, data_str.cend(), match, hex_re)) {
                    QString key = "x'" + QString::fromStdString(match[1].str()) + "'";
                    if (m_foundKeys && !m_foundKeys->contains(key)) {
                        m_foundKeys->append(key);
                        qDebug() << "Found potential key:" << key.left(30) << "...";
                    }
                    search_start = match.suffix().first;
                }
            }
        }

        unsigned long long nextAddr = reinterpret_cast<unsigned long long>(mbi.BaseAddress) + mbi.RegionSize;
        if (nextAddr <= reinterpret_cast<unsigned long long>(address)) {
            break;
        }
        address = reinterpret_cast<unsigned char*>(nextAddr);
    }

    return m_foundKeys && !m_foundKeys->isEmpty();
}

QByteArray QQDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    QByteArray buffer(size, 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory((HANDLE)processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    
    return QByteArray();
}

QList<QQContact> QQDecoder::getAllContacts(const QString& dbPath, const QString& key) {
    QList<QQContact> contacts;

    QString msgDbPath = dbPath + "/Msg2.0/Msg3.0.db";
    if (!QFile::exists(msgDbPath)) {
        msgDbPath = dbPath + "/Msg/Msg3.0.db";
    }
    
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "Msg3.0.db not found:" << msgDbPath;
        return contacts;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "qq_connection");
    db.setDatabaseName(msgDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase("qq_connection");
        return contacts;
    }

    QString sql = QString("SELECT FriendUin, NickName, Remark, FaceUrl FROM Friend;");
    QSqlQuery query(db);
    
    if (!query.exec(sql)) {
        sql = QString("SELECT uin, name, remark FROM Contact;");
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
        if (query.record().count() > 2) {
            contact.remark = query.value(2).toString();
        }
        
        if (contact.remark.isEmpty()) {
            contact.remark = contact.name;
        }
        
        contacts.append(contact);
    }

    QString groupSql = QString("SELECT GroupCode, GroupName, Memo FROM GroupInfo;");
    QSqlQuery groupQuery(db);
    
    if (groupQuery.exec(groupSql)) {
        while (groupQuery.next()) {
            QQContact contact;
            contact.id = "group_" + groupQuery.value(0).toString();
            contact.name = groupQuery.value(1).toString();
            if (groupQuery.record().count() > 2) {
                contact.remark = groupQuery.value(2).toString();
            }
            contact.isGroup = true;
            
            if (contact.remark.isEmpty()) {
                contact.remark = contact.name;
            }
            
            contacts.append(contact);
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("qq_connection");
    
    qDebug() << "Loaded" << contacts.size() << "QQ contacts";
    return contacts;
}

QList<QQMessage> QQDecoder::getChatHistory(const QString& dbPath, const QString& key, 
                                           const QString& talkerId, int limit) {
    QList<QQMessage> messages;

    QString msgDbPath = dbPath + "/Msg2.0/Msg3.0.db";
    if (!QFile::exists(msgDbPath)) {
        msgDbPath = dbPath + "/Msg/Msg3.0.db";
    }
    
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "Msg3.0.db not found:" << msgDbPath;
        return messages;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "qq_connection");
    db.setDatabaseName(msgDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase("qq_connection");
        return messages;
    }

    QString cleanTalkerId = talkerId;
    if (cleanTalkerId.startsWith("group_")) {
        cleanTalkerId = cleanTalkerId.mid(6);
    }

    QString sql = QString("SELECT MsgId, FromUin, ToUin, MsgType, Content, SendTime, IsSend "
                          "FROM Message WHERE FromUin = ? OR ToUin = ? ORDER BY SendTime DESC LIMIT ?;");
    QSqlQuery query(db);
    query.bindValue(0, cleanTalkerId);
    query.bindValue(1, cleanTalkerId);
    query.bindValue(2, limit);
    
    if (!query.exec()) {
        qWarning() << "Query failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("qq_connection");
        return messages;
    }

    while (query.next()) {
        QQMessage msg;
        msg.id = query.value(0).toString();
        msg.senderId = query.value(1).toString();
        msg.talkerId = talkerId;
        msg.type = query.value(3).toInt();
        msg.content = query.value(4).toString();
        msg.createTime = query.value(5).toLongLong();
        msg.isSelf = query.value(6).toInt() == 1;
        
        messages.append(msg);
    }

    std::reverse(messages.begin(), messages.end());

    db.close();
    QSqlDatabase::removeDatabase("qq_connection");
    
    qDebug() << "Loaded" << messages.size() << "QQ messages for" << talkerId;
    return messages;
}

bool QQDecoder::connectToNapCat(const QString& host, int port) {
    Q_UNUSED(host);
    Q_UNUSED(port);
    return true;
}

QList<QQContact> QQDecoder::getContactsFromAPI() {
    QList<QQContact> contacts;
    return contacts;
}

QList<QQMessage> QQDecoder::getMessagesFromAPI(const QString& talkerId, int limit) {
    Q_UNUSED(talkerId);
    Q_UNUSED(limit);
    QList<QQMessage> messages;
    return messages;
}
