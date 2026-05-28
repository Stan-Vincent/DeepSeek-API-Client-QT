#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // 获取用户输入的设置
    QString getApiKey()const;
    QString getModelName() const;
    int getMaxTokens() const;
    double getTemperature() const;
    bool isThinkingEnabled() const;

    // 预先填充设置
    void setApiKey(const QString &key);
    void setModelName(const QString &model);
    void setMaxTokens(int tokens);
    void setTemperature(double temp);
    void setThinkingEnabled(bool enabled);


private:
    Ui::SettingsDialog *ui;

    QLineEdit *m_apiKeyEdit;
    QLineEdit *m_modelEdit;
    QSpinBox *m_maxTokensSpin;
    QDoubleSpinBox *m_temperatureSpin;
    QCheckBox *m_thinkingCheck;
};

#endif // SETTINGSDIALOG_H
