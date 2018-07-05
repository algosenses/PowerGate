#include <QStandardItemModel>
#include "loginsettingsdialog.h"

LoginSettingsDialog::LoginSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    port = 5501;
    remoteMrgEnabled = true;
    
    ui.portLineEdit->setText(QString().number(port));
    QIntValidator* validator = new QIntValidator(0, 0xffff, parent);
    ui.portLineEdit->setValidator(validator);

    connect(ui.remoteCheckBox, SIGNAL(toggled(bool)), this, SLOT(portCheckBoxToggled(bool)));

    ui.srvsCBox->addItem("CTP [Ctp.dll]");
    ui.srvsCBox->addItem("SGIT* [Sgit.dll]");
    ui.srvsCBox->addItem("TDX* [TDX.dll]");
    ui.srvsCBox->addItem("LMAX* [LMAX.dll]");
    ui.srvsCBox->addItem("Femas* [Femas.dll]");
    ui.srvsCBox->addItem("Zeusing* [Zeusing.dll]");
    ui.srvsCBox->addItem("UT* [UT.dll]");
    ui.srvsCBox->addItem("IB* [IB.dll]");
    ui.srvsCBox->addItem("BTCC* [BTCC.dll]");

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui.srvsCBox->model());
    bool disabled = true;
    int rows = ui.srvsCBox->count();
    for (int i = 1; i < rows; i++) {
        QStandardItem* item = model->item(i);
        item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled :
            item->flags() | Qt::ItemIsEnabled);
    }

    ui.algoCBox->addItem(tr("Disabled"));
    ui.algoCBox->addItem(tr("Enabled*"));
    model = qobject_cast<QStandardItemModel*>(ui.algoCBox->model());
    disabled = true;
    QStandardItem* item = model->item(1);
    item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled : item->flags() | Qt::ItemIsEnabled);

    ui.multiAcctCBox->addItem(tr("Disabled"));
    ui.multiAcctCBox->addItem(tr("Enabled*"));
    model = qobject_cast<QStandardItemModel*>(ui.multiAcctCBox->model());
    disabled = true;
    item = model->item(1);
    item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled : item->flags() | Qt::ItemIsEnabled);

    ui.localLoadCBox->addItem(tr("Disabled"));
    ui.localLoadCBox->addItem(tr("Enabled*"));
    model = qobject_cast<QStandardItemModel*>(ui.localLoadCBox->model());
    disabled = true;
    item = model->item(1);
    item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled : item->flags() | Qt::ItemIsEnabled);

    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(btnOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(btnCancelClicked()));
}

LoginSettingsDialog::~LoginSettingsDialog()
{
}

bool LoginSettingsDialog::isRemoteMgrEnabled() const
{
    return remoteMrgEnabled;
}

void LoginSettingsDialog::setRemoteMgrEnabled(bool enable)
{
    remoteMrgEnabled = enable;
    if (enable) {
        ui.remoteCheckBox->setChecked(true);
        ui.portLineEdit->setEnabled(true);
    } else {
        ui.remoteCheckBox->setChecked(false);
        ui.portLineEdit->setDisabled(true);
    }
}

void LoginSettingsDialog::setPort(int port)
{
    this->port = port;
    ui.portLineEdit->setText(QString().number(port));
}

int LoginSettingsDialog::getPort() const
{
    return port;
}

void LoginSettingsDialog::btnOkClicked()
{
    port = ui.portLineEdit->text().toInt();

    accept();
}

void LoginSettingsDialog::btnCancelClicked()
{
    reject();
}

void LoginSettingsDialog::portCheckBoxToggled(bool toggled)
{
    if (toggled) {
        ui.portLineEdit->setEnabled(true);
        remoteMrgEnabled = true;
    } else {
        ui.portLineEdit->setDisabled(true);
        remoteMrgEnabled = false;
    }
}