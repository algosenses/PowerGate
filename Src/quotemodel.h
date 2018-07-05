#ifndef QUOTE_MODEL_H
#define QUOTE_MODEL_H

#include <string>
#include <QAbstractItemModel>
#include "Engine.h"
#include "circular.h"

using std::string;

using namespace AlgoSE;

typedef struct {
    char      instrument[32];
    char      updateTime[32];
    double    open;
    double    price;
    int       change;
    double    close;
    double    highest;
    double    lowest;
    double    upperLimit;
    double    lowerLimit;
    long long openInt;
    long long volume;
    double    bidPrice1;
    int       bidVolume1;
    double    askPrice1;
    int       askVolume1;
} QuoteItem;

class QuoteModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    QuoteModel(QObject *parent);
    ~QuoteModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
//    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();
    void notifyNewQuote(const AlgoSE::Tick& tick);
    QStringList& getHeaderLabels();

private:
    void copyQuoteItem(QuoteItem& quote, const AlgoSE::Tick& tick, bool newQuote);

private:
    QStringList m_headerLabels;
    QList<QuoteItem> m_quotes;

    int updateTimeWidth;
    int defaultWidth;
};

#define MAX_TRADE_ITEM_SIZE    (20)

class TradesModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    typedef struct {
        char   type;
        double preClosePx;
        double lastPx;
        char   time[16];
        char   price[16];
        int    volume;
    } TradesItem;

    TradesModel(QObject *parent);
    ~TradesModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    //    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();
    void setInstrument(const string& instrument);
    const string& getInstrument() const;
    void notifyNewQuote(const AlgoSE::Tick& tick);

private:
    string instrument;
    QStringList m_headerLabels;

    circular_buffer<TradesItem> m_tradeList;

    int updateTimeWidth;
    int priceWidth;
    int defaultWidth;
};

#endif // QUOTE_MODEL_H