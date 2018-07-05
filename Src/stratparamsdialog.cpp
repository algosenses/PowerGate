#include "stratparamsdialog.h"

StratParamsDialog::StratParamsDialog(
    const QString& strategy,
    vector<StrategyParam>& params,
    vector<StrategyParam>& outParams,
    QWidget* parent)
    : QDialog(parent)
    , touchedParams(outParams)
{
    ui.setupUi(this);

    setWindowTitle(strategy);
    setWindowFlags(Qt::WindowCloseButtonHint | parent->windowFlags());
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(buttonOkClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(buttonCancelClicked()));

    algoTraderParamModel = new StratParamsModel(StrategyParamCategory::PARAM_CATEGORY_ALGO_TRADER, this);
    algoTraderParamDelegate = new StratParamsDelegate(this);
    ui.algoTraderParamTableView->setModel(algoTraderParamModel);
    ui.algoTraderParamTableView->setItemDelegate(algoTraderParamDelegate);
    ui.algoTraderParamTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.algoTraderParamTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui.algoTraderGroupBox->hide();

    userParamModel = new StratParamsModel(StrategyParamCategory::PARAM_CATEGORY_USER, this);
    userParamDelegate = new StratParamsDelegate(this);
    ui.userParamTableView->setModel(userParamModel);
    ui.userParamTableView->setItemDelegate(userParamDelegate);
    ui.userParamTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.userParamTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
//    ui.userGroupBox->hide();

    sysParamModel = new StratParamsModel(StrategyParamCategory::PARAM_CATEGORY_SYSTEM, this);
    sysParamDelegate = new StratParamsDelegate(this);
    ui.systemParamTableView->setModel(sysParamModel);
    ui.systemParamTableView->setItemDelegate(sysParamDelegate);
    ui.systemParamTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.systemParamTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui.systemGroupBox->hide();

    bool hasParam = false;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].category == StrategyParamCategory::PARAM_CATEGORY_ALGO_TRADER) {
            hasParam = true;
            algoTraderParamModel->updateParamItem(params[i]);
        }
    }
    if (hasParam) {
        ui.algoTraderGroupBox->show();
    }

    hasParam = false;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].category == StrategyParamCategory::PARAM_CATEGORY_USER) {
            hasParam = true;
            userParamModel->updateParamItem(params[i]);
        }
    }
    if (hasParam) {
        ui.userGroupBox->show();
    }

    hasParam = false;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].category == StrategyParamCategory::PARAM_CATEGORY_SYSTEM) {
            hasParam = true;
            sysParamModel->updateParamItem(params[i]);
        }
    }
    if (hasParam) {
        ui.systemGroupBox->show();
    }
}

StratParamsDialog::~StratParamsDialog()
{
}

void StratParamsDialog::showWindow()
{
    show();
}

void StratParamsDialog::buttonOkClicked()
{
    algoTraderParamModel->getTouchedParams(touchedParams);
    userParamModel->getTouchedParams(touchedParams);

    QDialog::accept();
}

void StratParamsDialog::buttonCancelClicked()
{
    QDialog::reject();
}