#include <string>
#include "authdialog.h"
#include "config.h"
#include "authentication.h"

AuthDialog::AuthDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(buttonOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(buttonCancelClicked()));

    srvIp[0] = '\0';

    rude::Config config;
    if (config.load("QtATS.ini")) {
        config.setSection("Authentication");
        const char* ip = config.getStringValue("SrvIp");
        if (ip) {
            strcpy(srvIp, ip);
            srvPort = config.getIntValue("SrvPort");
        }
    }
}

AuthDialog::~AuthDialog()
{
}

void AuthDialog::buttonOkClicked()
{
    std::string username = ui.username->text().toStdString();
    std::string passwd = ui.passwd->text().toStdString();

    if (srvIp[0] != '\0') {
        char name[32] = { 0 };
        char pwd[32] = { 0 };
        strncpy(name, username.c_str(), sizeof(name)-1);
        strncpy(pwd, passwd.c_str(), sizeof(pwd)-1);

        ui.username->setDisabled(true);
        ui.passwd->setDisabled(true);
        ui.okBtn->setDisabled(true);
        ui.cancelBtn->setDisabled(true);

        if (authenticate(username.c_str(), passwd.c_str(), srvIp, srvPort) == 1) {
            setWindowTitle("Authenticated!");
            accept();
        } else {
            setWindowTitle("Authenticate failed!");
        }

        ui.username->setEnabled(true);
        ui.passwd->setEnabled(true);
        ui.okBtn->setEnabled(true);
        ui.cancelBtn->setEnabled(true);
    }
}

void AuthDialog::buttonCancelClicked()
{
    reject();
}