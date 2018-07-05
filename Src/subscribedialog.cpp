#include "subscribedialog.h"

SubscribeDialog::SubscribeDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(buttonOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(buttonCancelClicked()));
}

SubscribeDialog::~SubscribeDialog()
{
}

QString SubscribeDialog::getInstrument() const
{
    return ui.instrument->text();
}

void SubscribeDialog::setInstrument(const QString& text)
{
    ui.instrument->setText(text);
}

void SubscribeDialog::buttonOkClicked()
{
    QDialog::accept();
}

void SubscribeDialog::buttonCancelClicked()
{
    QDialog::reject();
}
