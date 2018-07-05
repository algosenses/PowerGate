#include <QTimer>
#include <QCloseEvent>
#include "stratlogdialog.h"

StratLogDialog::StratLogDialog(int stratId, QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

	this->stratId = stratId;

	stratLogModel = new StratLogModel(this);
	ui.logsTableView->setModel(stratLogModel);
	ui.logsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.logsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui.logsTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	ui.logsTableView->horizontalHeader()->setMinimumSectionSize(0);
	QObject::connect(stratLogModel, &StratLogModel::rowsInserted, [this]()
	{
		if (ui.logsTableView->indexAt(ui.logsTableView->rect().bottomLeft()).row() == stratLogModel->rowCount() - 1) {
			QTimer::singleShot(0, ui.logsTableView, SLOT(scrollToBottom()));
		}
	});
}

StratLogDialog::~StratLogDialog()
{
}

void StratLogDialog::clear()
{
	stratLogModel->clear();
}

void StratLogDialog::appendLog(const AlgoSE::StrategyLog& log)
{
	stratLogModel->insertLogItem(log);
}

void StratLogDialog::reject()
{
	close();
}

void StratLogDialog::closeEvent(QCloseEvent *event)
{
	event->accept();
	emit closeEvt(stratId);
}