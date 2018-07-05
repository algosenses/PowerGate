#ifndef STRATEGY_LOG_MODEL_H
#define STRATEGY_LOG_MODEL_H

#include <QAbstractItemModel>
#include "Defines.h"

class StratLogModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	StratLogModel(QObject *parent);
	~StratLogModel();

	void clear();
	void insertLogItem(const AlgoSE::StrategyLog& log);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
	QString getLogLevelStr(int level) const;

private:
	int dateWidth;
	QStringList headerLabels;

	typedef struct {
		char datetime[32];
        AlgoSE::StrategyLog log;
	} LogItem;

	QList<LogItem> itemsList;
};

#endif // STRATEGY_LOG_MODEL_H
