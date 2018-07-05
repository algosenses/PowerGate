#ifndef STRATEGY_PARAMS_DIALOG_H
#define STRATEGY_PARAMS_DIALOG_H

#include <vector>
#include <QDialog>
#include "ui_stratparams.h"
#include "stratparamsmodel.h"

using std::vector;

class StratParamsDialog : public QDialog
{
    Q_OBJECT

public:
    void showWindow();
    StratParamsDialog(
        const QString& strategy, 
        vector<StrategyParam>& params,
        vector<StrategyParam>& outParams,
        QWidget* parent = 0);
    ~StratParamsDialog();

private:
    Ui::StratParams ui;

    StratParamsModel* algoTraderParamModel;
    StratParamsDelegate* algoTraderParamDelegate;

    StratParamsModel* userParamModel;
    StratParamsDelegate* userParamDelegate;

    StratParamsModel* sysParamModel;
    StratParamsDelegate* sysParamDelegate;

    vector<StrategyParam>& touchedParams;

private slots:
    void buttonOkClicked();
    void buttonCancelClicked();
};

#endif // STRATEGY_PARAMS_DIALOG_H