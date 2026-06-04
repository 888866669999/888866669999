#include "MessageSender.h"
#include "WindowFinder.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QDebug>
#include <QFileInfo>
#include <QHash>
#include <windows.h>

MessageSender::MessageSender(QObject* parent) 
    : QObject(parent), m_windowFinder(new WindowFinder()) {
}

MessageSender::~MessageSender() {
    delete m_windowFinder;
}

void MessageSender::addRandomDelay() {
    int delayMs = 1000 + QRandomGenerator::global()->bounded(2000);
    QThread::msleep(delayMs);
}

void MessageSender::simulateMouseMovement() {
    POINT originalPos;
    GetCursorPos(&originalPos);

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int centerX = screenGeometry.width() / 2;
    int centerY = screenGeometry.height() / 2;

    int steps = 20 + QRandomGenerator::global()->bounded(30);
    for (int i = 0; i < steps; ++i) {
        int x = centerX + QRandomGenerator::global()->bounded(-50, 51);
        int y = centerY + QRandomGenerator::global()->bounded(-50, 51);
        SetCursorPos(x, y);
        QThread::msleep(10);
    }

    SetCursorPos(originalPos.x, originalPos.y);
}

void MessageSender::pressKey(WORD key) {
    keybd_event(key, 0, 0, 0);
    QThread::msleep(10);
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
    QThread::msleep(10);
}

bool MessageSender::sendTextViaClipboard(HWND hwnd, const QString& text) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    // 保存当前剪贴板内容
    QString oldText = clipboard->text(QClipboard::Clipboard);

    clipboard->setText(text, QClipboard::Clipboard);
    QThread::msleep(100);

    if (!SetForegroundWindow(hwnd)) {
        qWarning() << "Failed to set foreground window";
        clipboard->setText(oldText, QClipboard::Clipboard);
        return false;
    }
    QThread::msleep(100);

    // 使用 INPUT 数组一次性发送完整按键序列，确保按键状态正确
    INPUT inputs[4] = {};
    // Ctrl down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    // V down
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    // V up
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    // Ctrl up
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
    QThread::msleep(100);

    // 恢复剪贴板
    clipboard->setText(oldText, QClipboard::Clipboard);

    return true;
}

bool MessageSender::sendFileViaClipboard(HWND hwnd, const QString& filePath) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "File not found:" << filePath;
        return false;
    }

    // 保存当前剪贴板内容
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* oldMimeData = clipboard->mimeData() ? clipboard->mimeData()->clone() : nullptr;

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(filePath);
    
    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    
    clipboard->setMimeData(mimeData, QClipboard::Clipboard);
    QThread::msleep(100);

    if (!SetForegroundWindow(hwnd)) {
        qWarning() << "Failed to set foreground window";
        if (oldMimeData) clipboard->setMimeData(oldMimeData);
        return false;
    }
    QThread::msleep(200);

    // 使用 INPUT 数组一次性发送完整按键序列
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));

    // 恢复剪贴板
    if (oldMimeData) {
        clipboard->setMimeData(oldMimeData);
    }

    return true;
}

bool MessageSender::sendText(Platform platform, const QString& contactId, const QString& contactName, const QString& text) {
    qDebug() << "Sending text to" << contactId << "(" << contactName << ") platform:" << static_cast<int>(platform);

    addRandomDelay();
    simulateMouseMovement();

    // 使用联系人的显示名称（昵称）作为窗口标题进行搜索，
    // 而非内部 ID（如 wxid_xxx），因为聊天窗口标题是昵称格式
    QString windowTitle = contactName.isEmpty() ? contactId : contactName;

    QList<HWND> windows = m_windowFinder->findAllWindows(windowTitle);
    if (windows.isEmpty()) {
        qWarning() << "No window found for" << windowTitle;
        return false;
    }

    HWND hwnd = windows.first();
    if (!m_windowFinder->bringToFront(hwnd)) {
        qWarning() << "Failed to bring window to front";
        return false;
    }
    QThread::msleep(200);

    bool result = sendTextViaClipboard(hwnd, text);
    if (result) {
        QThread::msleep(100);
        pressKey(VK_RETURN);
        qDebug() << "Message sent successfully";
    }

    return result;
}

bool MessageSender::sendFile(Platform platform, const QString& contactId, const QString& filePath) {
    qDebug() << "Sending file" << filePath << "to" << contactId;

    addRandomDelay();
    simulateMouseMovement();

    QList<HWND> windows = m_windowFinder->findAllWindows(contactId);
    if (windows.isEmpty()) {
        qWarning() << "No window found for" << contactId;
        return false;
    }

    HWND hwnd = windows.first();
    if (!m_windowFinder->bringToFront(hwnd)) {
        return false;
    }
    QThread::msleep(200);

    bool result = sendFileViaClipboard(hwnd, filePath);
    if (result) {
        QThread::msleep(100);
        pressKey(VK_RETURN);
        qDebug() << "File sent successfully";
    }

    return result;
}

bool MessageSender::sendKeys(HWND hwnd, const QString& keys) {
    if (!hwnd || keys.isEmpty()) return false;

    if (!SetForegroundWindow(hwnd)) {
        qWarning() << "Failed to set foreground window for sendKeys";
        return false;
    }
    QThread::msleep(50);

    // 键盘字符到虚拟键码的映射表
    static const QHash<QChar, WORD> charToVKey = {
        // 字母
        {'A', 'A'}, {'B', 'B'}, {'C', 'C'}, {'D', 'D'}, {'E', 'E'},
        {'F', 'F'}, {'G', 'G'}, {'H', 'H'}, {'I', 'I'}, {'J', 'J'},
        {'K', 'K'}, {'L', 'L'}, {'M', 'M'}, {'N', 'N'}, {'O', 'O'},
        {'P', 'P'}, {'Q', 'Q'}, {'R', 'R'}, {'S', 'S'}, {'T', 'T'},
        {'U', 'U'}, {'V', 'V'}, {'W', 'W'}, {'X', 'X'}, {'Y', 'Y'},
        {'Z', 'Z'},
        // 数字
        {'0', '0'}, {'1', '1'}, {'2', '2'}, {'3', '3'}, {'4', '4'},
        {'5', '5'}, {'6', '6'}, {'7', '7'}, {'8', '8'}, {'9', '9'},
        // 特殊字符
        {' ', VK_SPACE},
        {'\t', VK_TAB},
        {'\n', VK_RETURN},
        {'\r', VK_RETURN},
        {'.', VK_OEM_PERIOD},
        {',', VK_OEM_COMMA},
        {';', VK_OEM_1},
        {'/', VK_OEM_2},
        {'\\', VK_OEM_5},
        {'[', VK_OEM_4},
        {']', VK_OEM_6},
        {'\'', VK_OEM_7},
        {'`', VK_OEM_3},
        {'-', VK_OEM_MINUS},
        {'=', VK_OEM_PLUS},
    };

    // 需要 Shift 修饰符的字符
    static const QHash<QChar, WORD> shiftChars = {
        {'!', '1'}, {'@', '2'}, {'#', '3'}, {'$', '4'}, {'%', '5'},
        {'^', '6'}, {'&', '7'}, {'*', '8'}, {'(', '9'}, {')', '0'},
        {'_', VK_OEM_MINUS}, {'+', VK_OEM_PLUS},
        {':', VK_OEM_1}, {'"', VK_OEM_7},
        {'?', VK_OEM_2}, {'|', VK_OEM_5},
        {'{', VK_OEM_4}, {'}', VK_OEM_6},
        {'<', VK_OEM_COMMA}, {'>', VK_OEM_PERIOD},
        {'~', VK_OEM_3},
    };

    bool shiftDown = false;
    auto typeChar = [&](QChar ch) {
        if (shiftChars.contains(ch)) {
            WORD baseKey = shiftChars[ch];
            // 按下 Shift
            keybd_event(VK_SHIFT, 0, 0, 0);
            QThread::msleep(10);
            // 按下基础键
            keybd_event(baseKey, 0, 0, 0);
            QThread::msleep(10);
            keybd_event(baseKey, 0, KEYEVENTF_KEYUP, 0);
            QThread::msleep(10);
            // 释放 Shift
            keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
            QThread::msleep(10);
        } else if (charToVKey.contains(ch)) {
            WORD vk = charToVKey[ch];
            keybd_event(vk, 0, 0, 0);
            QThread::msleep(10);
            keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
            QThread::msleep(10);
        }
    };

    for (int i = 0; i < keys.size(); i++) {
        QChar ch = keys[i];
        
        // 特殊组合键处理
        if (ch == '{') {
            // 查找闭合括号
            int closeIdx = keys.indexOf('}', i);
            if (closeIdx > i) {
                QString combo = keys.mid(i + 1, closeIdx - i - 1).toUpper();
                if (combo == "ENTER" || combo == "RETURN") {
                    pressKey(VK_RETURN);
                } else if (combo == "TAB") {
                    pressKey(VK_TAB);
                } else if (combo == "ESC") {
                    pressKey(VK_ESCAPE);
                } else if (combo == "DEL" || combo == "DELETE") {
                    pressKey(VK_DELETE);
                } else if (combo == "BACK" || combo == "BACKSPACE") {
                    pressKey(VK_BACK);
                } else if (combo == "CTRL+C") {
                    keybd_event(VK_CONTROL, 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('C', 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                } else if (combo == "CTRL+V") {
                    keybd_event(VK_CONTROL, 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('V', 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                } else if (combo == "CTRL+A") {
                    keybd_event(VK_CONTROL, 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('A', 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('A', 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                } else if (combo == "CTRL+X") {
                    keybd_event(VK_CONTROL, 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('X', 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('X', 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                } else if (combo == "CTRL+Z") {
                    keybd_event(VK_CONTROL, 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('Z', 0, 0, 0);
                    QThread::msleep(10);
                    keybd_event('Z', 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                    QThread::msleep(10);
                }
                i = closeIdx;
                continue;
            }
        }
        
        typeChar(ch);
    }

    return true;
}
