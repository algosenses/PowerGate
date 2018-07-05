#ifndef SUBSCRIBE_DIALOG_H
#define SUBSCRIBE_DIALOG_H

#include <QDialog>
#include "ui_subscribedialog.h"

class SubscribeDialog : public QDialog
{
    Q_OBJECT

public:
    SubscribeDialog(char initKey, QWidget* parent = 0);
    SubscribeDialog(QWidget* parent = 0);
    ~SubscribeDialog();
    QString getInstrument() const;
    void setInstrument(const QString& text);

private:
    Ui::SubscribeDlg ui;

private slots:
    void buttonOkClicked();
    void buttonCancelClicked();
};

#endif // SUBSCRIBE_DIALOG_H