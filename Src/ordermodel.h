#ifndef ORDER_MODEL_H
#define ORDER_MODEL_H

#include <unordered_map>
#include <string>
#include <QAbstractItemModel>
#include "Defines.h"

typedef struct {
    char   id[32];
    int    srvId;
    char   clOrdId[32];
    char   ordId[32];
    char   stratName[32]; // strategy name
    char   instrument[32];
    int    action;
    int    quantity;
    double price;
    double cost;
    int    traded;
    int    status;
    double avgPrice;
    char   datetime[32];
    int    closeEffect;
    QString statusMsg;
} OrderStatusItem;

class OrderModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    OrderModel(QObject* parent);
    ~OrderModel();

    void updateOrderStatus(const AlgoSE::Order& order);
    void updateExecution(const AlgoSE::Execution& execution);
    void updateStratStatus(const AlgoSE::StrategyStatus& state);
    const QStringList& getHeaderLabels() const;
    int getHeaderNumber() const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void getAllOrderItems(std::vector<OrderStatusItem>& orders);
    void getAllPendingOrders(std::vector<OrderStatusItem>& orders);
    bool getPendingOrder(int row, OrderStatusItem& order);

private:
    void copyOrderItem(OrderStatusItem& item, const AlgoSE::Order& status);

private:
    QStringList headerLabels;
    int headerNum;
    QList<OrderStatusItem> itemsList;

    std::unordered_map<int, std::string> stratNames;

    int idWidth;
    int instrumentWidth;
    int stratWidth;
    int priceWidth;
    int quantityWidth;
    int statusWidth;
    int actionWidth;
    int sideWidth;
    int datetimeWidth;
    int statusMsgWidth;
};

#endif // ORDER_MODEL_H