#include "stratposdialog.h"

StratPosDialog::StratPosDialog(
    int stratId,
    vector<StrategyPosition>& position,
    QWidget* parent)
    : pos(position)
    , QDialog(parent)
{
    ui.setupUi(this);

    posModel = new StratPosModel(position, this);
    posDelegate = new StratPosDelegate(this);

    ui.tableView->setModel(posModel);
    ui.tableView->setItemDelegate(posDelegate);
    ui.tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui.insertBtn, SIGNAL(clicked()), this, SLOT(buttonInsertClicked()));
    connect(ui.deleteBtn, SIGNAL(clicked()), this, SLOT(buttonDeleteClicked()));
    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(buttonOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(buttonCancelClicked()));
}

StratPosDialog::~StratPosDialog()
{
}

void StratPosDialog::buttonInsertClicked()
{
    posModel->insertNewRow();
}

void StratPosDialog::buttonDeleteClicked()
{
    posModel->removeRow(ui.tableView->currentIndex().row());
}

void StratPosDialog::buttonOkClicked()
{
    pos.clear();
    QList<StrategyPosition>& all = posModel->getAllPositions();
    for (int i = 0; i < all.count(); i++) {
        pos.push_back(all[i]);
    }

    QDialog::accept();
}

void StratPosDialog::buttonCancelClicked()
{
    QDialog::reject();
}
