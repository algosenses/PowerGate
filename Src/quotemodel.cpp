#include <limits>
#include "main.h"
#include "quotemodel.h"

QuoteModel::QuoteModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_quotes.clear();

    m_headerLabels << tr("Instrument");
    m_headerLabels << tr("Open");
    m_headerLabels << tr("Last");
    m_headerLabels << tr("High");
    m_headerLabels << tr("Low");
    m_headerLabels << tr("Volume");
    m_headerLabels << tr("OpenInt");    
    m_headerLabels << tr("BidPrice");
    m_headerLabels << tr("BidVolume");
    m_headerLabels << tr("AskPrice");
    m_headerLabels << tr("AskVolume");
    m_headerLabels << tr("UpdateTime");
    m_headerLabels << "";

    updateTimeWidth = 200;
}

QuoteModel::~QuoteModel()
{
}

QModelIndex QuoteModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex QuoteModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant QuoteModel::data(const QModelIndex &index, int role) const
{
    int currentRow = m_quotes.count() - index.row() - 1;
    if (currentRow < 0 || currentRow >= m_quotes.count())
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
        if (indexColumn == 0) {
            return Qt::AlignLeft | Qt::AlignVCenter;
        }
        return Qt::AlignRight | Qt::AlignVCenter;
    }

    if (role == Qt::BackgroundRole) {
        switch (indexColumn)
        {
        case 2:
            return appGlobalData.appTheme.lightRedBlue;
        case 7:
        case 8:
            return appGlobalData.appTheme.lightRed;
        case 9:
        case 10:
            return appGlobalData.appTheme.lightGreen;
        default:
            return QVariant();
        }
    }

    if (role == Qt::ForegroundRole) {
        if (indexColumn == 2 && indexRow < m_quotes.size()) {
            if (m_quotes[indexRow].change == 1) {
                return appGlobalData.appTheme.red;
            } else if (m_quotes[indexRow].change == -1) {
                return appGlobalData.appTheme.green;
            }
        }

        return appGlobalData.appTheme.black;
    }

    if (role == Qt::DisplayRole && indexRow < m_quotes.size()) {
        const QuoteItem& data = m_quotes[indexRow];
        switch (indexColumn) {
        case 0:
            return data.instrument;
        case 1:
            if (data.open == (std::numeric_limits<double>::max)()) {
                return "--";
            }
            return data.open;
        case 2:
            if (data.price == (std::numeric_limits<double>::max)()) {
                return "--";
            }
            return data.price;
        case 3:
            if (data.highest == (std::numeric_limits<double>::max)()) {
                return "--";
            }
            return data.highest;
        case 4:
            if (data.lowest == (std::numeric_limits<double>::max)()) {
                return "--";
            }
            return data.lowest;
        case 5:
            return data.volume;
        case 6:
            return data.openInt;
        case 7:
            if (isnan(data.bidPrice1) || data.bidPrice1 == (std::numeric_limits<double>::max)()) {
                return "--";
            } else {
                return data.bidPrice1;
            }
        case 8:
            return data.bidVolume1;
        case 9:
            if (isnan(data.askPrice1) || data.askPrice1 == (std::numeric_limits<double>::max)()) {
                return "--";
            } else {
                return data.askPrice1;
            }
        case 10:
            return data.askVolume1;
        case 11:
            return data.updateTime;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

/*
Qt::ItemFlags QuoteModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
*/

QVariant QuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();
    if (role == Qt::TextAlignmentRole)
    {
        if (section == 0) {
            return Qt::AlignLeft;
        }
        return Qt::AlignRight;
    }

    if (role == Qt::SizeHintRole)
    {
        switch (section)
        {
        case 11: return QSize(updateTimeWidth, defaultHeightForRow);
        }
        return QVariant();
    }

    if (role!=Qt::DisplayRole) return QVariant();

    if (section >= m_headerLabels.count()) {
        return QVariant();
    }

    return m_headerLabels.at(section);
}

int QuoteModel::rowCount(const QModelIndex &parent) const
{
    return m_quotes.size();
}

int QuoteModel::columnCount(const QModelIndex &parent) const
{
    return m_headerLabels.count();
}

void QuoteModel::clear()
{
}

void QuoteModel::copyQuoteItem(QuoteItem& quote, const Tick& tick, bool newQuote)
{
    strcpy(quote.instrument, tick.instrument);
    quote.open = tick.openPrice;
    if (newQuote) {
        quote.change = 0;
    } else {
        if (fabs(tick.lastPrice - quote.price) < 0.000001) {
            quote.change = 0;
        } else if (tick.lastPrice > quote.price) {
            quote.change = 1;
        } else {
            quote.change = -1;
        }
    }
    quote.price = tick.lastPrice;
    quote.close = tick.closePrice;
    quote.highest = tick.highestPrice;
    quote.lowest = tick.lowestPrice;
    quote.upperLimit = tick.upperLimitPrice;
    quote.lowerLimit = tick.lowerLimitPrice;
    quote.openInt = tick.openInterest;
    quote.volume = tick.volume;
    quote.bidPrice1 = tick.bidPrice1;
    quote.bidVolume1 = tick.bidVolume1;
    quote.askPrice1 = tick.askPrice1;
    quote.askVolume1 = tick.askVolume1;

    int year, month, day, hour, minute, sec, ms;
    DateTime(tick.timestamp).getFields(year, month, day, hour, minute, sec, ms);
    sprintf(quote.updateTime, "%04d-%02d-%02d %02d:%02d:%02d.%03d", year, month, day, hour, minute, sec, ms);
}

void QuoteModel::notifyNewQuote(const Tick& tick)
{
    int i = 0;
    int size = m_quotes.size();
    for (i = 0; i < size; i++) {
        if (!strcmp(m_quotes[i].instrument, tick.instrument)) {
            break;
        }
    }

    if (i == size) {
        int currentIndex = size;
        beginInsertRows(QModelIndex(), currentIndex, currentIndex);
        QuoteItem item;
        copyQuoteItem(item, tick, true);
        m_quotes.push_back(item);
        endInsertRows();
        emit dataChanged(index(m_quotes.size()-1,0),index(m_quotes.size()-1,m_headerLabels.size()-1));
    } else {
        copyQuoteItem(m_quotes[i], tick, false);
        emit dataChanged(index(i,0),index(i,m_headerLabels.size()-1));
    }
}

QStringList& QuoteModel::getHeaderLabels()
{
    return m_headerLabels;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TradesModel::TradesModel(QObject *parent)
    : m_tradeList(MAX_TRADE_ITEM_SIZE)
    , QAbstractItemModel(parent)
{
    m_headerLabels << tr("Time");
    m_headerLabels << tr("Price");
    m_headerLabels << tr("Volume");

    updateTimeWidth = 80;
    priceWidth = 80;
}

TradesModel::~TradesModel()
{
}

QModelIndex TradesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex TradesModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant TradesModel::data(const QModelIndex &index, int role) const
{
    int currentRow = m_tradeList.size() - index.row() - 1;
    if (currentRow < 0 || currentRow >= m_tradeList.size())
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
        if (indexColumn == 0) {
            return Qt::AlignLeft | Qt::AlignVCenter;
        }
        return Qt::AlignRight | Qt::AlignVCenter;
    }

    if (role == Qt::ForegroundRole) {
        const TradesItem& data = m_tradeList[indexRow];
        switch (indexColumn)
        {
        case 1:
            if (data.lastPx > data.preClosePx) {
                return appGlobalData.appTheme.red;
            } else if (data.lastPx < data.preClosePx) {
                return appGlobalData.appTheme.green;
            }
        default:
            return QVariant();
        }
    }

    if (role == Qt::DisplayRole && indexRow < m_tradeList.size()) {
        const TradesItem& data = m_tradeList[indexRow];
        switch (indexColumn) {
        case 0:
            return data.time;
        case 1:
            return data.price;
        case 2:
            if (data.type == SecurityType::STOCK) {
                return data.volume / 100;
            } else {
                return data.volume;
            }
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant TradesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();
    if (role == Qt::TextAlignmentRole)
    {
        if (section == 0) {
            return Qt::AlignLeft;
        }
        return Qt::AlignRight;
    }

    if (role == Qt::SizeHintRole)
    {
        switch (section)
        {
        case 0: return QSize(updateTimeWidth, defaultHeightForRow);
        case 1: return QSize(priceWidth, defaultHeightForRow);
        }
        return QVariant();
    }

    if (role!=Qt::DisplayRole) return QVariant();

    if (section >= m_headerLabels.count()) {
        return QVariant();
    }

    return m_headerLabels.at(section);
}

int TradesModel::rowCount(const QModelIndex &parent) const
{
    return m_tradeList.size();
}

int TradesModel::columnCount(const QModelIndex &parent) const
{
    return m_headerLabels.count();
}

void TradesModel::clear()
{
    m_tradeList.clear();
}

void TradesModel::setInstrument(const string& instrument)
{
    this->instrument = instrument;
}

const string& TradesModel::getInstrument() const
{
    return instrument;
}

void TradesModel::notifyNewQuote(const Tick& tick)
{
    bool full = false;
    if (m_tradeList.size() == m_tradeList.capacity()) {
        full = true;
    }

    TradesItem item = { 0 };
    item.type = tick.secType;
    item.preClosePx = tick.preClosePrice;
    item.lastPx = tick.lastPrice;
    int year, month, day, hour, minute, sec, ms;
    DateTime(tick.timestamp).getFields(year, month, day, hour, minute, sec, ms);
    sprintf(item.time, "%02d:%02d:%02d", hour, minute, sec);
    sprintf(item.price, "%.02f", tick.lastPrice);
    item.volume = tick.volume;

    if (!full) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_tradeList.push_back(item);
        endInsertRows();
    } else {
        m_tradeList.push_back(item);
        dataChanged(index(0,0),index(m_tradeList.size()-1,m_headerLabels.size()-1));
    }
}