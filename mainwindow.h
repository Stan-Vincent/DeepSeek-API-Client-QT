#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QKeyEvent>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QTextDocumentFragment>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QSizePolicy>
#include <QScrollBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //构造函数
    explicit MainWindow(QWidget *parent = nullptr);
    //析构函数
    ~MainWindow() override;

protected:
    //重写事件过滤器（enter键）
    bool eventFilter(QObject* obj, QEvent *event) override;

private slots:
    //发送按钮
    void on_pushButton_Send_clicked();
    //停止按钮
    void on_pushButton_Stop_clicked();

    //清除对话
    void on_actionClearHistory_clicked();
    //新对话
    void on_actionNewChat_clicked();
    //重命名
    void on_actionRenameChat_clicked();
    //点击删除对话
    void on_actionDeleteChat_clicked();
    // API设置的槽函数
    void on_actionSettings_triggered();
    // 导出对话的槽函数
    void on_actionExportChat_triggered();
    //主题切换
    void on_actionToggleTheme_triggered();


    //左击对话列表
    void on_listWidget_itemClicked(QListWidgetItem *item);
    //右击对话按钮弹出菜单
    void on_listWidget_customContextMenuRequested(const QPoint &pos);
    // 输出框滚动
    void onOutputScrollChanged(int value);


private:
    Ui::MainWindow *ui;

    //控制器
    QNetworkAccessManager *manager;  
    // 当前对话历史
    QJsonArray m_history;
    //存储所有对话
    QList<QJsonObject> m_sessions;
    // 当前对话索引
    int m_currentSessionIndex = -1;
    // 保存当前正在进行的请求
    QNetworkReply *m_currentReply = nullptr;
    //是否允许自动滚动到底部
    bool m_autoScrollEnabled = true ;


    //设置属性
    struct UserSettings {
        QString apiKey;
        QString modelName;
        int maxTokens;
        double temperature;
        bool enableThinking;
        QString theme;   // "light" 或 "dark"
    } m_settings;

    //历史长度控制
    void truncateHistory(int maxMessages = 20);
    // 从文件加载所有会话
    void loadAllSessions();
    // 保存所有会话到文件
    void saveAllSessions();
    // 新建会话
    void addNewSession(const QString &title = "");
    // 切换到指定会话
    void switchToSession(int index);
    // 将当前 m_history 同步回 m_sessions
    void updateCurrentSessionHistory();
    // 刷新左侧列表显示
    void refreshListWidget();
    // 用 m_history 刷新聊天显示区
    void updateOutputFromHistory();
    //将累积的纯文本转换为Markdown显示
    void appendMarkdownContent(const QString &text);
    //加载设置
    void loadSettings();
    //保存设置
    void saveSettings();
    //尝试滚动到底部
    void tryScrollToBottom();
    //改变明暗主题
    void applyTheme(const QString &theme);
    //初始化菜单控件
    void initMenuAction();
    //设置输入框自动大小调整
    void inPutSizeAutoSetting();

};
#endif // MAINWINDOW_H
