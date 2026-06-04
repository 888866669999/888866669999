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

class WeChatAdapter;
class AIService;
class OpenAIProvider;
struct ChatMessage;

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

private:
    void setupUI();
    void setupConnections();
    void loadContacts();
    void loadChatHistory();

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

    WeChatAdapter* m_weChatAdapter;
    AIService* m_aiService;
    OpenAIProvider* m_openAIProvider;
    
    QString m_currentContactId;
    QList<ChatMessage> m_currentMessages;
};

#endif
