#include "WeChatDecoder.h"
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
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <QMutex>

// 用于生成唯一数据库连接名称的计数器
static QMutex dbMutex;
static int dbConnectionCounter = 0;

static QString generateUniqueConnectionName(const QString& prefix) {
    QMutexLocker locker(&dbMutex);
    return QString("%1_%2").arg(prefix).arg(++dbConnectionCounter);
}

WeChatDecoder::WeChatDecoder() : m_foundKeys(nullptr) {
}

WeChatDecoder::~WeChatDecoder() {
}

QString WeChatDecoder::findWeChatDataDir() {
    QStringList possiblePaths;
    
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString userName = QString::fromUtf8(qgetenv("USERNAME"));
    
    possiblePaths << "C:/Users/" + userName + "/Documents/WeChat Files";
    possiblePaths << "C:/Users/" + userName + "/AppData/Roaming/Tencent/WeChat";
    possiblePaths << docPath + "/WeChat Files";
    possiblePaths << appData + "/Tencent/WeChat";

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries) {
                QString wxidDir = entry.absoluteFilePath();
                QDir msgDir(wxidDir + "/Msg");
                if (msgDir.exists()) {
                    return wxidDir;
                }
            }
        }
    }

    return QString();
}

bool WeChatDecoder::extractKeysFromMemory(QStringList* keys) {
    if (!keys) return false;
    m_foundKeys = keys;
    keys->clear();

    QStringList pids = findWeChatProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No WeChat process found";
        return false;
    }

    qDebug() << "Found WeChat processes:" << pids;

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

QStringList WeChatDecoder::findWeChatProcesses() {
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
            if (processName.compare("WeChat.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("Weixin.exe", Qt::CaseInsensitive) == 0) {
                processes << QString::number(pe32.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
}

bool WeChatDecoder::scanProcessMemory(void* processHandle) {
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

QByteArray WeChatDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    QByteArray buffer(size, 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory((HANDLE)processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    
    return QByteArray();
}

bool WeChatDecoder::verifyKey(const QString& key, const QString& dbPath) {
    if (key.size() < 64) return false;
    
    QString hexKey = key.mid(2, 64);
    QByteArray encKey = QByteArray::fromHex(hexKey.toUtf8());
    
    if (encKey.size() != 32) {
        return false;
    }

    QFile file(dbPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray page1 = file.read(4096);
    file.close();

    if (page1.size() < 4096) {
        return false;
    }

    QByteArray salt = page1.left(16);
    
    QByteArray macSalt(salt);
    for (int i = 0; i < macSalt.size(); i++) {
        macSalt[i] = macSalt[i] ^ 0x3a;
    }

    QByteArray macKey = deriveKey(encKey, macSalt, 2, 32);
    
    QByteArray p1HmacData = page1.mid(16, 4096 - 80);
    QByteArray p1StoredHmac = page1.mid(4096 - 64, 64);
    
    QByteArray calculatedHmac = hmacSha512(macKey, p1HmacData);
    
    return calculatedHmac == p1StoredHmac;
}

QByteArray WeChatDecoder::deriveKey(const QByteArray& password, const QByteArray& salt, int iterations, int dklen) {
    const int blockSize = 64;
    QByteArray result;
    result.reserve(dklen);
    
    QByteArray currentSalt = salt;
    QByteArray digest;
    
    while (result.size() < dklen) {
        digest = hmacSha512(password, currentSalt);
        QByteArray block = digest;
        
        for (int i = 1; i < iterations; i++) {
            digest = hmacSha512(password, digest);
            for (int j = 0; j < block.size() && j < digest.size(); j++) {
                block[j] = block[j] ^ digest[j];
            }
        }
        
        result.append(block.left(qMin(block.size(), dklen - result.size())));
        currentSalt = block;
    }
    
    return result;
}

QByteArray WeChatDecoder::hmacSha512(const QByteArray& key, const QByteArray& data) {
    unsigned char digest[64];
    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) {
        return QByteArray();
    }
    HMAC_Init_ex(ctx, key.data(), key.size(), EVP_sha512(), nullptr);
    HMAC_Update(ctx, (const unsigned char*)data.data(), data.size());
    unsigned int len = sizeof(digest);
    HMAC_Final(ctx, digest, &len);
    HMAC_CTX_free(ctx);
    return QByteArray((char*)digest, len);
}

QPixmap WeChatDecoder::decryptImage(const QString& datPath) {
    return QPixmap();
}

QList<WeChatContact> WeChatDecoder::getAllContacts(const QString& dbPath, const QString& key) {
    QList<WeChatContact> contacts;

    QString contactDbPath = dbPath + "/Msg/MSG.db";
    if (!QFile::exists(contactDbPath)) {
        qWarning() << "MSG.db not found:" << contactDbPath;
        return contacts;
    }

    QString connectionName = generateUniqueConnectionName("wechat_contacts");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(contactDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase(connectionName);
        return contacts;
    }

    QString sql = QString("SELECT UserName, NickName, RemarkName, HeadImgUrl FROM Contact;");
    QSqlQuery query(db);
    
    if (!query.exec(sql)) {
        qWarning() << "Query failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return contacts;
    }

    while (query.next()) {
        WeChatContact contact;
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
    QSqlDatabase::removeDatabase(connectionName);
    
    qDebug() << "Loaded" << contacts.size() << "contacts";
    return contacts;
}

QList<WeChatMessage> WeChatDecoder::getChatHistory(const QString& dbPath, const QString& key, 
                                                   const QString& talkerId, int limit) {
    QList<WeChatMessage> messages;

    QString msgDbPath = dbPath + "/Msg/MSG.db";
    if (!QFile::exists(msgDbPath)) {
        qWarning() << "MSG.db not found:" << msgDbPath;
        return messages;
    }

    QString connectionName = generateUniqueConnectionName("wechat_messages");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(msgDbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase(connectionName);
        return messages;
    }

    QString sql = QString("SELECT MsgId, TalkerId, Content, CreateTime, Type, IsSelf, FromUserName, NickName "
                          "FROM MSG WHERE TalkerId = ? ORDER BY CreateTime DESC LIMIT ?;");
    QSqlQuery query(db);
    query.bindValue(0, talkerId);
    query.bindValue(1, limit);
    
    if (!query.exec()) {
        qWarning() << "Query failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return messages;
    }

    while (query.next()) {
        WeChatMessage msg;
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
    QSqlDatabase::removeDatabase(connectionName);
    
    qDebug() << "Loaded" << messages.size() << "messages for" << talkerId;
    return messages;
}
