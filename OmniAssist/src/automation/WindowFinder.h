#ifndef WINDOWFINDER_H
#define WINDOWFINDER_H

#include <QString>
#include <QWindow>
#include <windows.h>

class WindowFinder {
public:
    WindowFinder();
    ~WindowFinder();

    HWND findWindowByTitle(const QString& title);
    HWND findWindowByClass(const QString& className);
    QList<HWND> findAllWindows(const QString& titleContains);
    HWND findChildWindow(HWND parent, const QString& className, const QString& title);
    bool setForegroundWindow(HWND hwnd);
    bool bringToFront(HWND hwnd);

private:
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
    QString m_titleContains;
    QList<HWND> m_foundWindows;
};

#endif
