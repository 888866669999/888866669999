#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void setupConnections();

    QSplitter* m_splitter;
    QListWidget* m_contactList;
    QTextEdit* m_chatView;
    QTextEdit* m_inputEdit;

    QComboBox* m_providerCombo;
    QLineEdit* m_apiKeyEdit;
    QCheckBox* m_autoReplyCheck;
    QPushButton* m_aiReplyBtn;
    QPushButton* m_summaryBtn;
    QPushButton* m_personaBtn;
    QPushButton* m_sendBtn;
};

#endif
