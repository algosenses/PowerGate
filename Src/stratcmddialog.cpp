#include "stratcmddialog.h"

StratCmdDialog::StratCmdDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(buttonOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(buttonCancelClicked()));
}

StratCmdDialog::~StratCmdDialog()
{
}

QString StratCmdDialog::getCommand() const
{
    return ui.cmdLine->text();
}

void StratCmdDialog::buttonOkClicked()
{
    QDialog::accept();
}

void StratCmdDialog::buttonCancelClicked()
{
    QDialog::reject();
}
