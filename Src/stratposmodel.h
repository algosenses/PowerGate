#ifndef STRATEGY_POS_MODEL_H
#define STRATEGY_POS_MODEL_H

#include <vector>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include "Defines.h"

using std::vector;

using namespace AlgoSE;

#define DefaultInstrumentName "instrument"

class StratPosDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    StratPosDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class StratPosModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    StratPosModel(vector<StrategyPosition>& position, QObject *parent = 0);
    ~StratPosModel();

    QList<StrategyPosition>& getAllPositions();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void insertNewRow();
    void removeRow(int row);

private:
    QStringList headerLabels;
    QList<StrategyPosition> itemsList;
};

#endif // STRATEGY_POS_MODEL_H