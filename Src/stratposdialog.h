#ifndef STRATEGY_POS_DIALOG_H
#define STRATEGY_POS_DIALOG_H

#include <vector>
#include <QDialog>
#include "ui_stratpos.h"
#include "stratposmodel.h"

using std::vector;

class StratPosDialog : public QDialog
{
    Q_OBJECT

public:
    StratPosDialog(
        int stratId,
        // Input and Output 
        vector<StrategyPosition>& position,
        QWidget* parent = 0);
    ~StratPosDialog();

private:
    Ui::StratPos ui;

    vector<StrategyPosition>& pos;
    StratPosModel* posModel;
    StratPosDelegate* posDelegate;

private slots:
    void buttonInsertClicked();
    void buttonDeleteClicked();
    void buttonOkClicked();
    void buttonCancelClicked();
};

#endif // STRATEGY_POS_DIALOG_H