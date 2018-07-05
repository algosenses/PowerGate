#ifndef LOG_MODEL_H
#define LOG_MODEL_H

#include <QAbstractItemModel>
#include "Defines.h"

class LogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    LogModel(QObject *parent);
    ~LogModel();

    void insertLogItem(const AlgoSE::SystemLog& log);

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
    int levelWidth;
    QStringList headerLabels;

	typedef struct {
		char datetime[32];
		AlgoSE::SystemLog log;
	} LogItem;
    
	QList<LogItem> itemsList;
};

#endif // LOG_MODEL_H
