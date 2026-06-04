#include "MainWindow.h"
#include "../adapters/WeChatAdapter.h"
#include "../ai/AIService.h"
#include "../ai/providers/OpenAIProvider.h"
#include "../core/models/ChatMessage.h"
#include <QMessageBox>
#include <QDateTime>
#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();
    resize(1000, 700);
    setWindowTitle("OmniAssist - 全平台即时通讯智能中枢");
    
    m_weChatAdapter = new WeChatAdapter(this);
    m_aiService = new AIService();
    m_openAIProvider = new OpenAIProvider(this);
    m_aiService->setProvider(m_openAIProvider);
    
    loadContacts();
}

MainWindow::~MainWindow() {
    delete m_weChatAdapter;
    delete m_aiService;
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_contactList = new QListWidget(this);
    m_contactList->setMaximumWidth(300);
    m_contactList->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);

    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);
    m_chatView->setStyleSheet("QTextEdit { background-color: #1a1a2e; color: #eaeaea; }");

    auto* buttonLayout = new QHBoxLayout();
    m_aiReplyBtn = new QPushButton("AI 回复", this);
    m_summaryBtn = new QPushButton("生成摘要", this);
    m_personaBtn = new QPushButton("人物分析", this);
    buttonLayout->addWidget(m_aiReplyBtn);
    buttonLayout->addWidget(m_summaryBtn);
    buttonLayout->addWidget(m_personaBtn);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setStyleSheet("QTextEdit { background-color: #16213e; color: #eaeaea; }");
    m_sendBtn = new QPushButton("发送", this);

    rightLayout->addWidget(m_chatView);
    rightLayout->addLayout(buttonLayout);
    rightLayout->addWidget(m_inputEdit);
    rightLayout->addWidget(m_sendBtn);

    m_splitter->addWidget(m_contactList);
    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 3);

    auto* configWidget = new QWidget(this);
    auto* configLayout = new QHBoxLayout(configWidget);

    configLayout->addWidget(new QLabel("API 配置:", this));
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"OpenAI", "通义千问", "文心一言"});
    configLayout->addWidget(m_providerCombo);

    configLayout->addWidget(new QLabel("Key:", this));
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("请输入 API Key");
    configLayout->addWidget(m_apiKeyEdit);

    m_autoReplyCheck = new QCheckBox("自动回复", this);
    configLayout->addWidget(m_autoReplyCheck);

    mainLayout->addWidget(m_splitter);
    mainLayout->addWidget(configWidget);
}

void MainWindow::setupConnections() {
    connect(m_contactList, &QListWidget::itemClicked, this, &MainWindow::onContactClicked);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_aiReplyBtn, &QPushButton::clicked, this, &MainWindow::onAIReplyClicked);
    connect(m_summaryBtn, &QPushButton::clicked, this, &MainWindow::onSummaryClicked);
    connect(m_personaBtn, &QPushButton::clicked, this, &MainWindow::onPersonaClicked);
    connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &MainWindow::onApiKeyChanged);
}

void MainWindow::loadContacts() {
    m_contactList->clear();
    
    if (!m_weChatAdapter->isAvailable()) {
        m_chatView->setText("⚠️ 未检测到微信数据目录，请确保微信已安装并至少登录过一次");
        return;
    }

    QList<Contact> contacts = m_weChatAdapter->getContacts();
    
    if (contacts.isEmpty()) {
        m_chatView->setText("⚠️ 未能加载联系人列表，请确保微信正在运行");
        return;
    }

    for (const Contact& contact : contacts) {
        QString displayName = contact.remark.isEmpty() ? contact.name : contact.remark;
        QString itemText = QString("[微信] %1").arg(displayName);
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, contact.id);
        m_contactList->addItem(item);
    }
    
    m_chatView->setText("📋 已加载 " + QString::number(contacts.size()) + " 位联系人，请选择一个开始聊天");
}

void MainWindow::onContactClicked(QListWidgetItem* item) {
    if (!item) return;
    
    m_currentContactId = item->data(Qt::UserRole).toString();
    QString displayName = item->text();
    
    m_chatView->clear();
    m_chatView->append("<b>--- " + displayName + " ---</b>");
    m_chatView->append("");
    
    loadChatHistory();
}

void MainWindow::loadChatHistory() {
    if (m_currentContactId.isEmpty()) return;
    
    QList<ChatMessage> messages = m_weChatAdapter->getChatHistory(m_currentContactId, 50);
    m_currentMessages = messages;
    
    for (const ChatMessage& msg : messages) {
        QString sender = msg.isSelf ? "我" : msg.senderName;
        QString timeStr = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm");
        QString color = msg.isSelf ? "#4a9eff" : "#67c23a";
        
        QString html = QString("<span style='color:%1'>[%2] %3:</span> %4")
            .arg(color)
            .arg(timeStr)
            .arg(sender)
            .arg(msg.content);
        
        m_chatView->append(html);
    }
    
    if (messages.isEmpty()) {
        m_chatView->append("暂无聊天记录");
    }
}

void MainWindow::onSendClicked() {
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    
    m_inputEdit->clear();
    
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    m_chatView->append(QString("<span style='color:#4a9eff'>[%1] 我:</span> %2").arg(timeStr).arg(text));
    
    m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
}

void MainWindow::onAIReplyClicked() {
    if (m_currentMessages.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择联系人并加载聊天记录");
        return;
    }
    
    if (m_apiKeyEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入 API Key");
        return;
    }
    
    m_aiReplyBtn->setEnabled(false);
    
    QString reply = m_aiService->autoReply(m_currentMessages);
    
    if (!reply.isEmpty()) {
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
        m_chatView->append(QString("<span style='color:#ff9800'>[%1] AI:</span> %2").arg(timeStr).arg(reply));
        m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
    }
    
    m_aiReplyBtn->setEnabled(true);
}

void MainWindow::onSummaryClicked() {
    if (m_currentMessages.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择联系人并加载聊天记录");
        return;
    }
    
    if (m_apiKeyEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入 API Key");
        return;
    }
    
    m_summaryBtn->setEnabled(false);
    
    QJsonObject summary = m_aiService->generateMeetingSummary(m_currentMessages);
    
    m_chatView->append("");
    m_chatView->append("<b>📝 会议纪要</b>");
    m_chatView->append(summary["summary"].toString());
    
    m_summaryBtn->setEnabled(true);
}

void MainWindow::onPersonaClicked() {
    if (m_currentMessages.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择联系人并加载聊天记录");
        return;
    }
    
    if (m_apiKeyEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入 API Key");
        return;
    }
    
    m_personaBtn->setEnabled(false);
    
    QString analysis = m_aiService->analyzeContactPersona(m_currentMessages);
    
    m_chatView->append("");
    m_chatView->append("<b>👤 人物画像分析</b>");
    m_chatView->append(analysis);
    
    m_personaBtn->setEnabled(true);
}

void MainWindow::onApiKeyChanged(const QString& key) {
    m_openAIProvider->setApiKey(key);
}
