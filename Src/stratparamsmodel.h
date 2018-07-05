#ifndef STRATEGY_PARAMS_MODEL_H
#define STRATEGY_PARAMS_MODEL_H

#include <vector>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include "Defines.h"

using std::vector;

using namespace AlgoSE;

class StratParamsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    StratParamsDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class StratParamsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    StratParamsModel(int category, QObject *parent = 0);
    ~StratParamsModel();

    void updateParamItem(const AlgoSE::StrategyParam& param);
    const QStringList& getHeaderLabels() const;
    void getTouchedParams(vector<StrategyParam>& params);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    typedef struct {
        bool touch;
        StrategyParam param;
    } StratParamItem;

    QStringList headerLabels;
    QList<StratParamItem> itemsList;
    int paramCategory;
};

#endif // STRATEGY_PARAMS_MODEL_H