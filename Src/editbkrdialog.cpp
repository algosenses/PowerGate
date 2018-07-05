#include <QMessageBox>
#include "main.h"
#include "editbkrdialog.h"

BrokerServersDelegate::BrokerServersDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *BrokerServersDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    editor->setFrame(false);
    int indexColumn = index.column();
    
    const char* ValidIpAddressRegex = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
    const char* ValidHostnameRegex = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$";
    char Regex[512];
    sprintf(Regex, "((^|, )(%s|%s))+$", ValidIpAddressRegex, ValidHostnameRegex);
    if (indexColumn == 0) {
        QRegExp regx("^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$");
        QRegExpValidator* validator = new QRegExpValidator(regx, parent);
        editor->setValidator(validator);
    } else if (indexColumn == 1) {
        QIntValidator* validator = new QIntValidator(0, 0xffff, parent);
        editor->setValidator(validator);
    }

    return editor;
}

void BrokerServersDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void BrokerServersDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    QString text = lineEdit->text();

    model->setData(index, text, Qt::EditRole);
}

void BrokerServersDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

////////////////////////////////////////////////////////////////////////////////
BrokerServersModel::BrokerServersModel(std::vector<ServerItem>& srvs, QObject* parent)
    : QAbstractItemModel(parent)
{
    for (size_t i = 0; i < srvs.size(); i++) {
        itemsList.append(srvs[i]);
    }

    headerLabels << tr("Address");
    headerLabels << tr("Port");
}

BrokerServersModel::~BrokerServersModel()
{
}

QModelIndex BrokerServersModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex BrokerServersModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant BrokerServersModel::data(const QModelIndex &index, int role) const
{
    int currentRow = itemsList.size() - index.row() - 1;
    if (currentRow < 0 || currentRow >= itemsList.size())
        return QVariant();

    if (role != Qt::DisplayRole &&
        role != Qt::ToolTipRole &&
        role != Qt::EditRole &&
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

    if (role == Qt::BackgroundRole) {
        if (!(indexRow % 2)) {
            return appGlobalData.appTheme.altRowColor;
        } else {
            return QVariant();
        }
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (indexRow < itemsList.size()) {
            const ServerItem& item = itemsList[indexRow];
            switch (indexColumn) {
            case 0:
                return QString().fromStdString(item.address);
            case 1:
                if (item.port == 0) {
                    return QVariant();
                } else {
                    return item.port;
                }
            }
        }
    }

    return QVariant();
}

bool BrokerServersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    int indexColumn = index.column();
    int indexRow = index.row();

    if (indexColumn == 0 && value.toString().toStdString().empty()) {
        return false;
    }

    if (indexColumn == 0 && value.toString().toStdString().empty()) {
        return false;
    }

    if (indexRow >= itemsList.size()) {
        return false;
    }

    if (indexColumn == 0) {
        itemsList[indexRow].address = value.toString().toStdString();
    } else if (indexColumn == 1) {
        itemsList[indexRow].port = value.toInt();
    }

    return true;
}

Qt::ItemFlags BrokerServersModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0) {
        int row = index.row();
        if (row < itemsList.count()) {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
        }
    } else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant BrokerServersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();
    
    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int BrokerServersModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.size();
}

int BrokerServersModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

void BrokerServersModel::insertNewRow()
{
    int currentIndex = itemsList.size();
    beginInsertRows(QModelIndex(), currentIndex, currentIndex);
    ServerItem addr;
    addr.address.clear();
    addr.port = 0;
    itemsList.push_back(addr);
    endInsertRows();
}

void BrokerServersModel::removeRow(int row)
{
    if (row < 0 || row >= itemsList.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    itemsList.removeAt(row);
    endRemoveRows();
}

void BrokerServersModel::reset()
{
    beginResetModel();
    itemsList.clear();
    endResetModel();
}

int BrokerServersModel::getServerList(std::vector<ServerItem>& out)
{
    out.clear();

    for (auto& elem : itemsList) {
        out.push_back(elem);
    }

    return out.size();
}

////////////////////////////////////////////////////////////////////////////////
EditBkrDialog::EditBkrDialog(std::list<Broker>& brokers,
                             std::string& bkrName, std::string& bkrID, std::string& srvName,
                             QWidget* parent)
    : brokers(brokers), QDialog(parent)
{
    ui.setupUi(this);

    currEditMode = MODE_MOTIFY;
    origBkrName  = bkrName;
    origBkrID    = bkrID;
    origSrvName  = srvName;

    if (bkrName.empty() || srvName.empty()) {
        currEditMode = MODE_CREATE_NEW;
    }

    std::vector<ServerItem> tradingSrvs;
    std::vector<ServerItem> mdSrvs;

    ui.srvNameLineEdit->setText(QString().fromLocal8Bit(srvName.c_str()));
    for (auto& bkr : brokers) {
        if (bkr.name == bkrName) {
            ui.bkrIDLineEdit->setText(QString().fromStdString(bkr.ID));
            ui.bkrNameLineEdit->setText(QString().fromLocal8Bit(bkr.name.c_str()));
            for (auto& srv : bkr.servers) {
                if (srv.name == srvName) {
                    tradingSrvs = srv.tradingSrvs;
                    mdSrvs = srv.marketdataSrvs;
                }
            }
                
        }
    }
        

    ui.bkrNameLineEdit->setFocus();

    QIntValidator* validator = new QIntValidator(0, 999999, parent);
    ui.bkrIDLineEdit->setValidator(validator);

    tradingSrvsModel = new BrokerServersModel(tradingSrvs, this);
    tradingSrvsDelegate = new BrokerServersDelegate(this);

    ui.tradingSrvs->setModel(tradingSrvsModel);
    ui.tradingSrvs->setItemDelegate(tradingSrvsDelegate);
    ui.tradingSrvs->setColumnWidth(0, 400);
    ui.tradingSrvs->horizontalHeader()->setStretchLastSection(true);

    mdSrvsModel = new BrokerServersModel(mdSrvs, this);
    mdSrvsDelegate = new BrokerServersDelegate(this);

    ui.mdSrvs->setModel(mdSrvsModel);
    ui.mdSrvs->setItemDelegate(mdSrvsDelegate);
    ui.mdSrvs->setColumnWidth(0, 400);
    ui.mdSrvs->horizontalHeader()->setStretchLastSection(true);

    connect(ui.tradingAddBtn, SIGNAL(clicked()), this, SLOT(tradingAddBtnClicked()));
    connect(ui.tradingDeleteBtn, SIGNAL(clicked()), this, SLOT(tradingDeleteBtnClicked()));
    connect(ui.mdAddBtn, SIGNAL(clicked()), this, SLOT(mdAddBtnClicked()));
    connect(ui.mdDeleteBtn, SIGNAL(clicked()), this, SLOT(mdDeleteBtnClicked()));
    connect(ui.createNewBtn, SIGNAL(clicked()), this, SLOT(createNewBtnClicked()));
    connect(ui.deleteBtn, SIGNAL(clicked()), this, SLOT(deleteBtnClicked()));
    connect(ui.okBtn, SIGNAL(clicked()), this, SLOT(okBtnClicked()));
    connect(ui.cancelBtn, SIGNAL(clicked()), this, SLOT(cancelBtnClicked()));

}

EditBkrDialog::~EditBkrDialog()
{

}

const std::string& EditBkrDialog::getCurrBkrName() const
{
    return currBkrName;
}

const std::string& EditBkrDialog::getCurrBkrID() const
{
    return currBkrID;
}

const std::string& EditBkrDialog::getCurrSrvName() const
{
    return currSrvName;
}

void EditBkrDialog::showErrorTips(QString& tips)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tips);
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, tr("Ok"));
    msgBox.exec();
}

bool EditBkrDialog::checkFieldsValue()
{
    QString name = ui.srvNameLineEdit->text();
    QString tips;
    if (name.isEmpty()) {
        tips = tr("Please input server name.");
        showErrorTips(tips);
        ui.srvNameLineEdit->setFocus();
        return false;
    }

    QString bkrName = ui.bkrNameLineEdit->text();
    if (bkrName.isEmpty()) {
        tips = tr("Please input broker name.");
        showErrorTips(tips);
        ui.bkrNameLineEdit->setFocus();
        return false;
    }

    QString brokerID = ui.bkrIDLineEdit->text();
    if (brokerID.isEmpty()) {
        tips = tr("Please input broker ID.");
        showErrorTips(tips);
        ui.bkrIDLineEdit->setFocus();
        return false;
    }

    if (ui.tradingSrvs->model()->rowCount() == 0) {
        tips = tr("Please add at least one trading front.");
        showErrorTips(tips);
        ui.tradingSrvs->setFocus();
        return false;
    }

    if (ui.mdSrvs->model()->rowCount() == 0) {
        tips = tr("Please add at least one MD front.");
        showErrorTips(tips);
        ui.mdSrvs->setFocus();
        return false;
    }

    int bkrID = brokerID.toInt();
    for (auto& bkr : brokers) {
        int id = atoi(bkr.ID.c_str());
        if (id == bkrID) {
            for (auto& srvs : bkr.servers) {
                if (srvs.name == name.toStdString()) {
                    tips = tr("Server name is already existed.");
                    showErrorTips(tips);
                    ui.srvNameLineEdit->setFocus();
                    return false;
                }
            }
        }
    }

    return true;
}

void EditBkrDialog::deleteBrokerConfig()
{
    int bkrID = atoi(origBkrID.c_str());
    for (auto& bkr : brokers) {
        int id = atoi(bkr.ID.c_str());
        if (id == bkrID) {
            auto it = bkr.servers.begin();
            while (it != bkr.servers.end()) {
                if (it->name == origSrvName) {
                    it = bkr.servers.erase(it);
                } else {
                    it++;
                }
            }
        }
    }

    // remove empty items.
    auto itor = brokers.begin();
    while (itor != brokers.end()) {
        if (itor->servers.empty()) {
            itor = brokers.erase(itor);
        } else {
            itor++;
        }
    }

    currBkrID.clear();
    currBkrName.clear();
    currSrvName.clear();

    accept();
}

void EditBkrDialog::motifyBrokerConfig()
{
    if (!checkFieldsValue()) {
        return;
    }

    currSrvName = ui.srvNameLineEdit->text().toLocal8Bit();
    currBkrName = ui.bkrNameLineEdit->text().toLocal8Bit();
    currBkrID = ui.bkrIDLineEdit->text().toLocal8Bit();

    int bkrID = atoi(origBkrID.c_str());
    for (auto& bkr : brokers) {
        int id = atoi(bkr.ID.c_str());
        if (id == bkrID) {
            bkr.ID = currBkrID;
            bkr.name = currBkrName;
            for (auto& s : bkr.servers) {
                if (s.name == origSrvName) {
                    s.name = currSrvName;
                    tradingSrvsModel->getServerList(s.tradingSrvs);
                    mdSrvsModel->getServerList(s.marketdataSrvs);

                    accept();
                    return;
                }
            }

            accept();
            return;
        }
    }
}

void EditBkrDialog::addBrokerConfig()
{
    if (!checkFieldsValue()) {
        return;
    }

    QString name = ui.srvNameLineEdit->text();
    QString bkrName = ui.bkrNameLineEdit->text();
    QString brokerID = ui.bkrIDLineEdit->text();

    int bkrID = brokerID.toInt();
    for (auto& bkr : brokers) {
        int id = atoi(bkr.ID.c_str());
        if (id == bkrID) {
            BrokerServer srv;
            srv.name = name.toLocal8Bit();
            tradingSrvsModel->getServerList(srv.tradingSrvs);
            mdSrvsModel->getServerList(srv.marketdataSrvs);
            bkr.servers.push_back(srv);
            
            currBkrID = bkr.ID;
            currBkrName = bkr.name;
            currSrvName = srv.name;

            accept();

            return;
        }
    }

    Broker bkr;
    bkr.ID = brokerID.toStdString();
    bkr.name = bkrName.toLocal8Bit();
    BrokerServer srv;
    srv.name = name.toLocal8Bit();
    tradingSrvsModel->getServerList(srv.tradingSrvs);
    mdSrvsModel->getServerList(srv.marketdataSrvs);
    bkr.servers.push_back(srv);

    brokers.push_back(bkr);

    currBkrID = bkr.ID;
    currBkrName = bkr.name;
    currSrvName = srv.name;

    accept();

    return;
}

void EditBkrDialog::tradingAddBtnClicked()
{
    int rowCount = ui.tradingSrvs->model()->rowCount();

    if (rowCount == 0) {
        tradingSrvsModel->insertNewRow();
        rowCount = ui.tradingSrvs->model()->rowCount();
    } else {
        QModelIndex idx = tradingSrvsModel->index(rowCount - 1, 0);
        QString addr = tradingSrvsModel->data(idx).toString();
        idx = tradingSrvsModel->index(rowCount - 1, 1);
        int port = tradingSrvsModel->data(idx).toInt();
        if (!addr.isEmpty() && port != 0) {
            tradingSrvsModel->insertNewRow();
            rowCount = ui.tradingSrvs->model()->rowCount();
        }
    }

    ui.tradingSrvs->selectRow(rowCount-1);
    QModelIndex index = ui.tradingSrvs->model()->index(rowCount-1, 0, QModelIndex());
    ui.tradingSrvs->edit(index);
}

void EditBkrDialog::tradingDeleteBtnClicked()
{
    tradingSrvsModel->removeRow(ui.tradingSrvs->currentIndex().row());
}

void EditBkrDialog::mdAddBtnClicked()
{
    int rowCount = ui.mdSrvs->model()->rowCount();

    if (rowCount == 0) {
        mdSrvsModel->insertNewRow();
        rowCount = ui.mdSrvs->model()->rowCount();
    } else {
        QModelIndex idx = mdSrvsModel->index(rowCount - 1, 0);
        QString addr = mdSrvsModel->data(idx).toString();
        idx = mdSrvsModel->index(rowCount - 1, 1);
        int port = mdSrvsModel->data(idx).toInt();
        if (!addr.isEmpty() && port != 0) {
            mdSrvsModel->insertNewRow();
            rowCount = ui.mdSrvs->model()->rowCount();
        }
    }

    ui.mdSrvs->selectRow(rowCount - 1);
    QModelIndex index = ui.mdSrvs->model()->index(rowCount - 1, 0, QModelIndex());
    ui.mdSrvs->edit(index);
}

void EditBkrDialog::mdDeleteBtnClicked()
{
    mdSrvsModel->removeRow(ui.mdSrvs->currentIndex().row());
}

void EditBkrDialog::createNewBtnClicked()
{
    currEditMode = MODE_CREATE_NEW;
    setWindowTitle(tr("Create New"));

    ui.srvNameLineEdit->clear();
    ui.bkrIDLineEdit->clear();
    ui.bkrNameLineEdit->clear();
    tradingSrvsModel->reset();
    mdSrvsModel->reset();

    ui.bkrNameLineEdit->setFocus();
}

void EditBkrDialog::deleteBtnClicked()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tr("Are you sure you want to delete this configuration?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    if (currEditMode == MODE_CREATE_NEW) {
        reject();
    } else {
        deleteBrokerConfig();
    }
}

void EditBkrDialog::okBtnClicked()
{
    if (currEditMode == MODE_CREATE_NEW) {
        addBrokerConfig();
    } else if (currEditMode == MODE_MOTIFY) {
        motifyBrokerConfig();
    }
}

void EditBkrDialog::cancelBtnClicked()
{
    reject();
}