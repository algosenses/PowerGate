#ifndef TRADE_PANEL_H
#define TRADE_PANEL_H

#include <vector>
#include <QDialog>
#include "Defines.h"
#include "Engine.h"
#include "ui_tradepanel.h"

class TradePanel : public QDialog
{
    Q_OBJECT

public:
    TradePanel(AlgoSE::Engine* engine, QWidget* parent = 0);
    ~TradePanel();
    void setPosition(const char* instrument, int side, int quantity);
    void updateMarketData(const AlgoSE::Tick& tick);

private slots:
    void posQtyValueChanged(int);
    void pxTypeBtnClicked();
    void askBtnClicked();
    void bidBtnClicked();
    void lastBtnClicked();
    void upperBtnClicked();
    void lowerBtnClicked();
    void buyBtnClicked();
    void shortBtnClicked();
    void closeBtnClicked();

private:
    void updatePxType(int type);

private:
    Ui::TradePanel ui;

    AlgoSE::Engine* pAlogSE;
    AlgoSE::IEngine::MarketDataAPI* pMktDataApi;
    AlgoSE::IEngine::TradingAPI* pTradingApi;

    enum {
        PX_TYPE_LIMIT = 0,
        PX_TYPE_MARKET,
        PX_TYPE_OPPOSITE,
    };

    std::string instrument;
    int pxType;
    int posSide;
    int openVol;
    int closeVol;
    double priceUnit;
    double lastPx;
    double askPx;
    unsigned long askVol;
    double bidPx;
    unsigned long bidVol;
    double upperLimit;
    double lowerLimit;
};

#endif // TRADE_PANEL_H