#include "QQDecoder.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <regex>
#include <set>
#include <algorithm>
#include <QMutex>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// 用于生成唯一数据库连接名称的计数器
static QMutex dbMutex;
static int dbConnectionCounter = 0;

static QString generateUniqueConnectionName(const QString& prefix) {
    QMutexLocker locker(&dbMutex);
    return QString("%1_%2").arg(prefix).arg(++dbConnectionCounter);
}

QQDecoder::QQDecoder() : m_foundKeys(nullptr) {
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

QStringList QQDecoder::extractKeysFromMemory() {
    QStringList keys;

    QStringList pids = findQQProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No QQ process found";
        return keys;
    }

    qDebug() << "Found QQ processes:" << pids;

    for (const QString& pidStr : pids) {
        DWORD pid = pidStr.toULong();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        
        if (hProcess) {
            scanProcessMemory(hProcess, &keys);
            CloseHandle(hProcess);
        }
    }

    return keys;
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
            // 支持旧版 QQ.exe 和新版 QQNT.exe
            if (processName.compare("QQ.exe", Qt::CaseInsensitive) == 0 ||
                processName.compare("QQNT.exe", Qt::CaseInsensitive) == 0) {
                processes << QString::number(pe32.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
}

bool QQDecoder::scanProcessMemory(void* processHandle, QStringList* foundKeys) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* address = reinterpret_cast<unsigned char*>(sysInfo.lpMinimumApplicationAddress);
    unsigned long long maxAddress = reinterpret_cast<unsigned long long>(sysInfo.lpMaximumApplicationAddress);
    
    const std::set<DWORD> READABLE = {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

    while (reinterpret_cast<unsigned long long>(address) < maxAddress) {
        if (VirtualQueryEx((HANDLE)processHandle, address, &mbi, sizeof(mbi)) == 0) {
            break;
        }

        if (mbi.State == MEM_COMMIT && READABLE.count(mbi.Protect) && 
            mbi.RegionSize > 0 && mbi.RegionSize < 500 * 1024 * 1024) {
            
            QByteArray memory = readProcessMemory(processHandle, mbi.BaseAddress, mbi.RegionSize);
            
            if (!memory.isEmpty()) {
                // 精确匹配 SQLite 密钥格式: x'128个十六进制字符'（64 字节密钥）
                std::regex hex_re(R"(x'([0-9a-fA-F]{128})')", std::regex::icase);
                std::string data_str(memory.constData(), memory.size());
                std::smatch match;
                
                std::string::const_iterator search_start(data_str.cbegin());
                while (std::regex_search(search_start, data_str.cend(), match, hex_re)) {
                    QString key = "x'" + QString::fromStdString(match[1].str()) + "'";
                    if (foundKeys && !foundKeys->contains(key)) {
                        foundKeys->append(key);
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

    return foundKeys && !foundKeys->isEmpty();
}

QByteArray QQDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    // 限制读取大小以防止整数溢出
    const size_t MAX_READ_SIZE = 100 * 1024 * 1024;  // 100MB
    if (size > MAX_READ_SIZE) {
        size = MAX_READ_SIZE;
    }
    
    QByteArray buffer(static_cast<int>(size), 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory((HANDLE)processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(static_cast<int>(bytesRead));
        return buffer;
    }
    
    return QByteArray();
}

bool QQDecoder::decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key) {
    if (!QFile::exists(dbPath)) {
        qWarning() << "Database file not found:" << dbPath;
        return false;
    }

    // 解析密钥格式: x'HEX'
    QString hexKey = key;
    if (key.startsWith("x'") && key.endsWith("'")) {
        hexKey = key.mid(2, key.length() - 3);
    }
    QByteArray encKey = QByteArray::fromHex(hexKey.toUtf8());
    if (encKey.size() != 32) {
        qWarning() << "Invalid key size:" << encKey.size() << "(expected 32 bytes)";
        return false;
    }

    QFile inFile(dbPath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open database for reading:" << dbPath;
        return false;
    }

    QByteArray fileData = inFile.readAll();
    inFile.close();

    const int PAGE_SIZE = 4096;
    const int HMAC_SIZE = 20;   // QQ 使用 HMAC-SHA1 (20 字节)
    const int SALT_SIZE = 16;
    const int IV_SIZE = 16;
    const int MAC_SALT_XOR = 0x3a;

    qint64 fileSize = fileData.size();
    if (fileSize % PAGE_SIZE != 0) {
        qWarning() << "File size is not a multiple of page size:" << fileSize;
        return false;
    }

    int numPages = fileSize / PAGE_SIZE;
    qDebug() << "Decrypting QQ database:" << numPages << "pages";

    // 提取第一页的盐值（前 16 字节）
    QByteArray salt = fileData.mid(0, SALT_SIZE);

    // 生成 MAC 盐值
    QByteArray macSalt(salt);
    for (int i = 0; i < macSalt.size(); i++) {
        macSalt[i] = macSalt[i] ^ MAC_SALT_XOR;
    }

    // 使用 PBKDF2 从原始密钥派生 HMAC 密钥 (iterations=2)
    // QQ 使用 HMAC-SHA1
    unsigned char hmacKeyBuf[32];
    if (PKCS5_PBKDF2_HMAC(encKey.data(), encKey.size(),
                           reinterpret_cast<const unsigned char*>(macSalt.data()), macSalt.size(),
                           2, EVP_sha1(),
                           32, hmacKeyBuf) != 1) {
        qCritical() << "PBKDF2 key derivation failed for QQ macKey";
        return false;
    }
    QByteArray macKey(reinterpret_cast<char*>(hmacKeyBuf), 32);

    // 使用 PBKDF2 从原始密钥派生加密密钥 (iterations=2)
    unsigned char fileKeyBuf[32];
    if (PKCS5_PBKDF2_HMAC(encKey.data(), encKey.size(),
                           reinterpret_cast<const unsigned char*>(salt.data()), salt.size(),
                           2, EVP_sha1(),
                           32, fileKeyBuf) != 1) {
        qCritical() << "PBKDF2 key derivation failed for QQ fileKey";
        return false;
    }
    QByteArray fileKey(reinterpret_cast<char*>(fileKeyBuf), 32);

    QByteArray decryptedData;
    decryptedData.reserve(fileSize);

    for (qint64 i = 0; i < numPages; i++) {
        QByteArray page = fileData.mid(i * PAGE_SIZE, PAGE_SIZE);
        if (page.size() != PAGE_SIZE) {
            qWarning() << "Page" << i << "size mismatch:" << page.size();
            return false;
        }

        // 提取 HMAC 和数据
        QByteArray storedHmac = page.mid(PAGE_SIZE - HMAC_SIZE, HMAC_SIZE);
        QByteArray pageContent;
        
        if (i == 0) {
            // 第一页: salt(16) + 数据内容 + HMAC(20)
            pageContent = page.mid(SALT_SIZE, PAGE_SIZE - SALT_SIZE - HMAC_SIZE);
        } else {
            pageContent = page.mid(0, PAGE_SIZE - HMAC_SIZE);
        }

        // 验证 HMAC-SHA1
        unsigned char hmacDigest[EVP_MAX_MD_SIZE];
        unsigned int hmacLen = 0;
        HMAC(EVP_sha1(), macKey.data(), macKey.size(),
             reinterpret_cast<const unsigned char*>(pageContent.data()), pageContent.size(),
             hmacDigest, &hmacLen);
        QByteArray calculatedHmac(reinterpret_cast<char*>(hmacDigest), hmacLen);
        
        if (calculatedHmac != storedHmac) {
            qWarning() << "HMAC verification failed for page" << i;
        }

        // 提取 IV 并解密
        QByteArray iv = pageContent.left(IV_SIZE);
        QByteArray encryptedContent = pageContent.mid(IV_SIZE);

        // 使用 AES-256-CBC 解密
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            qCritical() << "Failed to create EVP_CIPHER_CTX";
            return false;
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               reinterpret_cast<const unsigned char*>(fileKey.data()),
                               reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
            qCritical() << "EVP_DecryptInit_ex failed";
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        EVP_CIPHER_CTX_set_padding(ctx, 0);

        QByteArray decryptedPage(encryptedContent.size() + EVP_MAX_BLOCK_LENGTH, 0);
        int outLen1 = 0, outLen2 = 0;

        if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(decryptedPage.data()),
                              &outLen1, reinterpret_cast<const unsigned char*>(encryptedContent.data()), encryptedContent.size()) != 1) {
            qCritical() << "EVP_DecryptUpdate failed for page" << i;
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(decryptedPage.data()) + outLen1, &outLen2) != 1) {
            // 对于数据库页面，Final 失败可能是正常的
            outLen2 = 0;
        }

        EVP_CIPHER_CTX_free(ctx);
        decryptedPage.resize(outLen1 + outLen2);

        // 重组页面
        if (i == 0) {
            // 第一页需要恢复完整的 SQLite 文件头 (16 字节: "SQLite format 3\0")
            QByteArray sqliteHeader("SQLite format 3", 16);
            decryptedData.append(sqliteHeader);
            decryptedData.append(decryptedPage);
            while (decryptedData.size() < PAGE_SIZE) {
                decryptedData.append('\0');
            }
        } else {
            decryptedData.append(decryptedPage);
        }
    }

    // 写入解密后的文件
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open output file:" << outputPath;
        return false;
    }

    outFile.write(decryptedData);
    outFile.close();

    qDebug() << "QQ database decrypted successfully:" << outputPath;
    return true;
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

    // 如果有密钥，先尝试解密数据库
    QString dbToUse = msgDbPath;
    if (!key.isEmpty()) {
        QString decryptedPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/omniassist_qq_decrypted.db";
        if (decryptDatabase(msgDbPath, decryptedPath, key)) {
            dbToUse = decryptedPath;
        }
    }

    QString connectionName = generateUniqueConnectionName("qq_contacts");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbToUse);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase(connectionName);
        return contacts;
    }

    QString sql = QString("SELECT FriendUin, NickName, Remark, FaceUrl FROM Friend;");
    QSqlQuery query(db);
    
    if (!query.exec(sql)) {
        sql = QString("SELECT uin, name, remark FROM Contact;");
        if (!query.exec(sql)) {
            qWarning() << "Query failed:" << query.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase(connectionName);
            return contacts;
        }
    }

    while (query.next()) {
        QQContact contact;
        contact.id = query.value(0).toString();
        contact.name = query.value(1).toString();
        contact.remark = query.value(2).toString();
        contact.avatarPath = query.record().count() > 3 ? query.value(3).toString() : QString();
        
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
    QSqlDatabase::removeDatabase(connectionName);
    
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

    // 如果有密钥，先尝试解密数据库
    QString dbToUse = msgDbPath;
    if (!key.isEmpty()) {
        QString decryptedPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/omniassist_qq_messages_decrypted.db";
        if (decryptDatabase(msgDbPath, decryptedPath, key)) {
            dbToUse = decryptedPath;
        }
    }

    QString connectionName = generateUniqueConnectionName("qq_messages");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbToUse);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase(connectionName);
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
        QSqlDatabase::removeDatabase(connectionName);
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
    QSqlDatabase::removeDatabase(connectionName);
    
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
