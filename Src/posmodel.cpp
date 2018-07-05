#include "main.h"
#include "posmodel.h"

PositionModel::PositionModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    headerLabels << tr("ServiceID");
    headerLabels << tr("Instrument");
    headerLabels << tr("Side");
    headerLabels << tr("Price");
    headerLabels << tr("Total");
    headerLabels << tr("Yesterday");
    headerLabels << tr("Today");
    headerLabels << tr("Closable");
    headerLabels << tr("MTM PnL");
    headerLabels << "";

    srvIdWidth = 40;
    instrumentWidth = 100;
    priceWidth = 80;
    quantityWidth = 80;
    actionWidth = 60;
    sideWidth = 30;
    pnlWidth = 120;
}

PositionModel::~PositionModel()
{
}

const QStringList& PositionModel::getHeaderLabels() const
{
    return headerLabels;
}

QModelIndex PositionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex PositionModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant PositionModel::data(const QModelIndex &index, int role) const
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
        if (indexRow < itemsList.size()) {
            if (indexColumn == 2) {
                if (itemsList[indexRow].side == PositionSide::POSITION_SIDE_LONG) {
                    return appGlobalData.appTheme.red;
                } else if (itemsList[indexRow].side == PositionSide::POSITION_SIDE_SHORT) {
                    return appGlobalData.appTheme.darkGreen;
                }
            } else if (indexColumn == 8) {
                if (itemsList[indexRow].posPnL >= 0) {
                    return appGlobalData.appTheme.red;
                } else {
                    return appGlobalData.appTheme.darkGreen;
                }
            }
        }

        return appGlobalData.appTheme.black;
    }

    if (role == Qt::DisplayRole && indexRow < itemsList.size()) {
        const PositionItem& item = itemsList[indexRow];
        switch (indexColumn) {
        case 0:
            return item.srvId;
        case 1:
            return item.instrument;
        case 2:
            return item.side == PositionSide::POSITION_SIDE_LONG ? tr("Buy") : tr("Sell");
        case 3:
            return item.price;
        case 4:
            return item.total;
        case 5:
            return item.yesterday;
        case 6:
            return item.today;
        case 7:
            return item.closable;
        case 8:
            return item.posPnL;
        }
    }

    return QVariant();
}

Qt::ItemFlags PositionModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant PositionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)return QVariant();

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft;
    }

    if (role == Qt::SizeHintRole)
    {
        switch (section)
        {
        case 0: return QSize(srvIdWidth, defaultHeightForRow);
        case 1: return QSize(instrumentWidth, defaultHeightForRow);
        case 2: return QSize(sideWidth, defaultHeightForRow);
        case 3: return QSize(priceWidth, defaultHeightForRow);
        case 4:
        case 5:
        case 6:
        case 7: return QSize(quantityWidth, defaultHeightForRow);
        case 8: return QSize(pnlWidth, defaultHeightForRow);
        default:
            break;
        }
        return QVariant();
    }

    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int PositionModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.count();
}

int PositionModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

bool PositionModel::getPosItem(const char* instrument, PositionItem& item)
{
    for (auto& elem : itemsList) {
        if (!_stricmp(elem.instrument, instrument)) {
            item = elem;
            return true;
        }
    }

    return false;
}

bool PositionModel::getPosItem(int row, PositionItem& item)
{
    if (row < 0 || row >= itemsList.size()) {
        return false;
    }

    item = itemsList[row];

    return true;
}

void PositionModel::getAllPosItems(std::vector<PositionItem>& items)
{
    for (auto& item : itemsList) {
        items.push_back(item);
    }
}

void PositionModel::copyPosItem(PositionItem& item, const AlgoSE::AccountPosition& pos)
{
    item.srvId = pos.srvId;
    strcpy(item.instrument, pos.instrument);
    item.side = pos.side;
    item.total = pos.cumQty;
    item.today = pos.todayQty;
    item.yesterday = pos.histQty;
    item.closable = pos.cumQty - pos.frozen;
    item.price = pos.avgPx;
    item.realPnL = pos.realizedPnL;
    item.posPnL = pos.unrealizedPnL;
}

void PositionModel::updatePosition(const AlgoSE::AccountPosition& pos)
{
    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].srvId == pos.srvId &&
            !strcmp(pos.instrument, itemsList[i].instrument) && 
            itemsList[i].side == pos.side) {
            break;
        }
    }

    if (i == size) {
        if (pos.cumQty > 0) {
            int currentIndex = size;
            int offset = 0;
            for (auto& elem : itemsList) {
                int cmp = strcmp(pos.instrument, elem.instrument);
                if (cmp < 0) {
                    offset++;
                    continue;
                } else {
                    if (cmp == 0) {
                        if (pos.side > elem.side) {
                            offset++;
                        }
                    }
                    break;
                }
            }

            if (offset > itemsList.size()) {
                assert(false);
            }

            beginInsertRows(QModelIndex(), currentIndex, currentIndex);
            PositionItem item = { 0 };
            copyPosItem(item, pos);
//            itemsList.push_back(item);
            itemsList.insert(offset, item);
            endInsertRows();
        }
    } else {
        copyPosItem(itemsList[i], pos);
        if (itemsList[i].total > 0) {
            emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
        } else {
            beginRemoveRows(QModelIndex(), i, i);
            itemsList.removeAt(i);
            endRemoveRows();
//            emit dataChanged(index(0, 0), index(itemsList.size(), headerLabels.size() - 1));
        }
    }
}