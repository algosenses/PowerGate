#include "main.h"
#include "ordermodel.h"

OrderModel::OrderModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    headerLabels << tr("ID");
    headerLabels << tr("Strategy");
    headerLabels << tr("Instrument");
    headerLabels << tr("Side");
    headerLabels << tr("Action");
    headerLabels << tr("Status");
    headerLabels << tr("Price");
    headerLabels << tr("AvgPrice");
    headerLabels << tr("Quantity");
    headerLabels << tr("Traded");
    headerLabels << tr("DateTime");
    headerLabels << tr("StatusMsg");
    headerLabels << "";

    headerNum = headerLabels.size() - 1;

    idWidth = 90;
    instrumentWidth = 80;
    stratWidth = 90;
    priceWidth = 80;
    quantityWidth = 60;
    statusWidth = 80;
    actionWidth = 50;
    sideWidth = 50;
    datetimeWidth = 170;
    statusMsgWidth = 500;
}

OrderModel::~OrderModel()
{
    itemsList.clear();
}

const QStringList& OrderModel::getHeaderLabels() const
{
    return headerLabels;
}

int OrderModel::getHeaderNumber() const
{
    return headerNum;
}

QModelIndex OrderModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex OrderModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant OrderModel::data(const QModelIndex &index, int role) const
{
    int currentRow = itemsList.count() - index.row() - 1;
    if (currentRow < 0 || currentRow >= itemsList.count())
        return QVariant();

    if (role != Qt::DisplayRole &&
        role != Qt::ToolTipRole &&
        role != Qt::ForegroundRole &&
        role != Qt::BackgroundRole &&
        role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    int indexColumn = index.column();
    int indexRow = index.row();

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft | Qt::AlignVCenter;
    }

    if (role == Qt::BackgroundRole)
    {
        if (!(indexRow % 2)) {
            return appGlobalData.appTheme.altRowColor;
        } else {
            return QVariant();
        }
    }

    if (role == Qt::ForegroundRole) {
        if (indexColumn == 3 && indexRow < itemsList.size()) {
            if (itemsList[indexRow].action == OrderAction::BUY ||
                itemsList[indexRow].action == OrderAction::BUY_TO_COVER) {
                return appGlobalData.appTheme.red;
            } else if (itemsList[indexRow].action == OrderAction::SELL ||
                itemsList[indexRow].action == OrderAction::SELL_SHORT) {
                return appGlobalData.appTheme.darkGreen;
            }
        }

        if (indexColumn == 5 && indexRow < itemsList.size()) {
            const OrderStatusItem& item = itemsList[indexRow];
            if (item.status == OrderState::CANCELED ||
                item.status == OrderState::REJECTED) {
                return appGlobalData.appTheme.red;
            } else if (item.status == OrderState::FILLED) {
                return appGlobalData.appTheme.darkGreen;
            }
        }

        return appGlobalData.appTheme.black;
    }

    if (role == Qt::DisplayRole && indexRow < itemsList.size()) {
        const OrderStatusItem& item = itemsList[indexRow];
        switch (indexColumn) {
        case 0: 
            return item.id;
        case 1:
            return item.stratName;
        case 2:
            return item.instrument;
        case 3:
            return (item.action == OrderAction::BUY || item.action == OrderAction::BUY_TO_COVER) ? tr("Buy") : tr("Sell");
        case 4:
            return (item.action == OrderAction::BUY || item.action == OrderAction::SELL_SHORT) ? tr("Open") : 
                ((item.closeEffect == CloseEffect::CLOSE_YESTERDAY) ? tr("CloseYd") :
                 (item.closeEffect == CloseEffect::CLOSE_TODAY) ? tr("CloseToday") : tr("Close"));
        case 5: // status
            switch (item.status) {
            case OrderState::ACCEPTED:
                return tr("Accepted");
            case OrderState::CANCELED:
                return tr("Canceled");
            case OrderState::FILLED:
                return tr("Filled");
            case OrderState::PARTIALLY_FILLED:
                return tr("Part_Filled");
            case OrderState::PENDING_CANCEL:
                return tr("Pending_Cancel");
            case OrderState::PENDING_NEW:
                return tr("Pending_New");
            case OrderState::REJECTED:
                return tr("Rejected");
            case OrderState::SUBMITTED:
                return tr("Submitted");
            default:
            case OrderState::UNKNOWN:
                return tr("Unknown");
            }
            break;
        case 6:
            return item.price;
        case 7:
            if (fabs(item.avgPrice) < 0.000001) {
                return "-";
            } else {
                return item.avgPrice;
            }
        case 8:
            return item.quantity;
        case 9:
            return item.traded;
        case 10:
            return item.datetime;
        case 11:
            return item.statusMsg;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags OrderModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant OrderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();
    
    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft;
    }

    if (role == Qt::SizeHintRole)
    {
        switch (section)
        {
        case 0: return QSize(idWidth, defaultHeightForRow);
        case 1: return QSize(stratWidth, defaultHeightForRow);
        case 2: return QSize(instrumentWidth, defaultHeightForRow);
        case 3: return QSize(sideWidth, defaultHeightForRow);
        case 4: return QSize(actionWidth, defaultHeightForRow);
        case 5: return QSize(statusWidth, defaultHeightForRow);
        case 6:
        case 7: return QSize(priceWidth, defaultHeightForRow);
        case 8:
        case 9:
            return QSize(quantityWidth, defaultHeightForRow);
        case 10:
            return QSize(datetimeWidth, defaultHeightForRow);
        case 11:
            return QSize(statusMsgWidth, defaultHeightForRow);
        }
        return QVariant();
    }

    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int OrderModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.count();
}

void OrderModel::getAllOrderItems(std::vector<OrderStatusItem>& orders)
{
    for (auto& item : itemsList) {
        orders.push_back(item);
    }
}

void OrderModel::getAllPendingOrders(std::vector<OrderStatusItem>& orders)
{
    for (auto& item : itemsList) {
        int status = item.status;
        if (status != OrderState::CANCELED &&
            status != OrderState::FILLED &&
            status != OrderState::REJECTED &&
            status != OrderState::UNKNOWN) {
            orders.push_back(item);
        }
    }
}

bool OrderModel::getPendingOrder(int row, OrderStatusItem& order)
{
    if (row < 0 || row >= itemsList.size()) {
        return false;
    }
    int status = itemsList[row].status;
    if (status != OrderState::CANCELED &&
        status != OrderState::FILLED &&
        status != OrderState::REJECTED &&
        status != OrderState::UNKNOWN) {
        order = itemsList[row];
        return true;
    }

    return false;
}

int OrderModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

void OrderModel::copyOrderItem(OrderStatusItem& item, const AlgoSE::Order& status)
{
    item.srvId = status.srvId;
    strcpy(item.clOrdId, status.clOrdId);
    strcpy(item.ordId, status.ordId);
    sprintf(item.id, "%02d:%s", status.srvId, status.ordId);
    strcpy(item.instrument, status.instrument);
    item.statusMsg = QString().fromLocal8Bit(status.stateMsg);
    item.action = status.action;
    item.price = status.price;
    item.quantity = status.quantity;
    item.traded = status.tradedQty;
    item.avgPrice = status.avgTradedPx;
    item.status = status.status;
    item.closeEffect = status.closeEffect;
    int year, month, day, hour, minute, sec, ms;
    DateTime(status.timestamp).getFields(year, month, day, hour, minute, sec, ms);
    sprintf(item.datetime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, sec);

    if (status.ownerId >= 0) {
        auto it = stratNames.find(status.ownerId);
        if (it != stratNames.end()) {
            strcpy(item.stratName, it->second.c_str());
        }
    }
}

void OrderModel::updateOrderStatus(const AlgoSE::Order& order)
{
    if (order.ordId[0] == '\0' || order.srvId < 0) {
        // We receive an unknown order.
        if (order.stateMsg[0] != '\0') {
            PowerGate* gate = qobject_cast<PowerGate*>(QObject::parent());
            AlgoSE::SystemLog sysLog = { 0 };
            sysLog.level = LogLevel::LOG_ERROR;
            SYSTEMTIME lt;
            GetLocalTime(&lt);
            sysLog.timestamp = DateTime().toTimeStamp();
            strncpy(sysLog.text, order.stateMsg, sizeof(sysLog.text) - 1);

            gate->onSystemLog(sysLog);
        }

        return;
    }

    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (!strcmp(itemsList[i].ordId, order.ordId) &&
            itemsList[i].srvId == order.srvId) {
            break;
        }
    }

    if (i == size) {
        int currentIndex = size;
        beginInsertRows(QModelIndex(), currentIndex, currentIndex);
        OrderStatusItem item = { 0 };
        copyOrderItem(item, order);
        itemsList.push_front(item);
        endInsertRows();
//        emit dataChanged(index(itemsList.size() - 1, 0), index(itemsList.size() - 1, headerLabels.size() - 1));
    } else {
        copyOrderItem(itemsList[i], order);
        emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
    }
}

void OrderModel::updateStratStatus(const AlgoSE::StrategyStatus& state)
{
    if (state.strategyId <= 0) {
        return;
    }

    if (state.name == nullptr || state.name[0] == '\0') {
        return;
    }

    auto it = stratNames.find(state.strategyId);
    if (it == stratNames.end()) {
        stratNames.insert(std::make_pair(state.strategyId, state.name));
    }
}

void OrderModel::updateExecution(const AlgoSE::Execution& exec)
{

}