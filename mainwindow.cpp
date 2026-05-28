#include "mainwindow.h"
#include "settingsdialog.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    // 监听输出框的滚动条，实现自动设定滚动条是否锁定的功能
    connect(ui->textEdit_Output->verticalScrollBar(),&QScrollBar::valueChanged,this,&MainWindow::onOutputScrollChanged);

    //输入框设置
    inPutSizeAutoSetting();

    qDebug()<<"backup";

    // 设置 Markdown 样式表 (CSS),现代风格
    QFile file("MarkDown.txt");

    if(file.open(QIODevice::ReadOnly)){
        QString styleSheet = file.readAll();
        ui->textEdit_Output->document()->setDefaultStyleSheet(styleSheet);

    }

    //连接菜单按键信号与槽
    initMenuAction();

    // 加载用户设置
    loadSettings();
    //设置亮色/暗色主题
    applyTheme(m_settings.theme);

    //禁用停止按钮
    ui->pushButton_Stop->setEnabled(false);

    //设置会话列表上下文菜单策略：部件能发出 QWidget::customContextMenuRequested() 信号
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    //网络访问API围绕一个QNetworkAccessManager对象构建
    manager = new QNetworkAccessManager(this);

    //给输入框添加事件过滤器
    ui->textEdit_Input->installEventFilter(this);

    //加载所有会话
    loadAllSessions();

    //选择对话
    if (m_sessions.isEmpty()==false)
    {
        switchToSession(0); //选择第一个对话
    }
    else
    {
        addNewSession("新对话");
        switchToSession(0);
    }

    //刷新对话列表
    refreshListWidget();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//重写输入框Key_Enter为发送，（Shift+Enter为换行）
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textEdit_Input && event->type() == QEvent::KeyPress) {

        //类型转化: QEvent --> QkeyEvent
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                on_pushButton_Send_clicked();
                return true;
            }
            else {
                return false;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

//发送按钮
void MainWindow::on_pushButton_Send_clicked()
{
    if(m_settings.apiKey == ""){
        QMessageBox::question(this,"错误","API Key为空");
        return ;
    }

    // 如果已有请求在进行，先中断
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    //获取输入文本
    QString text = ui->textEdit_Input->toPlainText();

    if (text.isEmpty())
        return;

    ui->textEdit_Input->clear();
    ui->textEdit_Output->moveCursor(QTextCursor::End);
    ui->textEdit_Output->insertPlainText("----> " + text + "\n");

    QTextCursor startCursor = ui->textEdit_Output->textCursor();
    startCursor.movePosition(QTextCursor::End);
    int replyStartPos = startCursor.position();

    //初始化申请
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    m_history.append(userMsg);

    QJsonObject requestBody;
    requestBody["messages"] = m_history;
    requestBody["model"] = m_settings.modelName;
    requestBody["max_tokens"] = m_settings.maxTokens;
    requestBody["temperature"] = m_settings.temperature;
    requestBody["stream"] = true;

    QJsonObject thinkObject;
    thinkObject["type"] = "enabled";
    if (m_settings.enableThinking) {    // 思考模式
        QJsonObject thinkObject;
        thinkObject["type"] = "enabled";
        requestBody["thinking"] = thinkObject;
    }

    QNetworkRequest request;
    request.setUrl(QUrl("https://api.deepseek.com/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_settings.apiKey.toUtf8());

    QNetworkReply *reply = manager->post(request, QJsonDocument(requestBody).toJson());

    m_currentReply = reply;

    // 启用停止按钮，禁用发送按钮
    ui->pushButton_Stop->setEnabled(true);
    ui->pushButton_Send->setEnabled(false);
    ui->statusBar->showMessage("回复生成中...");

    // 用于临时累积 AI 回复内容
    QString markdownBuffer;

    //持续读取API返回的文本输出
    connect(reply, &QNetworkReply::readyRead, this, [=]() mutable {

        while (reply->canReadLine()) {
            QByteArray line = reply->readLine();
            if (line.startsWith("data: ")) {
                QByteArray jsonData = line.mid(6).trimmed();

                // 流结束
                if (jsonData == "[DONE]") {
                    if (!markdownBuffer.isEmpty()) {
                        // 1. 删除从起始位置到末尾的内容
                        QTextDocument *doc = ui->textEdit_Output->document();
                        QTextCursor cursor(doc);
                        cursor.setPosition(replyStartPos);
                        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                        cursor.removeSelectedText();

                        // 2. 在相同位置插入 Markdown 渲染后的完整回复
                        appendMarkdownContent(markdownBuffer);

                        // 3. 保存历史记录
                        QJsonObject assistantMsg;
                        assistantMsg["role"] = "assistant";
                        assistantMsg["content"] = markdownBuffer;
                        m_history.append(assistantMsg);
                        updateCurrentSessionHistory();
                        markdownBuffer.clear();

                        ui->textEdit_Output->moveCursor(QTextCursor::End);
                    }
                    // 恢复界面状态
                    ui->pushButton_Stop->setEnabled(false);
                    ui->pushButton_Send->setEnabled(true);
                    ui->statusBar->showMessage("回复结束", 2000);
                    m_currentReply = nullptr;
                    break;
                }

                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
                if (error.error == QJsonParseError::NoError && doc.isObject()) {

                    QJsonObject obj = doc.object();
                    QJsonArray choices = obj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject delta = choices[0].toObject()["delta"].toObject();
                        QString content = delta["content"].toString();
                        if (!content.isEmpty()) {
                            // 累积原始 Markdown 文本
                            markdownBuffer += content;
                            tryScrollToBottom();
                            ui->textEdit_Output->insertPlainText(content);

                        }
                    }
                }
            }
        }
    });

    //回答结束后
    connect(reply, &QNetworkReply::finished, this, [=]() {
        // 如果 finished 时还没清理,这里做二次保障
        if (m_currentReply == reply) {
            m_currentReply = nullptr;
            ui->pushButton_Stop->setEnabled(false);
            ui->pushButton_Send->setEnabled(true);
        }

        ui->textEdit_Output->insertPlainText("\n\n");
        reply->deleteLater();
    });

    //出现错误
    connect(reply, &QNetworkReply::errorOccurred, this, [=](QNetworkReply::NetworkError code)
    {
        Q_UNUSED(code);
        if (reply != m_currentReply)
            return;

        QString errorMsg = reply->errorString();

        if (code == QNetworkReply::OperationCanceledError) {
            ui->statusBar->showMessage("已停止", 2000);
            // 移除刚添加的用户消息（可选）
            if (!m_history.isEmpty() && m_history.last().toObject()["role"].toString() == "user")
            {
                m_history.removeLast();
                updateCurrentSessionHistory();
                updateOutputFromHistory(); // 刷新显示，移除未完成的用户消息
            }
        }
        else
        {
            ui->statusBar->showMessage("错误: " + errorMsg, 3000);
            ui->textEdit_Output->insertPlainText("\n[错误] " + errorMsg + "\n\n");
        }
        // 清理
        ui->pushButton_Stop->setEnabled(false);
        ui->pushButton_Send->setEnabled(true);
        m_currentReply = nullptr;
        reply->deleteLater();
    });
}


//截断历史对话，防止对话过长
void MainWindow::truncateHistory(int maxMessages)
{
    if (m_history.size() > maxMessages) {
        QJsonArray newHistory;

        newHistory.append(m_history.first());//保留第一项：系统配置

        for (int i = m_history.size() - (maxMessages - 1); i < m_history.size(); ++i) {
            newHistory.append(m_history[i]);
        }
        m_history = newHistory;
        updateCurrentSessionHistory();
    }
}

//加载所有会话记录
void MainWindow::loadAllSessions()
{
    //会话文件路径
    QString path = QCoreApplication::applicationDirPath() + "/sessions.json";

    QFile file(path);
    if (!file.exists()) {   //文件不存在这添加默认文件
        QJsonObject defaultSession;
        defaultSession["id"] = 1;
        defaultSession["title"] = "对话 1";
        QJsonArray defaultHistory;
        QJsonObject sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = "You are a helpful assistant.";
        defaultHistory.append(sysMsg);
        defaultSession["history"] = defaultHistory;
        m_sessions.append(defaultSession);

        saveAllSessions();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法读取会话文件:" << file.errorString();
        return;
    }

    //获取文件
    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "会话文件解析失败，使用默认会话";
        return;
    }

    m_sessions.clear();
    QJsonArray array = doc.array();

    //文件转化到m_sessions
    foreach (const QJsonValue &val , array) {
        m_sessions.append(val.toObject());
    }

    for (int i = 0; i < m_sessions.size(); ++i) {
        QJsonObject session = m_sessions[i];
        QJsonArray history = session["history"].toArray();

        if (history.isEmpty() || history[0].toObject()["role"].toString() != "system") {
            QJsonObject sysMsg;
            sysMsg["role"] = "system";
            sysMsg["content"] = "You are a helpful assistant.";
            history.prepend(sysMsg);
            session["history"] = history;
            m_sessions[i] = session;
        }
    }
}

//保存所有会话记录
void MainWindow::saveAllSessions()
{
    QString path = QCoreApplication::applicationDirPath() + "/sessions.json";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法保存会话文件:" << file.errorString();
        return;
    }

    QJsonArray array;
    foreach (const QJsonObject &obj , m_sessions) {
        array.append(obj);
    }

    //写入文件
    QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented));
}

//添加新会话
void MainWindow::addNewSession(const QString &title)
{
    static int nextId = 1;
    foreach (const QJsonObject &sess , m_sessions) {
        if (sess["id"].toInt() >= nextId)
            nextId = sess["id"].toInt() + 1;
    }

    QJsonObject newSession;
    newSession["id"] = nextId;
    newSession["title"] = title.isEmpty() ? QString("对话 %1").arg(nextId) : title;
    QJsonArray history;
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = "You are a helpful assistant.";
    history.append(sysMsg);
    newSession["history"] = history;
    m_sessions.append(newSession);
    saveAllSessions();
    refreshListWidget();
}

//改变当前会话
void MainWindow::switchToSession(int index)
{
    //如果切换会中断当前请求
    if (m_currentReply != nullptr)
    {
        m_currentReply->abort();

        ui->pushButton_Stop->setEnabled(false);
        ui->pushButton_Send->setEnabled(true);
    }

    if (index < 0 || index >= m_sessions.size())
        return;
    if (index == m_currentSessionIndex)
        return;

    if (m_currentSessionIndex >= 0 && m_currentSessionIndex < m_sessions.size())
    {
        QJsonObject current = m_sessions[m_currentSessionIndex];
        current["history"] = m_history;
        m_sessions[m_currentSessionIndex] = current;
    }
    m_currentSessionIndex = index;
    m_history = m_sessions[index]["history"].toArray();
    updateOutputFromHistory();
    ui->listWidget->setCurrentRow(index);
}

//更新当前会话历史
void MainWindow::updateCurrentSessionHistory()
{
    if (m_currentSessionIndex >= 0 && m_currentSessionIndex < m_sessions.size()) {
        QJsonObject session = m_sessions[m_currentSessionIndex];
        session["history"] = m_history;
        m_sessions[m_currentSessionIndex] = session;
        saveAllSessions();
    }
}

//刷新会话历史表
void MainWindow::refreshListWidget()
{
    ui->listWidget->clear();

    for (int i = 0; i < m_sessions.size(); ++i)
    {
        QString title = m_sessions[i]["title"].toString();
        QListWidgetItem *item = new QListWidgetItem(title);
        item->setData(Qt::UserRole, i);
        ui->listWidget->addItem(item);
    }

    if (m_currentSessionIndex >= 0 && m_currentSessionIndex < ui->listWidget->count())
        ui->listWidget->setCurrentRow(m_currentSessionIndex);
}

//更新当前对话历史
void MainWindow::updateOutputFromHistory()
{
    ui->textEdit_Output->clear();

    ui->textEdit_Output->moveCursor(QTextCursor::End);


    foreach (const QJsonValue &val , m_history) {
        QJsonObject obj = val.toObject();
        QString role = obj["role"].toString();
        QString content = obj["content"].toString();

        if (role == "user")
        {
            appendMarkdownContent("----> " + content);
            ui->textEdit_Output->insertPlainText("\n");
        }
        else if (role == "assistant")
        {
            appendMarkdownContent(content);
            ui->textEdit_Output->insertPlainText("\n\n");
        }
    }
    ui->textEdit_Output->moveCursor(QTextCursor::End);
}

void MainWindow::appendMarkdownContent(const QString &text)
{
    // 标记处理，确保文档位置在末尾
    QTextCursor cursor = ui->textEdit_Output->textCursor();

    cursor.movePosition(QTextCursor::End);

    // 使用 insertMarkdown
    cursor.insertMarkdown(text);
}

void MainWindow::loadSettings()
{
    QString path = QCoreApplication::applicationDirPath() + "/settings.json";
    QFile file(path);
    if (!file.exists()) {
        // 默认值
        m_settings.apiKey = ""; //默认密钥留空
        m_settings.modelName = "deepseek-v4-pro";
        m_settings.maxTokens = 4096;
        m_settings.temperature = 1.0;
        m_settings.enableThinking = true;
        m_settings.theme = "light";
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isObject())
    {
        QJsonObject obj = doc.object();
        m_settings.apiKey = obj["apiKey"].toString();
        m_settings.modelName = obj["modelName"].toString();
        m_settings.maxTokens = obj["maxTokens"].toInt();
        m_settings.temperature = obj["temperature"].toDouble();
        m_settings.enableThinking = obj["enableThinking"].toBool();
        m_settings.theme = obj["theme"].toString();
        if (m_settings.theme.isEmpty())
            m_settings.theme = "light";
    }
}

void MainWindow::saveSettings()
{
    QString path = QCoreApplication::applicationDirPath() + "/settings.json";
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly))
        return;

    QJsonObject obj;
    obj["apiKey"] = m_settings.apiKey;
    obj["modelName"] = m_settings.modelName;
    obj["maxTokens"] = m_settings.maxTokens;
    obj["temperature"] = m_settings.temperature;
    obj["enableThinking"] = m_settings.enableThinking;
    obj["theme"] = m_settings.theme;

    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::tryScrollToBottom()
{
    if (m_autoScrollEnabled) {
        ui->textEdit_Output->moveCursor(QTextCursor::End);
        ui->textEdit_Output->ensureCursorVisible();
    }
}

void MainWindow::applyTheme(const QString &theme)
{
    QString stylePath =  theme + ".txt";
    QFile file(stylePath);
    if (file.open(QIODevice::ReadOnly)) {
        QString style = file.readAll();
        qApp->setStyleSheet(style);
        //qDebug() << "Loading theme from:" << stylePath;

        //qDebug()<< "---MarkDown.txt---\n\n";
    }
    else {
        qDebug() << "Failed to load theme file:" << stylePath;
    }
}

void MainWindow::initMenuAction()
{
    connect(ui->newChatAction,&QAction::triggered,this,&MainWindow::on_actionNewChat_clicked);
    connect(ui->renameChatAction,&QAction::triggered,this,&MainWindow::on_actionRenameChat_clicked);
    connect(ui->deleteChatAction,&QAction::triggered,this,&MainWindow::on_actionDeleteChat_clicked);
    connect(ui->clearHistoryAction,&QAction::triggered,this,&MainWindow::on_actionClearHistory_clicked);
    connect(ui->exitAction,&QAction::triggered,this,&MainWindow::close);
    connect(ui->settingsAction,&QAction::triggered,this,&MainWindow::on_actionSettings_triggered);
    connect(ui->exportChatAction,&QAction::triggered,this,&MainWindow::on_actionExportChat_triggered);
    connect(ui->toggleThemeAction, &QAction::triggered, this, &MainWindow::on_actionToggleTheme_triggered);
    ui->newChatAction->setShortcut(QKeySequence::New);
    ui->renameChatAction->setShortcut(QKeySequence("Ctrl+R"));
    ui->deleteChatAction->setShortcut(QKeySequence("Ctrl+D"));
    ui->clearHistoryAction->setShortcut(QKeySequence("Ctrl+H"));
    ui->exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    ui->settingsAction->setShortcut(QKeySequence("Ctrl+S"));
    ui->exportChatAction->setShortcut(QKeySequence("Ctrl+E"));
    ui->toggleThemeAction->setShortcut(QKeySequence("Ctrl+T"));

}

void MainWindow::inPutSizeAutoSetting()
{
    //实现输入框自动调整大小
    ui->textEdit_Input->setMinimumHeight(30);
    ui->textEdit_Input->setMaximumHeight(250);
    ui->textEdit_Input->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);

    // 在 textChanged 槽中动态调整最小高度
    connect(ui->textEdit_Input,&QTextEdit::textChanged,this,[this](){
        int contentHeight = ui->textEdit_Input->document()->size().height();
        int maxHeight = ui->textEdit_Input->maximumHeight();

        int newHeight = qMin(contentHeight,maxHeight);
        // 只调整最小高度，让布局重新计算
        ui->textEdit_Input->setMinimumHeight(newHeight+20);
        // 强制更新布局
        ui->textEdit_Input->updateGeometry();
    });
}

//清除当前历史对话记录
void MainWindow::on_actionClearHistory_clicked()
{
    if (m_currentSessionIndex < 0)
        return; //没有有效对话

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "清空当前历史记录",
        "确定要清空当前历史记录吗？",
        QMessageBox::Yes |QMessageBox::No);

    if(reply != QMessageBox::Yes){
        return ;
    }

    QJsonArray newHistory;
    if (!m_history.isEmpty() && m_history[0].toObject()["role"].toString() == "system") {
        newHistory.append(m_history[0]);
    } else {
        QJsonObject sys;
        sys["role"] = "system";
        sys["content"] = "You are a helpful assistant.";
        newHistory.append(sys);
    }
    m_history = newHistory;
    updateCurrentSessionHistory();
    updateOutputFromHistory();
    ui->textEdit_Output->insertPlainText("[系统] 当前对话历史已清空。\n\n");
}

//新对话
void MainWindow::on_actionNewChat_clicked()
{
    addNewSession();
    int newIndex = m_sessions.size() - 1;
    switchToSession(newIndex);
}

void MainWindow::on_actionRenameChat_clicked()
{
    // 重命名功能
    int index = m_currentSessionIndex;
    bool ok;
    QString newTitle = QInputDialog::getText(this, "重命名对话", "请输入新名称：",
                                             QLineEdit::Normal,
                                             m_sessions[index]["title"].toString(),
                                             &ok);
    if (ok && !newTitle.isEmpty()) {
        QJsonObject session = m_sessions[index];
        session["title"] = newTitle;
        m_sessions[index] = session;
        saveAllSessions();
        refreshListWidget();
        if (index == m_currentSessionIndex) {
            // 如果重命名的是当前会话，可以显示提示（可选）
            ui->textEdit_Output->insertPlainText("[系统] 当前对话已重命名为：" + newTitle + "\n\n");
        }
    }
}

//左击会话列表
void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    int index = item->data(Qt::UserRole).toInt();
    if (index != m_currentSessionIndex) {
        switchToSession(index);
    }
}

//删除当前会话
void MainWindow::on_actionDeleteChat_clicked()
{
    if (m_currentSessionIndex < 0 || m_currentSessionIndex >= m_sessions.size())
        return;

    // 确认删除对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "删除对话",
        "确定要删除当前对话「" + m_sessions[m_currentSessionIndex]["title"].toString() + "」吗？\n此操作不可撤销。",
        QMessageBox::Yes | QMessageBox::No
        );
    if (reply != QMessageBox::Yes)
        return;

    // 1. 中断当前正在进行的网络请求（如果有）
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        ui->pushButton_Stop->setEnabled(false);
        ui->pushButton_Send->setEnabled(true);
    }

    // 2. 直接清空输出区，避免残留
    ui->textEdit_Output->clear();

    // 3. 从会话列表中移除当前会话
    m_sessions.removeAt(m_currentSessionIndex);

    // 4. 处理索引变化
    if (m_sessions.isEmpty()) {
        // 没有会话了，新建一个默认会话
        addNewSession("新对话");
        m_currentSessionIndex = 0;
    } else {
        // 如果删除的不是最后一个，则保持原索引（因为后面的元素前移了）；否则指向最后一个
        if (m_currentSessionIndex >= m_sessions.size())
            m_currentSessionIndex = m_sessions.size() - 1;
    }

    // 5. 加载新会话的历史并刷新显示
    m_history = m_sessions[m_currentSessionIndex]["history"].toArray();

    updateOutputFromHistory();   // 这里会清空输出区并显示新会话的历史

    // 6. 保存并刷新列表
    saveAllSessions();
    refreshListWidget();
    ui->listWidget->setCurrentRow(m_currentSessionIndex);

    // 7. 重置自动滚动标志
    m_autoScrollEnabled = true;
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dlg(this);

    dlg.setStyleSheet(qApp->styleSheet());
    // 追加对话框背景（根据当前主题选择颜色）
    QString bgColor;
    if (m_settings.theme == "light") {
        bgColor = "#f0f4f8";   // 与主窗口背景一致
    } else {
        bgColor = "#1e293b";   // 深色主题背景
    }
    dlg.setStyleSheet(dlg.styleSheet() + QString(" QDialog { background-color: %1; }").arg(bgColor));

    // 预填当前设置
    dlg.setApiKey(m_settings.apiKey);
    dlg.setModelName(m_settings.modelName);
    dlg.setMaxTokens(m_settings.maxTokens);
    dlg.setTemperature(m_settings.temperature);
    dlg.setThinkingEnabled(m_settings.enableThinking);

    if(dlg.exec() == QDialog::Accepted){
        m_settings.apiKey = dlg.getApiKey();
        m_settings.modelName = dlg.getModelName();
        m_settings.maxTokens = dlg.getMaxTokens();
        m_settings.temperature = dlg.getTemperature();
        m_settings.enableThinking = dlg.isThinkingEnabled();
        saveSettings();
        QMessageBox::information(this, "设置已保存", "偏好设置已保存，下次请求将生效。");
    }
}

void MainWindow::on_actionExportChat_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,"导出对话","",
                            "MarkDown (*.md);;HTML (*.html);;纯文本 (*.txt)");
    if(fileName.isEmpty())
        return ;

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this,"错误","无法写入");
        return ;
    }
    if(fileName.endsWith(".md")){
        //导出MarkDown
        file.write(ui->textEdit_Output->toMarkdown().toUtf8());
    }
    else if(fileName.endsWith(".html")){
        // 导出 HTML
        file.write(ui->textEdit_Output->toHtml().toUtf8());
    }
    else if(fileName.endsWith(".txt")){
        // 默认纯文本
        file.write(ui->textEdit_Output->toPlainText().toUtf8());
    }
    else{
        QMessageBox::warning(this,"错误","无法导出");
        return ;
    }
    file.close();
    QMessageBox::information(this,"成功","对话已导出到 "+fileName);
}

void MainWindow::on_actionToggleTheme_triggered()
{
    if (m_settings.theme == "light") {
        m_settings.theme = "dark";
    } else {
        m_settings.theme = "light";
    }
    applyTheme(m_settings.theme);
    saveSettings();   // 保存用户偏好

}

//右击会话列表弹出菜单
void MainWindow::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget->itemAt(pos);
    if (!item)
        return;  // 没有点击到任何项

    QMenu menu(this);
    QAction *newChatAction = menu.addAction("新对话");
    QAction *renameAction = menu.addAction("重命名");
    QAction *clearChatAction = menu.addAction("清空对话");
    QAction *deleteAction = menu.addAction("删除对话");
    QAction *exportChatAction = menu.addAction("导出对话");


    QAction *selectedAction = menu.exec(ui->listWidget->viewport()->mapToGlobal(pos));
    if (selectedAction == newChatAction) {
        on_actionNewChat_clicked();
    }
    else if (selectedAction == renameAction) {
        on_actionRenameChat_clicked();
    }
    else if (selectedAction == clearChatAction) {
        on_actionClearHistory_clicked();
    }
    else if (selectedAction == deleteAction) {
        on_actionDeleteChat_clicked();
    }
    else if (selectedAction == exportChatAction) {
        on_actionExportChat_triggered();
    }



}

void MainWindow::onOutputScrollChanged(int value)
{
    QScrollBar *scrollBar = ui->textEdit_Output->verticalScrollBar();
    if (scrollBar == nullptr)
        return;

    // 检查是否滚动到底部（允许一定误差，最大值 - 可见范围 ≤ 5）
    bool atBottom = ( (scrollBar->maximum() - scrollBar->value() ) <= 5);

    if (atBottom == true) {
        m_autoScrollEnabled = true;
    } else {
        m_autoScrollEnabled = false;
    }
}


void MainWindow::on_pushButton_Stop_clicked()
{
    if (m_currentReply) {
        m_currentReply->abort();   // 中断请求
        // 注意：abort() 后，reply 会发射 errorOccurred 信号，我们可以在那里统一清理
        ui->statusBar->showMessage("已停止", 2000);
    }
}

