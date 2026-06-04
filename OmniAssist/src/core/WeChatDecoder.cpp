#include "WeChatDecoder.h"
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
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
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

QStringList WeChatDecoder::extractKeysFromMemory() {
    QStringList keys;

    QStringList pids = findWeChatProcesses();
    if (pids.isEmpty()) {
        qWarning() << "No WeChat process found";
        return keys;
    }

    qDebug() << "Found WeChat processes:" << pids;

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

bool WeChatDecoder::scanProcessMemory(void* processHandle, QStringList* foundKeys) {
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
                // 精确匹配 SQLite 密钥格式: x'64个十六进制字符'（64 字节 = 128 个十六进制字符）
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

QByteArray WeChatDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
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
    
    // 微信使用 HMAC-SHA512 进行校验（存储 64 字节）
    QByteArray calculatedHmac = hmacSha512(macKey, p1HmacData);
    
    return calculatedHmac == p1StoredHmac;
}

QByteArray WeChatDecoder::deriveKey(const QByteArray& password, const QByteArray& salt, int iterations, int dklen) {
    // 使用 OpenSSL 的 PKCS5_PBKDF2_HMAC 实现标准 PBKDF2
    QByteArray key(dklen, 0);
    PKCS5_PBKDF2_HMAC(password.data(), password.size(),
                       reinterpret_cast<const unsigned char*>(salt.data()), salt.size(),
                       iterations, EVP_sha1(),
                       dklen, reinterpret_cast<unsigned char*>(key.data()));
    return key;
}

QByteArray WeChatDecoder::hmacSha1(const QByteArray& key, const QByteArray& data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) {
        qCritical() << "Failed to create HMAC_CTX";
        return QByteArray();
    }
    if (HMAC_Init_ex(ctx, key.data(), key.size(), EVP_sha1(), nullptr) != 1) {
        qCritical() << "HMAC_Init_ex failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    if (HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(data.data()), data.size()) != 1) {
        qCritical() << "HMAC_Update failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    unsigned int len = sizeof(digest);
    if (HMAC_Final(ctx, digest, &len) != 1) {
        qCritical() << "HMAC_Final failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    HMAC_CTX_free(ctx);
    return QByteArray(reinterpret_cast<char*>(digest), len);
}

QByteArray WeChatDecoder::hmacSha512(const QByteArray& key, const QByteArray& data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) {
        qCritical() << "Failed to create HMAC_CTX";
        return QByteArray();
    }
    if (HMAC_Init_ex(ctx, key.data(), key.size(), EVP_sha512(), nullptr) != 1) {
        qCritical() << "HMAC_Init_ex failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    if (HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(data.data()), data.size()) != 1) {
        qCritical() << "HMAC_Update failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    unsigned int len = sizeof(digest);
    if (HMAC_Final(ctx, digest, &len) != 1) {
        qCritical() << "HMAC_Final failed";
        HMAC_CTX_free(ctx);
        return QByteArray();
    }
    HMAC_CTX_free(ctx);
    return QByteArray(reinterpret_cast<char*>(digest), len);
}

QPixmap WeChatDecoder::decryptImage(const QString& datPath) {
    QFile file(datPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open image file:" << datPath;
        return QPixmap();
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 4) {
        qWarning() << "Image file too small:" << datPath;
        return QPixmap();
    }

    // 微信图片使用 XOR 加密，通过文件头确定密钥
    // JPEG 文件头: FF D8 FF
    // PNG 文件头: 89 50 4E 47
    // GIF 文件头: 47 49 46 38
    quint8 xorKey = 0;
    
    // 尝试 JPEG 头 (最常见的微信图片格式)
    quint8 key1 = data[0] ^ 0xFF;
    quint8 key2 = data[1] ^ 0xD8;
    
    if (key1 == key2) {
        // JPEG 格式
        xorKey = key1;
    } else {
        // 尝试 PNG 头
        quint8 key3 = data[0] ^ 0x89;
        quint8 key4 = data[1] ^ 0x50;
        if (key3 == key4) {
            xorKey = key3;
        } else {
            // 尝试 GIF 头
            quint8 key5 = data[0] ^ 0x47;
            quint8 key6 = data[1] ^ 0x49;
            if (key5 == key6) {
                xorKey = key5;
            } else {
                // 使用前两个字节异或作为密钥
                xorKey = key1;
                qWarning() << "Unknown image format, using fallback XOR key";
            }
        }
    }

    qDebug() << "XOR key:" << QString("0x%1").arg(xorKey, 2, 16, QChar('0'));

    // 解密数据
    QByteArray decrypted(data.size(), 0);
    for (int i = 0; i < data.size(); i++) {
        decrypted[i] = static_cast<char>(static_cast<quint8>(data[i]) ^ xorKey);
    }

    QPixmap pixmap;
    if (!pixmap.loadFromData(decrypted)) {
        qWarning() << "Failed to load decrypted image";
        return QPixmap();
    }

    return pixmap;
}

bool WeChatDecoder::decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key) {
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
    const int HMAC_SIZE = 64;   // HMAC-SHA512
    const int SALT_SIZE = 16;
    const int IV_SIZE = 16;
    const int MAC_SALT_XOR = 0x3a;

    qint64 fileSize = fileData.size();
    if (fileSize % PAGE_SIZE != 0) {
        qWarning() << "File size is not a multiple of page size:" << fileSize;
        return false;
    }

    int numPages = fileSize / PAGE_SIZE;
    qDebug() << "Decrypting" << numPages << "pages";

    // 提取第一页的盐值（前 16 字节）
    QByteArray salt = fileData.mid(0, SALT_SIZE);

    // 生成 MAC 盐值
    QByteArray macSalt(salt);
    for (int i = 0; i < macSalt.size(); i++) {
        macSalt[i] = macSalt[i] ^ MAC_SALT_XOR;
    }

    // 使用 PBKDF2 从原始密钥派生 HMAC 密钥 (iterations=2)
    unsigned char macKeyBuf[32];
    if (PKCS5_PBKDF2_HMAC(encKey.data(), encKey.size(),
                           reinterpret_cast<const unsigned char*>(macSalt.data()), macSalt.size(),
                           2, EVP_sha1(),
                           32, macKeyBuf) != 1) {
        qCritical() << "PBKDF2 key derivation failed for macKey";
        return false;
    }
    QByteArray macKey(reinterpret_cast<char*>(macKeyBuf), 32);

    // 使用 PBKDF2 从原始密钥派生加密密钥 (iterations=2, 使用 HMAC-SHA1)
    unsigned char fileKeyBuf[32];
    if (PKCS5_PBKDF2_HMAC(encKey.data(), encKey.size(),
                           reinterpret_cast<const unsigned char*>(salt.data()), salt.size(),
                           2, EVP_sha1(),
                           32, fileKeyBuf) != 1) {
        qCritical() << "PBKDF2 key derivation failed for fileKey";
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
            // 第一页: salt(16) + 数据内容 + HMAC(64)
            pageContent = page.mid(SALT_SIZE, PAGE_SIZE - SALT_SIZE - HMAC_SIZE);
        } else {
            pageContent = page.mid(0, PAGE_SIZE - HMAC_SIZE);
        }

        // 验证 HMAC
        QByteArray calculatedHmac = hmacSha512(macKey, pageContent);
        if (calculatedHmac != storedHmac) {
            qWarning() << "HMAC verification failed for page" << i;
            // 不中断，继续解密（可能是数据库结构差异）
        }

        // 提取 IV 并解密
        QByteArray iv = pageContent.left(IV_SIZE);
        QByteArray encryptedContent = pageContent.mid(IV_SIZE);

        // 使用 AES-256-CBC 解密
        QByteArray decryptedPage = aes256CbcDecrypt(fileKey, iv, encryptedContent);
        if (decryptedPage.isEmpty()) {
            qWarning() << "Decryption failed for page" << i;
            return false;
        }

        // 重组页面
        if (i == 0) {
            // 第一页需要恢复完整的 SQLite 文件头 (16 字节: "SQLite format 3\0")
            QByteArray sqliteHeader("SQLite format 3", 16);
            decryptedData.append(sqliteHeader);
            decryptedData.append(decryptedPage);
            // 填充到页面大小
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

    qDebug() << "Database decrypted successfully:" << outputPath;
    return true;
}

QByteArray WeChatDecoder::aes256CbcDecrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& data) {
    if (key.size() != 32 || iv.size() != 16) {
        qWarning() << "Invalid key or IV size";
        return QByteArray();
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qCritical() << "Failed to create EVP_CIPHER_CTX";
        return QByteArray();
    }

    // 初始化 AES-256-CBC 解密
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, 
                           reinterpret_cast<const unsigned char*>(key.data()),
                           reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
        qCritical() << "EVP_DecryptInit_ex failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    // 不使用填充（微信数据库数据已经是页面大小的倍数）
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    QByteArray result(data.size() + EVP_MAX_BLOCK_LENGTH, 0);
    int outLen1 = 0, outLen2 = 0;

    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(result.data()),
                          &outLen1, reinterpret_cast<const unsigned char*>(data.data()), data.size()) != 1) {
        qCritical() << "EVP_DecryptUpdate failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(result.data()) + outLen1, &outLen2) != 1) {
        qWarning() << "EVP_DecryptFinal_ex failed (may be expected for raw pages)";
        // 对于数据库页面，Final 失败可能是正常的
        outLen2 = 0;
    }

    EVP_CIPHER_CTX_free(ctx);
    result.resize(outLen1 + outLen2);
    return result;
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

    // 注意：微信 MSG.db 是 SQLCipher 加密数据库，需要先调用 decryptDatabase() 解密
    // 如果直接打开会失败并返回空列表。建议先实现 decryptDatabase 函数。
    if (!db.open()) {
        qWarning() << "Failed to open database (may be encrypted with SQLCipher):" << db.lastError().text();
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
