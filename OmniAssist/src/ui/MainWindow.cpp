#include "MainWindow.h"
#include "../adapters/WeChatAdapter.h"
#include "../adapters/QQAdapter.h"
#include "../ai/AIService.h"
#include "../ai/providers/OpenAIProvider.h"
#include "../core/models/ChatMessage.h"
#include "../core/models/Platform.h"
#include <QMessageBox>
#include <QDateTime>
#include <QThread>
#include <QDebug>
#include <QFile>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();
    resize(1100, 700);
    setWindowTitle("OmniAssist - 全平台即时通讯智能中枢");
    
    m_weChatAdapter = new WeChatAdapter(this);
    m_qqAdapter = new QQAdapter(this);
    m_aiService = new AIService();
    m_openAIProvider = new OpenAIProvider(this);
    m_aiService->setProvider(m_openAIProvider);
    
    loadContacts();
}

MainWindow::~MainWindow() {
    delete m_weChatAdapter;
    delete m_qqAdapter;
    delete m_aiService;
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(2);

    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("搜索联系人...");
    m_searchBar->setStyleSheet("QLineEdit { padding: 8px 12px; border-radius: 20px; background-color: #2a2a3e; color: #eaeaea; border: none; }");
    m_searchBar->setMaximumWidth(280);
    leftLayout->addWidget(m_searchBar);
    leftLayout->addSpacing(8);

    m_contactList = new QListWidget(this);
    m_contactList->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a2e;
            color: #eaeaea;
            border: none;
        }
        QListWidget::item {
            padding: 12px;
            border-bottom: 1px solid #2a2a3e;
        }
        QListWidget::item:hover {
            background-color: #2a2a3e;
        }
        QListWidget::item:selected {
            background-color: #4a9eff;
        }
    )");
    m_contactList->setMaximumWidth(300);
    m_contactList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(m_contactList);

    m_splitter->addWidget(leftPanel);

    auto* centerPanel = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setStyleSheet("QWidget { background-color: #16213e; padding: 12px; }");
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    m_contactNameLabel = new QLabel("选择联系人", this);
    m_contactNameLabel->setStyleSheet("QLabel { color: #eaeaea; font-size: 14px; font-weight: bold; }");
    titleLayout->addWidget(m_contactNameLabel);
    titleLayout->addStretch();
    
    m_toolButton = new QPushButton(this);
    m_toolButton->setIcon(QIcon::fromTheme("system-search"));
    m_toolButton->setStyleSheet("QPushButton { background-color: transparent; border: none; color: #eaeaea; padding: 8px; }");
    titleLayout->addWidget(m_toolButton);
    centerLayout->addWidget(m_titleBar);

    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);
    m_chatView->setStyleSheet(R"(
        QTextEdit {
            background-color: #0f0f1a;
            color: #eaeaea;
            border: none;
            padding: 16px;
            font-size: 14px;
        }
    )");
    centerLayout->addWidget(m_chatView);

    auto* functionBar = new QWidget(this);
    functionBar->setStyleSheet("QWidget { background-color: #16213e; padding: 8px; }");
    auto* functionLayout = new QHBoxLayout(functionBar);
    
    m_aiReplyBtn = new QPushButton("🎯 AI 回复", this);
    m_summaryBtn = new QPushButton("📝 生成摘要", this);
    m_personaBtn = new QPushButton("👤 人物分析", this);
    
    QPushButton* btns[] = {m_aiReplyBtn, m_summaryBtn, m_personaBtn};
    for (auto btn : btns) {
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #4a9eff;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 6px;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #3a8eff;
            }
            QPushButton:disabled {
                background-color: #3a3a4e;
            }
        )");
        functionLayout->addWidget(btn);
    }
    functionLayout->addStretch();
    centerLayout->addWidget(functionBar);

    auto* inputBar = new QWidget(this);
    inputBar->setStyleSheet("QWidget { background-color: #16213e; padding: 12px; }");
    auto* inputLayout = new QVBoxLayout(inputBar);
    
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
        }
    )");
    inputLayout->addWidget(m_inputEdit);
    
    auto* sendLayout = new QHBoxLayout();
    m_sendBtn = new QPushButton("发送", this);
    m_sendBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4a9eff;
            color: white;
            border: none;
            padding: 8px 24px;
            border-radius: 6px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3a8eff;
        }
    )");
    sendLayout->addStretch();
    sendLayout->addWidget(m_sendBtn);
    inputLayout->addLayout(sendLayout);
    centerLayout->addWidget(inputBar);

    m_splitter->addWidget(centerPanel);

    m_memberList = new QListWidget(this);
    m_memberList->setMaximumWidth(220);
    m_memberList->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a2e;
            color: #eaeaea;
            border: none;
        }
        QListWidget::item {
            padding: 8px 12px;
            border-bottom: 1px solid #2a2a3e;
        }
        QListWidget::item:hover {
            background-color: #2a2a3e;
        }
    )");
    m_memberList->setVisible(false);
    m_splitter->addWidget(m_memberList);

    mainLayout->addWidget(m_splitter);

    auto* configBar = new QWidget(this);
    configBar->setStyleSheet("QWidget { background-color: #0f0f1a; padding: 8px 16px; }");
    auto* configLayout = new QHBoxLayout(configBar);
    
    configLayout->addWidget(new QLabel("API 配置:", this));
    configLayout->addWidget(new QLabel("📱", this));
    
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"OpenAI", "通义千问", "文心一言"});
    m_providerCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
        }
    )");
    configLayout->addWidget(m_providerCombo);
    
    configLayout->addWidget(new QLabel("Key:", this));
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("请输入 API Key");
    m_apiKeyEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
            min-width: 200px;
        }
    )");
    configLayout->addWidget(m_apiKeyEdit);
    
    m_autoReplyCheck = new QCheckBox("自动回复", this);
    m_autoReplyCheck->setStyleSheet("QCheckBox { color: #eaeaea; }");
    configLayout->addWidget(m_autoReplyCheck);
    
    m_platformTabs = new QTabWidget(this);
    m_platformTabs->addTab(new QWidget(), "💬 微信");
    m_platformTabs->addTab(new QWidget(), "🐧 QQ");
    m_platformTabs->setStyleSheet(R"(
        QTabWidget::tab-bar {
            alignment: center;
        }
        QTabBar::tab {
            background-color: #1a1a2e;
            color: #eaeaea;
            padding: 4px 12px;
            border-radius: 4px;
            margin: 0 4px;
        }
        QTabBar::tab:selected {
            background-color: #4a9eff;
        }
    )");
    configLayout->addStretch();
    configLayout->addWidget(m_platformTabs);
    
    mainLayout->addWidget(configBar);
}

void MainWindow::setupConnections() {
    connect(m_contactList, &QListWidget::itemClicked, this, &MainWindow::onContactClicked);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_aiReplyBtn, &QPushButton::clicked, this, &MainWindow::onAIReplyClicked);
    connect(m_summaryBtn, &QPushButton::clicked, this, &MainWindow::onSummaryClicked);
    connect(m_personaBtn, &QPushButton::clicked, this, &MainWindow::onPersonaClicked);
    connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &MainWindow::onApiKeyChanged);
    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    connect(m_platformTabs, &QTabWidget::currentChanged, this, &MainWindow::onPlatformChanged);
}

void MainWindow::loadContacts() {
    m_contactList->clear();
    
    int currentPlatform = m_platformTabs->currentIndex();
    IPlatformAdapter* adapter = (currentPlatform == 0) ? 
        static_cast<IPlatformAdapter*>(m_weChatAdapter) : 
        static_cast<IPlatformAdapter*>(m_qqAdapter);

    if (!adapter->isAvailable()) {
        QString platformName = currentPlatform == 0 ? "微信" : "QQ";
        m_chatView->setText(QString("⚠️ 未检测到%1数据目录，请确保%1已安装并至少登录过一次").arg(platformName));
        return;
    }

    QList<Contact> contacts = adapter->getContacts();
    
    if (contacts.isEmpty()) {
        QString platformName = currentPlatform == 0 ? "微信" : "QQ";
        m_chatView->setText(QString("⚠️ 未能加载%1联系人列表").arg(platformName));
        return;
    }

    for (const Contact& contact : contacts) {
        QString displayName = contact.remark.isEmpty() ? contact.name : contact.remark;
        QString platformIcon = currentPlatform == 0 ? "💬" : "🐧";
        QString itemText = QString("%1 %2").arg(platformIcon).arg(displayName);
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, QVariant::fromValue(contact));
        m_contactList->addItem(item);
    }
    
    m_chatView->setText("📋 已加载 " + QString::number(contacts.size()) + " 位联系人，请选择一个开始聊天");
}

void MainWindow::onContactClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<Contact>()) return;
    
    m_currentContact = data.value<Contact>();
    QString displayName = m_currentContact.remark.isEmpty() ? m_currentContact.name : m_currentContact.remark;
    
    m_contactNameLabel->setText(displayName);
    m_memberList->clear();
    
    loadChatHistory();
}

void MainWindow::loadChatHistory() {
    if (m_currentContact.id.isEmpty()) return;
    
    int currentPlatform = m_platformTabs->currentIndex();
    IPlatformAdapter* adapter = (currentPlatform == 0) ? 
        static_cast<IPlatformAdapter*>(m_weChatAdapter) : 
        static_cast<IPlatformAdapter*>(m_qqAdapter);
    
    QList<ChatMessage> messages = adapter->getChatHistory(m_currentContact.id, 50);
    m_currentMessages = messages;
    
    m_chatView->clear();
    
    for (const ChatMessage& msg : messages) {
        QString sender = msg.isSelf ? "我" : (msg.senderName.isEmpty() ? "未知" : msg.senderName);
        QString timeStr = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm");
        QString color = msg.isSelf ? "#4a9eff" : "#67c23a";
        
        QString html = QString(R"(
            <div style="margin-bottom: 8px;">
                <span style="color:%1; font-size: 12px;">[%2]</span>
                <span style="color:%1; font-weight: bold; margin-left: 8px;">%3:</span>
                <span style="margin-left: 8px;">%4</span>
            </div>
        )").arg(color).arg(timeStr).arg(sender).arg(msg.content);
        
        m_chatView->append(html);
    }
    
    if (messages.isEmpty()) {
        m_chatView->append("暂无聊天记录");
    }
    
    m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
}

void MainWindow::onSendClicked() {
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    
    m_inputEdit->clear();
    
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString html = QString(R"(
        <div style="margin-bottom: 8px;">
            <span style="color:#4a9eff; font-size: 12px;">[%1]</span>
            <span style="color:#4a9eff; font-weight: bold; margin-left: 8px;">我:</span>
            <span style="margin-left: 8px;">%2</span>
        </div>
    )").arg(timeStr).arg(text);
    
    m_chatView->append(html);
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
        QString html = QString(R"(
            <div style="margin-bottom: 8px; background-color: #2a2a3e; padding: 8px; border-radius: 8px;">
                <span style="color:#ff9800; font-size: 12px;">[%1]</span>
                <span style="color:#ff9800; font-weight: bold; margin-left: 8px;">AI:</span>
                <span style="margin-left: 8px;">%2</span>
            </div>
        )").arg(timeStr).arg(reply);
        
        m_chatView->append(html);
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
    m_chatView->append(R"(<div style="background-color: #2a2a3e; padding: 12px; border-radius: 8px;">)");
    m_chatView->append(R"(<span style="font-weight: bold; color: #4a9eff;">📝 会议纪要</span>)");
    m_chatView->append("");
    m_chatView->append(summary["summary"].toString());
    m_chatView->append("</div>");
    
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
    m_chatView->append(R"(<div style="background-color: #2a2a3e; padding: 12px; border-radius: 8px;">)");
    m_chatView->append(R"(<span style="font-weight: bold; color: #67c23a;">👤 人物画像分析</span>)");
    m_chatView->append("");
    m_chatView->append(analysis);
    m_chatView->append("</div>");
    
    m_personaBtn->setEnabled(true);
}

void MainWindow::onApiKeyChanged(const QString& key) {
    m_openAIProvider->setApiKey(key);
}

void MainWindow::onSearchChanged(const QString& text) {
    for (int i = 0; i < m_contactList->count(); ++i) {
        QListWidgetItem* item = m_contactList->item(i);
        QString itemText = item->text().toLower();
        bool visible = text.isEmpty() || itemText.contains(text.toLower());
        item->setHidden(!visible);
    }
}

void MainWindow::onPlatformChanged(int index) {
    m_currentContact = Contact();
    m_currentMessages.clear();
    loadContacts();
}
