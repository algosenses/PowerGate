#include <limits>
#include <QCloseEvent>
#include "quotedialog.h"

QuoteDialog::QuoteDialog(const string& instrument, QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    this->instrument = instrument;
    setWindowTitle(instrument.c_str());
    ui.instrumentLine->setText(instrument.c_str());
    lastAsk1Px = 0.0;
    lastBid1Px = 0.0;

    tradesModel = new TradesModel(this);

    ui.tradesList->setModel(tradesModel);
    ui.tradesList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.tradesList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui.tradesList->horizontalHeader()->setStretchLastSection(true);
}

const string& QuoteDialog::getInstrument() const
{
    return instrument;
}

void QuoteDialog::printPx(char* text, double price)
{
    if (price == (std::numeric_limits<double>::max)() ||
        price == -(std::numeric_limits<double>::max)()) {
        strcpy(text, "--");
        return;
    }

    sprintf(text, "%.02f", price);
}

void QuoteDialog::notifyNewQuote(const Tick& tick)
{
    char text[64];

    if (tick.type == AlgoSE::TickType::SNAPSHOT) {
        int askPxChange = 0;
        int bidPxChange = 0;
        string askStyleSheet;
        string bidStyleSheet;
        if (lastAsk1Px == 0) {
            if (tick.askPrice1 < tick.preClosePrice) {
                askPxChange = -1;
            } else if (tick.askPrice1 > tick.preClosePrice) {
                askPxChange = 1;
            }
        } else {
            if (tick.askPrice1 < lastAsk1Px) {
                askPxChange = -1;
            } else if (tick.askPrice1 > lastAsk1Px) {
                askPxChange = 1;
            }
        }

        lastAsk1Px = tick.askPrice1;
        if (askPxChange == 1) {
            askStyleSheet = "color:red;font:bold";
        } else if (askPxChange == -1) {
            askStyleSheet = "color:green;font:bold";
        }

        if (lastBid1Px == 0) {
            if (tick.bidPrice1 < tick.preClosePrice) {
                bidPxChange = -1;
            } else if (tick.bidPrice1 > tick.preClosePrice) {
                bidPxChange = 1;
            }
        } else {
            if (tick.bidPrice1 < lastBid1Px) {
                bidPxChange = -1;
            } else if (tick.bidPrice1 > lastBid1Px) {
                bidPxChange = 1;
            }
        }

        lastBid1Px = tick.bidPrice1;
        if (bidPxChange == 1) {
            bidStyleSheet = "color:red;font:bold";
        } else if (bidPxChange == -1) {
            bidStyleSheet = "color:green;font:bold";
        }

        if (tick.depth == 1) {
            printPx(text, tick.askPrice5); ui.ask10Px->setText(text); if (askPxChange) { ui.ask10Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice4); ui.ask9Px->setText(text); if (askPxChange) { ui.ask9Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice3); ui.ask8Px->setText(text); if (askPxChange) { ui.ask8Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice2); ui.ask7Px->setText(text); if (askPxChange) { ui.ask7Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice1); ui.ask6Px->setText(text); if (askPxChange) { ui.ask6Px->setStyleSheet(askStyleSheet.c_str()); }
        } else if (tick.depth == 0) {
            printPx(text, tick.askPrice5); ui.ask5Px->setText(text); if (askPxChange) { ui.ask5Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice4); ui.ask4Px->setText(text); if (askPxChange) { ui.ask4Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice3); ui.ask3Px->setText(text); if (askPxChange) { ui.ask3Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice2); ui.ask2Px->setText(text); if (askPxChange) { ui.ask2Px->setStyleSheet(askStyleSheet.c_str()); }
            printPx(text, tick.askPrice1); ui.ask1Px->setText(text); if (askPxChange) { ui.ask1Px->setStyleSheet(askStyleSheet.c_str()); }
        }

        if (tick.depth == 0) {
            printPx(text, tick.bidPrice1); ui.bid1Px->setText(text); if (bidPxChange) { ui.bid1Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice2); ui.bid2Px->setText(text); if (bidPxChange) { ui.bid2Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice3); ui.bid3Px->setText(text); if (bidPxChange) { ui.bid3Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice4); ui.bid4Px->setText(text); if (bidPxChange) { ui.bid4Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice5); ui.bid5Px->setText(text); if (bidPxChange) { ui.bid5Px->setStyleSheet(bidStyleSheet.c_str()); }
        } else if (tick.depth == 1) {
            printPx(text, tick.bidPrice1); ui.bid6Px->setText(text); if (bidPxChange) { ui.bid6Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice2); ui.bid7Px->setText(text); if (bidPxChange) { ui.bid7Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice3); ui.bid8Px->setText(text); if (bidPxChange) { ui.bid8Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice4); ui.bid9Px->setText(text); if (bidPxChange) { ui.bid9Px->setStyleSheet(bidStyleSheet.c_str()); }
            printPx(text, tick.bidPrice5); ui.bid10Px->setText(text); if (bidPxChange) { ui.bid10Px->setStyleSheet(bidStyleSheet.c_str()); }
        }

        Tick temp = tick;
        if (tick.secType == SecurityType::STOCK) {
            if (tick.depth == 0) {
                temp.askVolume1 /= 100;
                temp.askVolume2 /= 100;
                temp.askVolume3 /= 100;
                temp.askVolume4 /= 100;
                temp.askVolume5 /= 100;
            } else if (tick.depth == 1) {
                temp.askVolume1 /= 100;
                temp.askVolume2 /= 100;
                temp.askVolume3 /= 100;
                temp.askVolume4 /= 100;
                temp.askVolume5 /= 100;
            }

            if (tick.depth == 0) {
                temp.bidVolume1 /= 100;
                temp.bidVolume2 /= 100;
                temp.bidVolume3 /= 100;
                temp.bidVolume4 /= 100;
                temp.bidVolume5 /= 100;
            } else if (tick.depth == 1) {
                temp.bidVolume1 /= 100;
                temp.bidVolume2 /= 100;
                temp.bidVolume3 /= 100;
                temp.bidVolume4 /= 100;
                temp.bidVolume5 /= 100;
            }

            temp.volume /= 100;
        }

        if (tick.depth == 0) {
            sprintf(text, "%d", temp.askVolume1); ui.ask1Vol->setText(text);
            sprintf(text, "%d", temp.askVolume2); ui.ask2Vol->setText(text);
            sprintf(text, "%d", temp.askVolume3); ui.ask3Vol->setText(text);
            sprintf(text, "%d", temp.askVolume4); ui.ask4Vol->setText(text);
            sprintf(text, "%d", temp.askVolume5); ui.ask5Vol->setText(text);
        } else if (tick.depth == 1) {
            sprintf(text, "%d", temp.askVolume1); ui.ask6Vol->setText(text);
            sprintf(text, "%d", temp.askVolume2); ui.ask7Vol->setText(text);
            sprintf(text, "%d", temp.askVolume3); ui.ask8Vol->setText(text);
            sprintf(text, "%d", temp.askVolume4); ui.ask9Vol->setText(text);
            sprintf(text, "%d", temp.askVolume5); ui.ask10Vol->setText(text);
        }

        if (tick.depth == 0) {
            sprintf(text, "%d", temp.bidVolume1); ui.bid1Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume2); ui.bid2Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume3); ui.bid3Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume4); ui.bid4Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume5); ui.bid5Vol->setText(text);
        } else if (tick.depth == 1) {
            sprintf(text, "%d", temp.bidVolume1); ui.bid6Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume2); ui.bid7Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume3); ui.bid8Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume4); ui.bid9Vol->setText(text);
            sprintf(text, "%d", temp.bidVolume5); ui.bid10Vol->setText(text);
        }

        if (tick.preClosePrice > 0) {
            if (tick.lastPrice > tick.preClosePrice) {
                ui.last->setStyleSheet("color:red;font:bold");
            } else if (fabs(tick.lastPrice - tick.preClosePrice) < 0.000001) {
                ui.last->setStyleSheet("color:white;font:bold");
            } else if (tick.lastPrice < tick.preClosePrice) {
                ui.last->setStyleSheet("color:green;font:bold");
            }
        }

        printPx(text, tick.lastPrice); ui.last->setText(text);
        
        if (tick.preClosePrice > 0) {
            double change = tick.lastPrice - tick.preClosePrice;
            printPx(text, change); ui.change->setText(text);
            if (change >= 0) {
                ui.change->setStyleSheet("color:red;font:bold");
            } else {
                ui.change->setStyleSheet("color:green;font:bold");
            }
        } else {
            strcpy(text, "--"); ui.change->setText(text);
        }

        if (tick.preClosePrice == (std::numeric_limits<double>::max)() ||
            tick.preClosePrice == -(std::numeric_limits<double>::max)()) {
            strcpy(text, "--");
        } else if (tick.preClosePrice > 0) {
            sprintf(text, "%.02f%%", (tick.lastPrice - tick.preClosePrice) / tick.preClosePrice * 100); ui.rise->setText(text);
            if (tick.lastPrice > tick.preClosePrice) {
                ui.rise->setStyleSheet("color:red;font:bold");
            } else if (tick.lastPrice < tick.preClosePrice) {
                ui.rise->setStyleSheet("color:green;font:bold");
            }
        }

        printPx(text, tick.openPrice); ui.open->setText(text);
        if (tick.openPrice > tick.preClosePrice) {
            ui.open->setStyleSheet("color:red;font:bold");
        } else if (tick.openPrice < tick.preClosePrice) {
            ui.open->setStyleSheet("color:green;font:bold");
        }

        printPx(text, tick.highestPrice); ui.highest->setText(text);
        if (tick.highestPrice > tick.preClosePrice) {
            ui.highest->setStyleSheet("color:red;font:bold");
        } else if (tick.highestPrice < tick.preClosePrice) {
            ui.highest->setStyleSheet("color:green;font:bold");
        }

        printPx(text, tick.lowestPrice); ui.lowest->setText(text);
        if (tick.lowestPrice > tick.preClosePrice) {
            ui.lowest->setStyleSheet("color:red;font:bold");
        } else if (tick.lowestPrice < tick.preClosePrice) {
            ui.lowest->setStyleSheet("color:green;font:bold");
        }

        printPx(text, tick.upperLimitPrice); ui.upperLimit->setText(text);
        printPx(text, tick.lowerLimitPrice); ui.lowerLimit->setText(text);
        sprintf(text, "%ld", temp.volume); ui.volume->setText(text);
        sprintf(text, "%ld", (int)(tick.turnover / 10000)); ui.turnover->setText(text);

    } else if (tick.type == AlgoSE::TickType::TRADES) {
        tradesModel->notifyNewQuote(tick);
    }
}

void QuoteDialog::reject()
{
    close();
}

void QuoteDialog::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit closeEvt(instrument);
}

QuoteDialog::~QuoteDialog()
{
}