#include "tradepanel.h"

TradePanel::TradePanel(AlgoSE::Engine* engine, QWidget* parent)
    : QDialog(parent)
{
    pAlogSE = engine;
    pMktDataApi = pAlogSE->getMarketDataAPI();
    pTradingApi = pAlogSE->getTradingAPI();

    ui.setupUi(this);

    QObject::connect(ui.qtySpinBox, SIGNAL(valueChanged(int)), this, SLOT(posQtyValueChanged(int)));
    QObject::connect(ui.pxTypeBtn, SIGNAL(clicked()), this, SLOT(pxTypeBtnClicked()));
    QObject::connect(ui.askBtn, SIGNAL(clicked()), this, SLOT(askBtnClicked()));
    QObject::connect(ui.bidBtn, SIGNAL(clicked()), this, SLOT(bidBtnClicked()));
    QObject::connect(ui.lastBtn, SIGNAL(clicked()), this, SLOT(lastBtnClicked()));
    QObject::connect(ui.upperBtn, SIGNAL(clicked()), this, SLOT(upperBtnClicked()));
    QObject::connect(ui.lowerBtn, SIGNAL(clicked()), this, SLOT(lowerBtnClicked()));
    QObject::connect(ui.buyBtn, SIGNAL(clicked()), this, SLOT(buyBtnClicked()));
    QObject::connect(ui.shortBtn, SIGNAL(clicked()), this, SLOT(shortBtnClicked()));
    QObject::connect(ui.closeBtn, SIGNAL(clicked()), this, SLOT(closeBtnClicked()));

    pxType = PX_TYPE_LIMIT;
    openVol = 1;
    closeVol = 0;
}

TradePanel::~TradePanel()
{
}

void TradePanel::setPosition(const char* instrument, int side, int quantity)
{
    if (instrument == nullptr) {
        return;
    }

    posSide = side;
    closeVol = quantity;

    if (instrument[0] == '\0') {
        this->instrument = "";
        lastPx = 0;
    } else {
        this->instrument = instrument;
        lastPx = pMktDataApi->getLastPrice(instrument);
        ui.symbolLineEdit->setText(instrument);
        ui.askBtn->setText(QString().number(pMktDataApi->getAskPrice(instrument)));
        ui.bidBtn->setText(QString().number(pMktDataApi->getBidPrice(instrument)));
        ui.lastBtn->setText(QString().number(pMktDataApi->getLastPrice(instrument)));
        ui.upperBtn->setText(QString().number(pMktDataApi->getUpperLimitPrice(instrument)));
        ui.lowerBtn->setText(QString().number(pMktDataApi->getLowerLimitPrice(instrument)));
    }

    double tickSize = pMktDataApi->getTickSize(instrument);
    if (tickSize == 0) {
        tickSize = 1.0;
    }

    ui.qtySpinBox->setValue(quantity);
    ui.priceSpinBox->setSingleStep(tickSize);
    ui.priceSpinBox->setValue(lastPx);

    ui.buyBtn->setText(tr("Buy(") + QString().number(quantity) + ")");
    ui.shortBtn->setText(tr("Short(") + QString().number(quantity) + ")");
    if (quantity > 0) {
        if (side == AlgoSE::PositionSide::POSITION_SIDE_LONG) {
            ui.closeBtn->setText(tr("Sell(") + QString().number(quantity) + ")");
        } else if (side == AlgoSE::PositionSide::POSITION_SIDE_SHORT) {
            ui.closeBtn->setText(tr("Cover(") + QString().number(quantity) + ")");
        }
    } else {
        ui.closeBtn->setText(tr("Close(0)"));
    }

    ui.pxTypeBtn->setText(tr("LIMIT"));
    ui.priceSpinBox->setValue(lastPx);
}

void TradePanel::updateMarketData(const AlgoSE::Tick& tick)
{
    if (instrument != tick.instrument) {
        return;
    }

    askPx = tick.askPrice1;
    askVol = tick.askVolume1;
    bidPx = tick.bidPrice1;
    bidVol = tick.bidVolume1;
    upperLimit = tick.upperLimitPrice;
    lowerLimit = tick.lowerLimitPrice;

    ui.askBtn->setText(QString().number(tick.askPrice1));
    ui.bidBtn->setText(QString().number(tick.bidPrice1));
    ui.lastBtn->setText(QString().number(tick.lastPrice));
    ui.upperBtn->setText(QString().number(tick.upperLimitPrice));
    ui.lowerBtn->setText(QString().number(tick.lowerLimitPrice));
}

void TradePanel::updatePxType(int type)
{
    pxType = type;

    if (pxType == PX_TYPE_LIMIT) {
        ui.pxTypeBtn->setText(tr("LIMIT"));
        ui.priceSpinBox->setEnabled(true);
    } else if (pxType == PX_TYPE_MARKET) {
        ui.pxTypeBtn->setText(tr("MARKET"));
        ui.priceSpinBox->cleanText();
        ui.priceSpinBox->setDisabled(true);
    } else if (pxType == PX_TYPE_OPPOSITE) {
        ui.pxTypeBtn->setText(tr("OPPOSITE"));
        ui.priceSpinBox->cleanText();
        ui.priceSpinBox->setDisabled(true);
    }
}

void TradePanel::askBtnClicked()
{
    ui.priceSpinBox->setValue(askPx);
    updatePxType(PX_TYPE_LIMIT);
}

void TradePanel::bidBtnClicked()
{
    ui.priceSpinBox->setValue(bidPx);
    updatePxType(PX_TYPE_LIMIT);
}

void TradePanel::lastBtnClicked()
{
    ui.priceSpinBox->setValue(lastPx);
    updatePxType(PX_TYPE_LIMIT);
}

void TradePanel::upperBtnClicked()
{
    ui.priceSpinBox->setValue(upperLimit);
    updatePxType(PX_TYPE_LIMIT);
}

void TradePanel::lowerBtnClicked()
{
    ui.priceSpinBox->setValue(lowerLimit);
    updatePxType(PX_TYPE_LIMIT);
}

void TradePanel::buyBtnClicked()
{
    double qty = ui.qtySpinBox->value();
    double price = ui.priceSpinBox->value();

    if (pxType == PX_TYPE_LIMIT) {
        pTradingApi->buy(instrument.c_str(), qty, price, nullptr, 0);
    } else if (pxType == PX_TYPE_OPPOSITE) {
        price = pMktDataApi->getAskPrice(instrument.c_str());
        pTradingApi->buy(instrument.c_str(), qty, price, nullptr, 0);
    }
}

void TradePanel::shortBtnClicked()
{
    double qty = ui.qtySpinBox->value();
    double price = ui.priceSpinBox->value();

    if (pxType == PX_TYPE_LIMIT) {
        pTradingApi->sellShort(instrument.c_str(), qty, price, nullptr, 0);
    } else if (pxType == PX_TYPE_OPPOSITE) {
        price = pMktDataApi->getBidPrice(instrument.c_str());
        pTradingApi->sellShort(instrument.c_str(), qty, price, nullptr, 0);
    }
}

void TradePanel::closeBtnClicked()
{
    if (closeVol > 0) {
        double price = ui.priceSpinBox->value();
        double qty = ui.qtySpinBox->value();
        if (qty <= 0 || qty > closeVol) {
            return;
        }

        if (posSide == AlgoSE::PositionSide::POSITION_SIDE_LONG) {
            if (pxType == PX_TYPE_LIMIT) {
                pTradingApi->sell(instrument.c_str(), qty, price, nullptr, 0);
            } else if (pxType == PX_TYPE_OPPOSITE) {
                price = pMktDataApi->getBidPrice(instrument.c_str());
                pTradingApi->sell(instrument.c_str(), qty, price, nullptr, 0);
            }
        } else if (posSide == AlgoSE::PositionSide::POSITION_SIDE_SHORT) {
            if (pxType == PX_TYPE_LIMIT) {
                pTradingApi->buyToCover(instrument.c_str(), qty, price, nullptr, 0);
            } else if (pxType == PX_TYPE_OPPOSITE) {
                price = pMktDataApi->getAskPrice(instrument.c_str());
                pTradingApi->buyToCover(instrument.c_str(), qty, price, nullptr, 0);
            }
        }
    }
}

void TradePanel::posQtyValueChanged(int qty)
{
    ui.buyBtn->setText(tr("Buy(") + QString().number(qty) + ")");
    ui.shortBtn->setText(tr("Short(") + QString().number(qty) + ")");
    if (qty <= closeVol) {
        if (posSide == AlgoSE::PositionSide::POSITION_SIDE_LONG) {
            ui.closeBtn->setText(tr("Sell(") + QString().number(qty) + ")");
        } else if (posSide == AlgoSE::PositionSide::POSITION_SIDE_SHORT) {
            ui.closeBtn->setText(tr("Cover(") + QString().number(qty) + ")");
        }
    }
}

void TradePanel::pxTypeBtnClicked()
{
    pxType++;
    if (pxType > PX_TYPE_OPPOSITE) {
        pxType = PX_TYPE_LIMIT;
    }

    updatePxType(pxType);
}