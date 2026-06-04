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
#include <QList>
#include <QTabWidget>
#include <QFrame>

class WeChatAdapter;
class QQAdapter;
class AIService;
class OpenAIProvider;
struct ChatMessage;
struct Contact;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onContactClicked(QListWidgetItem* item);
    void onSendClicked();
    void onAIReplyClicked();
    void onSummaryClicked();
    void onPersonaClicked();
    void onApiKeyChanged(const QString& key);
    void onSearchChanged(const QString& text);
    void onPlatformChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void loadContacts();
    void loadChatHistory();

    QWidget* m_leftPanel;
    QWidget* m_rightPanel;
    QLineEdit* m_searchBar;
    QListWidget* m_contactList;
    QTextEdit* m_chatView;
    QTextEdit* m_inputEdit;
    QTextEdit* m_groupNotice;
    QListWidget* m_memberList;

    QWidget* m_titleBar;
    QLabel* m_contactNameLabel;

    QComboBox* m_providerCombo;
    QLineEdit* m_apiKeyEdit;
    QCheckBox* m_autoReplyCheck;
    QTabWidget* m_platformTabs;
    
    QPushButton* m_aiReplyBtn;
    QPushButton* m_summaryBtn;
    QPushButton* m_personaBtn;
    QPushButton* m_sendBtn;

    WeChatAdapter* m_weChatAdapter;
    QQAdapter* m_qqAdapter;
    AIService* m_aiService;
    OpenAIProvider* m_openAIProvider;
    
    Contact m_currentContact;
    QList<ChatMessage> m_currentMessages;
};

#endif
