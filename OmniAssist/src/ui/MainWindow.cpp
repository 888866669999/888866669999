#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();
    resize(1000, 700);
    setWindowTitle("OmniAssist - 全平台即时通讯智能中枢");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_contactList = new QListWidget(this);
    m_contactList->setMaximumWidth(300);

    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);

    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);

    auto* buttonLayout = new QHBoxLayout();
    m_aiReplyBtn = new QPushButton("AI 回复", this);
    m_summaryBtn = new QPushButton("生成摘要", this);
    m_personaBtn = new QPushButton("人物分析", this);
    buttonLayout->addWidget(m_aiReplyBtn);
    buttonLayout->addWidget(m_summaryBtn);
    buttonLayout->addWidget(m_personaBtn);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(100);
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
    configLayout->addWidget(m_apiKeyEdit);

    m_autoReplyCheck = new QCheckBox("自动回复", this);
    configLayout->addWidget(m_autoReplyCheck);

    mainLayout->addWidget(m_splitter);
    mainLayout->addWidget(configWidget);
}

void MainWindow::setupConnections() {
}
