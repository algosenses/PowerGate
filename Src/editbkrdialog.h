#ifndef EDIT_BROKER_DIALOG_H
#define EDIT_BROKER_DIALOG_H

#include <QDialog>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include "ui_editbkrdialog.h"
#include "logindialog.h"

class BrokerServersDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    BrokerServersDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class BrokerServersModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    BrokerServersModel(std::vector<ServerItem>& srvs, QObject* parent);
    ~BrokerServersModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void insertNewRow();
    void removeRow(int row);
    void reset();
    int  getServerList(std::vector<ServerItem>& out);

private:
    QStringList headerLabels;
    QList<ServerItem> itemsList;
};

class EditBkrDialog : public QDialog
{
    Q_OBJECT

public:
    EditBkrDialog(std::list<Broker>& brokers,
        std::string& bkrName, std::string& bkrID, std::string& srvName,
        QWidget* parent = 0);
    ~EditBkrDialog();
    const std::string& getCurrBkrName() const;
    const std::string& getCurrBkrID() const;
    const std::string& getCurrSrvName() const;

private slots:
    void tradingAddBtnClicked();
    void tradingDeleteBtnClicked();
    void mdAddBtnClicked();
    void mdDeleteBtnClicked();
    void createNewBtnClicked();
    void deleteBtnClicked();
    void okBtnClicked();
    void cancelBtnClicked();

private:
    void showErrorTips(QString& tips);
    bool checkFieldsValue();
    void deleteBrokerConfig();
    void motifyBrokerConfig();
    void addBrokerConfig();

private:
    Ui::EditBkrDlg ui;

    std::list<Broker>& brokers;

    BrokerServersModel* tradingSrvsModel;
    BrokerServersDelegate* tradingSrvsDelegate;

    BrokerServersModel* mdSrvsModel;
    BrokerServersDelegate* mdSrvsDelegate;

    enum {
        MODE_MOTIFY,
        MODE_CREATE_NEW,
    };

    int currEditMode;

    std::string origBkrName;
    std::string origBkrID;
    std::string origSrvName;

    std::string currBkrName;
    std::string currBkrID;
    std::string currSrvName;
};

#endif // EDIT_BROKER_DIALOG_H