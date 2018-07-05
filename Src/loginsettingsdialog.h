#ifndef LOGIN_SETTINGS_DIALOG_H
#define LOGIN_SETTINGS_DIALOG_H

#include <QDialog>
#include "ui_loginsettingsdialog.h"

class LoginSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    LoginSettingsDialog(QWidget* parent = 0);
    ~LoginSettingsDialog();

    bool isRemoteMgrEnabled() const;
    void setRemoteMgrEnabled(bool enable);
    void setPort(int port);
    int  getPort() const;

private slots:
    void btnOkClicked();
    void btnCancelClicked();
    void portCheckBoxToggled(bool);

private:
    Ui::LoginSettingsDlg ui;
    int port;
    bool remoteMrgEnabled;
};

#endif // LOGIN_SETTINGS_DIALOG_H