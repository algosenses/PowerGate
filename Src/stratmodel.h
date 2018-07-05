#ifndef STRATEGY_MODEL_H
#define STRATEGY_MODEL_H

#include <string>
#include <vector>
#include <QAbstractItemModel>

#include "Defines.h"

using std::string;
using std::vector;

typedef struct {
	char instrument[32];
	int  size;
} PosItem;

typedef vector<PosItem> PosList;

class StratModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    typedef struct {
        int     strategyId;
        char    name[32];
        char    desc[64];
        int     status;
        bool    verbose;
        int     submittion;
        int     cancellation;
        PosList posLongList;
        PosList posShortList;
        char    log[512];
        bool    redAlert;
    } StratItem;

    StratModel(QObject* parent);
    ~StratModel();

    void updateStratStatus(const AlgoSE::StrategyStatus& state);
    void updateStratPositions(const AlgoSE::StrategyPosition& pos);
    void updateStratLog(const AlgoSE::StrategyLog& log);
    void clearRedAlert(int stratId);
    const QStringList& getHeaderLabels() const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    void getAllStratItems(std::vector<StratItem>& strategies);

private:
    void copyStratItem(StratItem& item, const AlgoSE::StrategyStatus& state);

private:
    QStringList headerLabels;
    QList<StratItem> itemsList;

    int idWidth;
    int nameWidth;
    int paramWidth;
    int ctrlWidth;
    int pauseWidth;
    int submitWidth;
    int cancelWidth;
    int longWidth;
    int shortWidth;
    int showWidth;
};

#endif // STRATEGY_MODEL_H