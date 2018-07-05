#include "main.h"
#include "stratmodel.h"

const int StrategyAlertColumn = 11;

StratModel::StratModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    headerLabels << tr("ID");
    headerLabels << tr("Name");
    headerLabels << tr("Params");
    headerLabels << tr("Start");
    headerLabels << tr("Pause");
    headerLabels << tr("Submit");
    headerLabels << tr("Cancel");
    headerLabels << tr("Long");
    headerLabels << tr("Short");
    headerLabels << tr("Verbose");
    headerLabels << tr("Command");
    headerLabels << tr("Message");

    idWidth = 50;
    nameWidth = 100;
    paramWidth = 50;
    ctrlWidth = 60;
    pauseWidth = 60;
    submitWidth = 70;
    cancelWidth = 70;
    longWidth = 50;
    shortWidth = 50;
    showWidth = 70;
}

StratModel::~StratModel()
{
}

const QStringList& StratModel::getHeaderLabels() const
{
    return headerLabels;
}

QModelIndex StratModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex StratModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant StratModel::data(const QModelIndex &index, int role) const
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

    if (role == Qt::ForegroundRole) {
        // alert column
        if (indexColumn == StrategyAlertColumn) {
            if (itemsList[indexRow].redAlert) {
                return appGlobalData.appTheme.red;
            } else {
                return appGlobalData.appTheme.gray;
            }
        }

        return appGlobalData.appTheme.black;
    }

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

    if (role == Qt::ToolTipRole) {
        if (indexColumn == 1) {
            return QString(itemsList.at(indexRow).name);
        } else if (indexColumn == 11) {
            return QString().fromLocal8Bit(itemsList.at(indexRow).log);
        }
    }

    if (role == Qt::DisplayRole && indexRow < itemsList.size()) {
        const StratItem& item = itemsList[indexRow];
        switch (indexColumn) {
        case 0:
            return item.strategyId;
        case 1:
            return item.name;
        case 2:
            return headerLabels.at(2);
        case 3:
            if (item.status == Strategy::STOPPED) {
                return tr("Start");
            } else {
                return tr("Stop");
            }
        case 4:
            if (item.status != Strategy::PAUSED) {
                return tr("Pause");
            } else {
                return tr("Resume");
            }
        case 5:
            return item.submittion;
        case 6:
            return item.cancellation;
        case 7: {
            int size = 0;
            for (auto& pos : item.posLongList) {
                size += pos.size;
            }
            return size;
        }
        case 8:	{
            int size = 0;
            for (auto& pos : item.posShortList) {
                size += pos.size;
            }
            return size;
        }
        case 9:
            if (item.verbose) {
                return tr("Hide");
            } else {
                return tr("Show");
            }
        case 10:
            return headerLabels.at(10);
        case 11:
            return QString().fromLocal8Bit(item.log);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags StratModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant StratModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();
    
    if (role == Qt::TextAlignmentRole) {
        // message
        if (section != 11) {
            return Qt::AlignLeft;
        }
    }

    if (role == Qt::SizeHintRole)
    {
        switch (section)
        {
        case 0: return QSize(idWidth, defaultHeightForRow);
        case 1: return QSize(nameWidth, defaultHeightForRow);
        case 2: return QSize(paramWidth, defaultHeightForRow);
        case 3: return QSize(ctrlWidth, defaultHeightForRow);
        case 4: return QSize(pauseWidth, defaultHeightForRow);
        case 5: return QSize(submitWidth, defaultHeightForRow);
        case 6: return QSize(cancelWidth, defaultHeightForRow);
        case 7: return QSize(longWidth, defaultHeightForRow);
        case 8: return QSize(shortWidth, defaultHeightForRow);
        case 9: return QSize(showWidth, defaultHeightForRow);
        }
        return QVariant();
    }

    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int StratModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.count();
}

int StratModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

void StratModel::getAllStratItems(std::vector<StratItem>& strategies)
{
    for (auto& item : itemsList) {
        strategies.push_back(item);
    }
}

void StratModel::copyStratItem(StratItem& item, const AlgoSE::StrategyStatus& state)
{
    item.strategyId = state.strategyId;
    strcpy(item.name, state.name);
    item.status = state.state;
    item.verbose = state.verbose;
    item.submittion = state.submittion;
    item.cancellation = state.cancellation;
}

void StratModel::updateStratStatus(const AlgoSE::StrategyStatus& state)
{
    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].strategyId == state.strategyId) {
            break;
        }
    }

    if (i == size && state.state != Strategy::UNLOADED) {
        int currentIndex = size;
        beginInsertRows(QModelIndex(), currentIndex, currentIndex);
        StratItem item = { 0 };
        copyStratItem(item, state);
        itemsList.push_back(item);
        endInsertRows();
        emit dataChanged(index(itemsList.size() - 1, 0), index(itemsList.size() - 1, headerLabels.size() - 1));
    } else {
        if (state.state != Strategy::UNLOADED) {
            copyStratItem(itemsList[i], state);
            emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
        } else {
            beginRemoveRows(QModelIndex(), i, i);
            itemsList.removeAt(i);
            endRemoveRows();
        }
    }
}

void StratModel::updateStratPositions(const AlgoSE::StrategyPosition& pos)
{
    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].strategyId == pos.strategyId) {
            break;
        }
    }

    if (i == size) {
        return;
    }

    if (pos.side == PositionSide::POSITION_SIDE_LONG) {
        PosList& posList = itemsList[i].posLongList;
        size_t j = 0;
        for (; j < posList.size(); j++) {
            PosItem& item = posList[j];
            if (!stricmp(pos.instrument, item.instrument)) {
                item.size = pos.quantity;
                break;
            }
        }
        if (j == posList.size()) {
            PosItem item;
            strcpy(item.instrument, pos.instrument);
            item.size = pos.quantity;
            posList.push_back(item);
        }
    } else if (pos.side == PositionSide::POSITION_SIDE_SHORT) {
        PosList& posList = itemsList[i].posShortList;
        size_t j = 0;
        for (; j < posList.size(); j++) {
            PosItem& item = posList[j];
            if (!stricmp(pos.instrument, item.instrument)) {
                item.size = pos.quantity;
                break;
            }
        }
        if (j == posList.size()) {
            PosItem item;
            strcpy(item.instrument, pos.instrument);
            item.size = pos.quantity;
            posList.push_back(item);
        }
    }

    emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
}

void StratModel::updateStratLog(const AlgoSE::StrategyLog& log)
{
    if (log.text == nullptr || log.text[0] == '\0') {
        return;
    }

    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].strategyId == log.strategyId) {
            break;
        }
    }

    if (i == size) {
        return;
    }

    strncpy(itemsList[i].log, log.text, sizeof(itemsList[i].log));
    itemsList[i].redAlert = true;

    emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
}

void StratModel::clearRedAlert(int stratId)
{
    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].strategyId == stratId) {
            break;
        }
    }

    if (i == size) {
        return;
    }

    itemsList[i].redAlert = false;

    emit dataChanged(index(i, StrategyAlertColumn), index(i, StrategyAlertColumn));
}