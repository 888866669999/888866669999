#include "MainWindow.h"
#include "../adapters/WeChatAdapter.h"
#include "../adapters/QQAdapter.h"
#include "../ai/AIService.h"
#include "../ai/providers/OpenAIProvider.h"
#include "../core/models/ChatMessage.h"
#include "../core/models/Contact.h"
#include "../core/models/Platform.h"
#include "../automation/MessageSender.h"
#include <QMessageBox>
#include <QDateTime>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <QJsonObject>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    qRegisterMetaType<Contact>();
    qRegisterMetaType<ChatMessage>();

    setupUI();
    setupConnections();
    resize(1200, 750);
    setWindowTitle("OmniAssist - 全平台即时通讯智能中枢");
    
    m_weChatAdapter = new WeChatAdapter(this);
    m_qqAdapter = new QQAdapter(this);
    m_aiService = new AIService();
    m_openAIProvider = new OpenAIProvider(this);
    m_aiService->setProvider(m_openAIProvider);
    m_messageSender = new MessageSender(this);
    
    loadContacts();
}

MainWindow::~MainWindow() {
    // m_weChatAdapter 和 m_qqAdapter 有 this 作为父对象，Qt 会自动处理析构
    // 只需要手动删除没有父对象的 m_aiService
    delete m_aiService;
}

void MainWindow::setupUI() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1a1a2e;
        }
    )");

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_leftPanel = new QWidget(this);
    m_leftPanel->setStyleSheet("QWidget { background-color: #1a1a2e; }");
    m_leftPanel->setFixedWidth(280);
    auto* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    auto* searchBarWidget = new QWidget(this);
    searchBarWidget->setStyleSheet("QWidget { background-color: #2a2a3e; padding: 8px; }");
    auto* searchLayout = new QHBoxLayout(searchBarWidget);
    
    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("搜索");
    m_searchBar->setStyleSheet(R"(
        QLineEdit {
            background-color: #3a3a4e;
            color: #eaeaea;
            border: none;
            border-radius: 20px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QLineEdit::placeholder {
            color: #6a6a7e;
        }
    )");
    searchLayout->addWidget(m_searchBar);
    
    auto* addBtn = new QPushButton(this);
    addBtn->setIcon(QIcon::fromTheme("list-add"));
    addBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; color: #eaeaea; padding: 8px; }");
    searchLayout->addWidget(addBtn);
    
    leftLayout->addWidget(searchBarWidget);

    m_contactList = new QListWidget(this);
    m_contactList->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a2e;
            color: #eaeaea;
            border: none;
        }
        QListWidget::item {
            padding: 10px;
            border-bottom: 1px solid #2a2a3e;
        }
        QListWidget::item:hover {
            background-color: #2a2a3e;
        }
        QListWidget::item:selected {
            background-color: #4a9eff;
            border-left: 3px solid #00d4ff;
        }
    )");
    m_contactList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(m_contactList);

    mainLayout->addWidget(m_leftPanel);

    auto* centerPanel = new QWidget(this);
    centerPanel->setStyleSheet("QWidget { background-color: #0f0f1a; }");
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setStyleSheet("QWidget { background-color: #16213e; padding: 12px 16px; }");
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    
    m_contactNameLabel = new QLabel("选择联系人", this);
    m_contactNameLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; }");
    titleLayout->addWidget(m_contactNameLabel);
    
    titleLayout->addStretch();
    
    auto* actionButtons = new QWidget(this);
    auto* actionLayout = new QHBoxLayout(actionButtons);
    
    auto* callBtn = new QPushButton(this);
    callBtn->setIcon(QIcon::fromTheme("call-start"));
    callBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; padding: 8px; }");
    actionLayout->addWidget(callBtn);
    
    auto* videoBtn = new QPushButton(this);
    videoBtn->setIcon(QIcon::fromTheme("video"));
    videoBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; padding: 8px; }");
    actionLayout->addWidget(videoBtn);
    
    auto* moreBtn = new QPushButton(this);
    moreBtn->setIcon(QIcon::fromTheme("more"));
    moreBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; padding: 8px; }");
    actionLayout->addWidget(moreBtn);
    
    titleLayout->addWidget(actionButtons);
    
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
            line-height: 1.5;
        }
        QTextEdit QScrollBar:vertical {
            width: 6px;
            background-color: #1a1a2e;
        }
        QTextEdit QScrollBar::handle:vertical {
            background-color: #4a4a5e;
            border-radius: 3px;
        }
    )");
    centerLayout->addWidget(m_chatView);

    auto* functionBar = new QWidget(this);
    functionBar->setStyleSheet("QWidget { background-color: #16213e; padding: 8px 16px; }");
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
                padding: 8px 20px;
                border-radius: 20px;
                font-size: 13px;
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
    inputBar->setStyleSheet("QWidget { background-color: #16213e; padding: 12px 16px; }");
    auto* inputLayout = new QVBoxLayout(inputBar);
    
    auto* toolBar = new QWidget(this);
    auto* toolLayout = new QHBoxLayout(toolBar);
    
    QIcon icons[] = {
        QIcon::fromTheme("insert-emoji"),
        QIcon::fromTheme("image"),
        QIcon::fromTheme("video"),
        QIcon::fromTheme("file"),
        QIcon::fromTheme("mic"),
        QIcon::fromTheme("smile")
    };
    
    for (int i = 0; i < 6; ++i) {
        QPushButton* toolBtn = new QPushButton(this);
        toolBtn->setIcon(icons[i]);
        toolBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; padding: 8px; }");
        toolLayout->addWidget(toolBtn);
    }
    toolLayout->addStretch();
    
    inputLayout->addWidget(toolBar);
    
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(120);
    m_inputEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            border-radius: 8px;
            padding: 12px;
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
            padding: 10px 32px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3a8eff;
        }
        QPushButton:disabled {
            background-color: #3a3a4e;
        }
    )");
    sendLayout->addStretch();
    sendLayout->addWidget(m_sendBtn);
    inputLayout->addLayout(sendLayout);
    
    centerLayout->addWidget(inputBar);

    mainLayout->addWidget(centerPanel, 1);

    m_rightPanel = new QWidget(this);
    m_rightPanel->setStyleSheet("QWidget { background-color: #1a1a2e; }");
    m_rightPanel->setFixedWidth(240);
    m_rightPanel->setVisible(false);
    auto* rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto* groupNoticeTab = new QWidget(this);
    groupNoticeTab->setStyleSheet("QWidget { padding: 12px; }");
    auto* noticeLayout = new QVBoxLayout(groupNoticeTab);
    
    auto* noticeTitle = new QLabel("群公告", this);
    noticeTitle->setStyleSheet("QLabel { color: #eaeaea; font-size: 14px; font-weight: bold; }");
    noticeLayout->addWidget(noticeTitle);
    
    m_groupNotice = new QTextEdit(this);
    m_groupNotice->setReadOnly(true);
    m_groupNotice->setMaximumHeight(150);
    m_groupNotice->setStyleSheet(R"(
        QTextEdit {
            background-color: transparent;
            color: #a0a0b0;
            border: none;
            font-size: 12px;
            margin-top: 8px;
        }
    )");
    m_groupNotice->setText("暂无群公告");
    noticeLayout->addWidget(m_groupNotice);
    
    rightLayout->addWidget(groupNoticeTab);
    
    auto* separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("QFrame { color: #2a2a3e; }");
    rightLayout->addWidget(separator1);

    auto* memberTab = new QWidget(this);
    memberTab->setStyleSheet("QWidget { padding: 12px; }");
    auto* memberLayout = new QVBoxLayout(memberTab);
    
    auto* memberTitle = new QLabel("群聊成员", this);
    memberTitle->setStyleSheet("QLabel { color: #eaeaea; font-size: 14px; font-weight: bold; }");
    memberLayout->addWidget(memberTitle);
    
    m_memberList = new QListWidget(this);
    m_memberList->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            color: #eaeaea;
            border: none;
        }
        QListWidget::item {
            padding: 6px 8px;
            font-size: 12px;
        }
        QListWidget::item:hover {
            background-color: #2a2a3e;
            border-radius: 4px;
        }
    )");
    memberLayout->addWidget(m_memberList);
    
    rightLayout->addWidget(memberTab);
    
    mainLayout->addWidget(m_rightPanel);

    auto* statusBar = new QWidget(this);
    statusBar->setStyleSheet("QWidget { background-color: #0f0f1a; padding: 6px 16px; }");
    auto* statusLayout = new QHBoxLayout(statusBar);
    
    statusLayout->addWidget(new QLabel("API 配置:", this));
    
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"OpenAI", "通义千问", "文心一言"});
    m_providerCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 12px;
            min-width: 100px;
        }
        QComboBox QAbstractItemView {
            background-color: #2a2a3e;
            color: #eaeaea;
        }
    )");
    statusLayout->addWidget(m_providerCombo);
    
    statusLayout->addWidget(new QLabel("Key:", this));
    
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("API Key");
    m_apiKeyEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #2a2a3e;
            color: #eaeaea;
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 12px;
            min-width: 200px;
        }
    )");
    statusLayout->addWidget(m_apiKeyEdit);
    
    m_autoReplyCheck = new QCheckBox("自动回复", this);
    m_autoReplyCheck->setStyleSheet("QCheckBox { color: #eaeaea; font-size: 12px; }");
    statusLayout->addWidget(m_autoReplyCheck);
    
    statusLayout->addStretch();
    
    m_platformTabs = new QTabWidget(this);
    m_platformTabs->addTab(new QWidget(), "💬 微信");
    m_platformTabs->addTab(new QWidget(), "🐧 QQ");
    m_platformTabs->setStyleSheet(R"(
        QTabBar::tab {
            background-color: #2a2a3e;
            color: #eaeaea;
            padding: 4px 16px;
            border-radius: 12px;
            margin: 0 4px;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            background-color: #4a9eff;
            color: white;
        }
    )");
    statusLayout->addWidget(m_platformTabs);
    
    statusLayout->addSpacing(16);
    
    auto* statusLabel = new QLabel("✓ 已连接", this);
    statusLabel->setStyleSheet("QLabel { color: #67c23a; font-size: 12px; }");
    statusLayout->addWidget(statusLabel);
    
    mainLayout->addWidget(statusBar, 0, Qt::AlignBottom);
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

    if (!adapter->initialize()) {
        QString platformName = currentPlatform == 0 ? "微信" : "QQ";
        m_chatView->setText(QString("<div style='color:#ff6b6b; padding: 20px;'>⚠️ 无法初始化%1，请确保%1已安装并至少登录过一次</div>").arg(platformName));
        return;
    }

    if (!adapter->isAvailable()) {
        QString platformName = currentPlatform == 0 ? "微信" : "QQ";
        m_chatView->setText(QString("<div style='color:#ff6b6b; padding: 20px;'>⚠️ 未检测到%1数据目录，请确保%1已安装并至少登录过一次</div>").arg(platformName));
        return;
    }

    QList<Contact> contacts = adapter->getContacts();
    
    if (contacts.isEmpty()) {
        QString platformName = currentPlatform == 0 ? "微信" : "QQ";
        m_chatView->setText(QString("<div style='color:#ff6b6b; padding: 20px;'>⚠️ 未能加载%1联系人列表</div>").arg(platformName));
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
    
    m_chatView->setText(QString("<div style='color:#a0a0b0; padding: 20px;'>📋 已加载 %1 位联系人，请选择一个开始聊天</div>").arg(contacts.size()));
}

void MainWindow::onContactClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<Contact>()) return;
    
    m_currentContact = data.value<Contact>();
    QString displayName = m_currentContact.remark.isEmpty() ? m_currentContact.name : m_currentContact.remark;
    
    m_contactNameLabel->setText(displayName);
    
    bool isGroup = m_currentContact.id.startsWith("group_");
    m_rightPanel->setVisible(isGroup);
    
    if (isGroup) {
        m_memberList->clear();
        m_groupNotice->setText("暂无群公告");
        for (int i = 1; i <= 10; ++i) {
            QString role = (i == 1) ? "👑 群主" : (i <= 3) ? "🔧 管理员" : "";
            QString memberName = QString("成员%1").arg(i);
            QListWidgetItem* memberItem = new QListWidgetItem(memberName + " " + role);
            m_memberList->addItem(memberItem);
        }
    }
    
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
    
    QString lastDate;
    
    for (const ChatMessage& msg : messages) {
        QString dateStr = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("yyyy-MM-dd");
        QString timeStr = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm");
        
        if (dateStr != lastDate) {
            lastDate = dateStr;
            m_chatView->append(QString(R"(
                <div style="text-align: center; margin: 16px 0;">
                    <span style="background-color: #2a2a3e; padding: 4px 16px; border-radius: 10px; font-size: 12px; color: #8a8a9e;">%1</span>
                </div>
            )").arg(dateStr));
        }
        
        QString sender = msg.isSelf ? "我" : (msg.senderName.isEmpty() ? "未知" : msg.senderName);
        QString bubbleColor = msg.isSelf ? "#4a9eff" : "#2a2a3e";
        QString textColor = msg.isSelf ? "#ffffff" : "#eaeaea";
        QString align = msg.isSelf ? "right" : "left";
        
        // 对消息内容进行 HTML 转义以防止注入
        QString escapedContent = msg.content.toHtmlEscaped();
        
        QString html = QString(R"(
            <div style="display: flex; justify-content: %1; margin-bottom: 12px;">
                <div style="max-width: 70%;">
                    <div style="color: #8a8a9e; font-size: 12px; margin-bottom: 4px; padding: 0 8px;">%2</div>
                    <div style="background-color: %3; border-radius: 12px; padding: 10px 14px; color: %4;">
                        %5
                    </div>
                    <div style="color: #6a6a7e; font-size: 10px; margin-top: 4px; padding: 0 8px; text-align: right;">%6</div>
                </div>
            </div>
        )").arg(align).arg(sender.toHtmlEscaped()).arg(bubbleColor).arg(textColor).arg(escapedContent).arg(timeStr.toHtmlEscaped());
        
        m_chatView->append(html);
    }
    
    if (messages.isEmpty()) {
        m_chatView->append("<div style='color:#a0a0b0; padding: 20px;'>暂无聊天记录</div>");
    }
    
    m_chatView->verticalScrollBar()->setValue(m_chatView->verticalScrollBar()->maximum());
}

void MainWindow::onSendClicked() {
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    
    if (m_currentContact.id.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择联系人");
        return;
    }
    
    m_inputEdit->clear();
    
    Platform platform = (m_platformTabs->currentIndex() == 0) ? Platform::WeChat : Platform::QQ;
    bool sent = m_messageSender->sendText(platform, m_currentContact.id, text);
    
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    
    QString html = QString(R"(
        <div style="display: flex; justify-content: right; margin-bottom: 12px;">
            <div style="max-width: 70%;">
                <div style="color: #8a8a9e; font-size: 12px; margin-bottom: 4px; padding: 0 8px;">我</div>
                <div style="background-color: #4a9eff; border-radius: 12px; padding: 10px 14px; color: white;">
                    %1
                </div>
                <div style="color: #6a6a7e; font-size: 10px; margin-top: 4px; padding: 0 8px; text-align: right;">%2%3</div>
            </div>
        </div>
    )").arg(text.toHtmlEscaped()).arg(timeStr.toHtmlEscaped()).arg(sent ? "" : " ⚠️ 发送失败");
    
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
            <div style="display: flex; justify-content: left; margin-bottom: 12px;">
                <div style="max-width: 70%;">
                    <div style="color: #ff9800; font-size: 12px; margin-bottom: 4px; padding: 0 8px;">🤖 AI</div>
                    <div style="background-color: #2a2a3e; border-radius: 12px; padding: 10px 14px; color: #eaeaea; border: 1px solid #ff9800;">
                        %1
                    </div>
                    <div style="color: #6a6a7e; font-size: 10px; margin-top: 4px; padding: 0 8px; text-align: right;">%2</div>
                </div>
            </div>
        )").arg(reply.toHtmlEscaped()).arg(timeStr.toHtmlEscaped());
        
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
    m_chatView->append(R"(<div style="background-color: #2a2a3e; padding: 16px; border-radius: 12px; margin: 12px 0;">)");
    m_chatView->append(R"(<div style="font-weight: bold; color: #4a9eff; font-size: 14px; margin-bottom: 12px;">📝 会议纪要</div>)");
    m_chatView->append(R"(<div style="color: #eaeaea; line-height: 1.6;">)" + summary["summary"].toString().toHtmlEscaped() + "</div>");
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
    m_chatView->append(R"(<div style="background-color: #2a2a3e; padding: 16px; border-radius: 12px; margin: 12px 0;">)");
    m_chatView->append(R"(<div style="font-weight: bold; color: #67c23a; font-size: 14px; margin-bottom: 12px;">👤 人物画像分析</div>)");
    m_chatView->append(R"(<div style="color: #eaeaea; line-height: 1.6;">)" + analysis.toHtmlEscaped() + "</div>");
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
    m_rightPanel->setVisible(false);
    loadContacts();
}
