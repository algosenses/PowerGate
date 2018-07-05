#ifndef POSITION_MODEL_H
#define POSITION_MODEL_H

#include <QAbstractItemModel>
#include "Defines.h"

typedef struct {
    int    srvId;
    char   instrument[32];
    int    side;
    int    total;
    int    yesterday;
    int    today;
    int    closable;
    int    own;
    double price;
    double margin;
    double posPnL;
    double realPnL;
} PositionItem;

class PositionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PositionModel(QObject* parent);
    ~PositionModel();

    void updatePosition(const AlgoSE::AccountPosition& pos);
    const QStringList& getHeaderLabels() const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    
    bool getPosItem(const char* instrument, PositionItem& item);
    bool getPosItem(int row, PositionItem& item);
    void getAllPosItems(std::vector<PositionItem>& items);

private:
    void copyPosItem(PositionItem& item, const AlgoSE::AccountPosition& pos);

private:
    QStringList headerLabels;
    QList<PositionItem> itemsList;

    int srvIdWidth;
    int instrumentWidth;
    int priceWidth;
    int quantityWidth;
    int actionWidth;
    int sideWidth;
    int pnlWidth;
};

#endif // POSITION_MODEL_H