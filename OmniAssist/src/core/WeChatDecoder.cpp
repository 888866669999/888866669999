#include "WeChatDecoder.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

WeChatDecoder::WeChatDecoder() : m_foundKeys(nullptr) {
}

WeChatDecoder::~WeChatDecoder() {
}

QString WeChatDecoder::findWeChatDataDir() {
    QStringList possiblePaths;

    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    possiblePaths << docPath + "/WeChat Files";
    possiblePaths << docPath + "/Tencent Files/WeChat";

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            return path;
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

bool WeChatDecoder::decryptDatabase(const QString& dbPath, const QString& outputPath, const QString& key) {
    Q_UNUSED(dbPath);
    Q_UNUSED(outputPath);
    Q_UNUSED(key);
    qWarning() << "decryptDatabase not implemented yet";
    return false;
}

bool WeChatDecoder::verifyKey(const QString& dbPath, const QString& key) {
    Q_UNUSED(dbPath);
    Q_UNUSED(key);
    qWarning() << "verifyKey not implemented yet";
    return false;
}

QPixmap WeChatDecoder::decryptImage(const QString& datPath) {
    Q_UNUSED(datPath);
    qWarning() << "decryptImage not implemented yet";
    return QPixmap();
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
                processName.compare("WeChatWin.dll", Qt::CaseInsensitive) == 0) {
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
                                    qDebug() << "Found potential key:" << key.left(20) << "...";
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

QByteArray WeChatDecoder::readProcessMemory(void* processHandle, void* address, size_t size) {
    QByteArray buffer(size, 0);
    SIZE_T bytesRead = 0;
    
    if (ReadProcessMemory(processHandle, address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    
    return QByteArray();
}

bool WeChatDecoder::isValidKey(const QString& key, const QString& dbPath) {
    Q_UNUSED(key);
    Q_UNUSED(dbPath);
    return false;
}
