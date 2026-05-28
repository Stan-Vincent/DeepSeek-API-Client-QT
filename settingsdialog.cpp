#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowTitle("设置");



    m_apiKeyEdit = new QLineEdit;
    m_modelEdit = new QLineEdit;

    m_maxTokensSpin = new QSpinBox;
    m_maxTokensSpin->setRange(1,100000);

    m_temperatureSpin = new QDoubleSpinBox;
    m_temperatureSpin->setRange(0.0,2.0);

    m_thinkingCheck = new QCheckBox("启用思考模式");

    QFormLayout* formLayout = new QFormLayout;
    formLayout ->addRow("API Key:",m_apiKeyEdit);
    formLayout->addRow("模型名称:", m_modelEdit);
    formLayout->addRow("最大 Token 数:", m_maxTokensSpin);
    formLayout->addRow("温度 (0.0-2.0):", m_temperatureSpin);
    formLayout->addRow(m_thinkingCheck);

    // 创建标准按钮盒
    QDialogButtonBox *buttonBox =new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
    connect(buttonBox,&QDialogButtonBox::accepted,this,&QDialog::accept);
    connect(buttonBox,&QDialogButtonBox::rejected,this,&QDialog::reject);

    //恢复默认按钮
    QPushButton* restoreButton = new QPushButton("恢复默认",this);
    connect(restoreButton,&QPushButton::clicked,this,[this](){
        m_apiKeyEdit->setText("");   // API Key 清空
        m_modelEdit->setText("deepseek-v4-pro");   // 默认模型
        m_maxTokensSpin->setValue(4096);           // 默认最大 token
        m_temperatureSpin->setValue(1.0);          // 默认温度
        m_thinkingCheck->setChecked(true);         // 默认启用思考模式
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(restoreButton);
    buttonLayout->addStretch(); //加弹簧，使恢复默认按钮在左边左边
    buttonLayout->addWidget(buttonBox);


    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::getApiKey() const {
    return m_apiKeyEdit->text();
}

QString SettingsDialog::getModelName() const {
    return m_modelEdit->text();
}

int SettingsDialog::getMaxTokens() const {
    return m_maxTokensSpin->value();
}

double SettingsDialog::getTemperature() const {
    return m_temperatureSpin->value();
}

bool SettingsDialog::isThinkingEnabled() const {
    return m_thinkingCheck->isChecked();
}


void SettingsDialog::setApiKey(const QString &key) {
    m_apiKeyEdit->setText(key);
}

void SettingsDialog::setModelName(const QString &model) {
    m_modelEdit->setText(model);
}

void SettingsDialog::setMaxTokens(int tokens) {
    m_maxTokensSpin->setValue(tokens);
}

void SettingsDialog::setTemperature(double temp) {
    m_temperatureSpin->setValue(temp);
}

void SettingsDialog::setThinkingEnabled(bool enabled) {
    m_thinkingCheck->setChecked(enabled);
}
