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

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_CONTROL;
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.wVk = 'V';
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    input.ki.wVk = 'V';
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.wVk = VK_CONTROL;
    SendInput(1, &input, sizeof(INPUT));
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

    // 执行 Ctrl+V 粘贴文件
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_CONTROL;
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.wVk = 'V';
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    input.ki.wVk = 'V';
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(10);

    input.ki.wVk = VK_CONTROL;
    SendInput(1, &input, sizeof(INPUT));
    QThread::msleep(100);

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
    Q_UNUSED(hwnd);
    Q_UNUSED(keys);
    qDebug() << "sendKeys not fully implemented";
    return false;
}
