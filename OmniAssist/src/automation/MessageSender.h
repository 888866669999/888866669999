#ifndef MESSAGESENDER_H
#define MESSAGESENDER_H

#include <QString>
#include <QThread>
#include <memory>
#include "../core/models/Platform.h"

class WindowFinder;

class MessageSender : public QObject {
    Q_OBJECT

public:
    explicit MessageSender(QObject* parent = nullptr);
    ~MessageSender();

    bool sendText(Platform platform, const QString& contactId, const QString& contactName, const QString& text);
    bool sendFile(Platform platform, const QString& contactId, const QString& filePath);

private:
    void addRandomDelay();
    void simulateMouseMovement();
    bool sendTextViaClipboard(HWND hwnd, const QString& text);
    bool sendFileViaClipboard(HWND hwnd, const QString& filePath);
    bool sendKeys(HWND hwnd, const QString& keys);
    void pressKey(WORD key);

    std::unique_ptr<WindowFinder> m_windowFinder;
};

#endif
