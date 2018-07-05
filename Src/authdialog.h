#ifndef AUTH_DIALOG_H
#define AUTH_DIALOG_H

#include <QDialog>
#include "ui_authdialog.h"

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    AuthDialog(QWidget* parent = 0);
    ~AuthDialog();

private:
    Ui::AuthDlg ui;
    char srvIp[64];
    int srvPort;

private slots:
    void buttonOkClicked();
    void buttonCancelClicked();
};

#endif // AUTH_DIALOG_H