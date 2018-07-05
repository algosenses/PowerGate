#ifndef STRATEGY_LOG_DIALOG_H
#define STRATEGY_LOG_DIALOG_H

#include <QDialog>
#include "Defines.h"
#include "stratlogmodel.h"
#include "ui_stratlog.h"

class StratLogDialog : public QDialog
{
    Q_OBJECT

public:
	StratLogDialog(int stratId, QWidget* parent = 0);
    ~StratLogDialog();
	void clear();
	void appendLog(const AlgoSE::StrategyLog& log);
	void closeEvent(QCloseEvent *event);
	void reject();

signals:
	void closeEvt(int);

private:
    Ui::StratLog ui;

	int stratId;
	StratLogModel* stratLogModel;
};

#endif // STRATEGY_LOG_DIALOG_H