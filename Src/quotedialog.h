#ifndef QUOTE_DIALOG_H
#define QUOTE_DIALOG_H

#include <string>
#include <QDialog>
#include "quotemodel.h"
#include "ui_quotedialog.h"

using std::string;

class QuoteDialog : public QDialog
{
    Q_OBJECT

public:
    QuoteDialog(const string& instrument, QWidget* parent = 0);
    ~QuoteDialog();
    const string& getInstrument() const;
    void notifyNewQuote(const Tick& tick);
    void closeEvent(QCloseEvent *event);
    void reject();

signals:
    void closeEvt(const string&);

private:
    void printPx(char* text, double price);

private:
    Ui::QuoteDialog ui;

    string instrument;
    double lastAsk1Px;
    double lastBid1Px;
    TradesModel* tradesModel;
};

#endif // QUOTE_DIALOG_H