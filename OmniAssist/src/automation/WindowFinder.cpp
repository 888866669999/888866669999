#include "WindowFinder.h"
#include <QDebug>

WindowFinder::WindowFinder() {
}

WindowFinder::~WindowFinder() {
}

HWND WindowFinder::findWindowByTitle(const QString& title) {
    return FindWindowW(nullptr, title.toStdWString().c_str());
}

HWND WindowFinder::findWindowByClass(const QString& className) {
    return FindWindowW(className.toStdWString().c_str(), nullptr);
}

QList<HWND> WindowFinder::findAllWindows(const QString& titleContains) {
    m_titleContains = titleContains;
    m_foundWindows.clear();
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(this));
    return m_foundWindows;
}

BOOL CALLBACK WindowFinder::enumWindowsProc(HWND hwnd, LPARAM lParam) {
    WindowFinder* finder = reinterpret_cast<WindowFinder*>(lParam);
    
    int length = GetWindowTextLengthW(hwnd);
    if (length > 0) {
        std::wstring buffer(length + 1, L'\0');
        GetWindowTextW(hwnd, &buffer[0], length + 1);
        QString title = QString::fromStdWString(buffer);
        
        if (title.contains(finder->m_titleContains, Qt::CaseInsensitive)) {
            if (IsWindowVisible(hwnd)) {
                finder->m_foundWindows.append(hwnd);
            }
        }
    }
    return TRUE;
}

HWND WindowFinder::findChildWindow(HWND parent, const QString& className, const QString& title) {
    return FindWindowExW(parent, nullptr, 
        className.isEmpty() ? nullptr : className.toStdWString().c_str(),
        title.isEmpty() ? nullptr : title.toStdWString().c_str());
}

bool WindowFinder::setForegroundWindow(HWND hwnd) {
    return ::SetForegroundWindow(hwnd) != FALSE;
}

bool WindowFinder::bringToFront(HWND hwnd) {
    if (!IsWindow(hwnd)) return false;
    
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    
    return true;
}
