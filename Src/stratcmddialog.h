#ifndef STRATEGY_CMD_DIALOG_H
#define STRATEGY_CMD_DIALOG_H

#include <QDialog>
#include "ui_stratcmd.h"

class StratCmdDialog : public QDialog
{
    Q_OBJECT

public:
    StratCmdDialog(QWidget* parent = 0);
    ~StratCmdDialog();
    QString getCommand() const;

private:
    Ui::StratCmd ui;

private slots:
    void buttonOkClicked();
    void buttonCancelClicked();
};

#endif // STRATEGY_CMD_DIALOG_H