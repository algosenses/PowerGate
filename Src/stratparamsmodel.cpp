#include <QLineEdit>
#include "main.h"
#include "stratparamsmodel.h"

StratParamsDelegate::StratParamsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *StratParamsDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex &/* index */) const
{
    QLineEdit *editor = new QLineEdit(parent);
    editor->setFrame(false);

    return editor;
}

void StratParamsDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void StratParamsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    QString text = lineEdit->text();

    model->setData(index, text, Qt::EditRole);
}

void StratParamsDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

////////////////////////////////////////////////////////////////////////////////
StratParamsModel::StratParamsModel(int category, QObject *parent)
    : QAbstractItemModel(parent)
{
    paramCategory = category;
    
    headerLabels << "Name";
    headerLabels << "Value";
}

StratParamsModel::~StratParamsModel()
{
}

const QStringList& StratParamsModel::getHeaderLabels() const
{
    return headerLabels;
}

QModelIndex StratParamsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex StratParamsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant StratParamsModel::data(const QModelIndex &index, int role) const
{
    int currentRow = itemsList.count() - index.row() - 1;
    if (currentRow < 0 || currentRow >= itemsList.count())
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
            const StrategyParam& item = itemsList[indexRow].param;
            switch (indexColumn) {
            case 0:
                return QString().fromUtf8(item.name);
            case 1:
                return QString().fromUtf8(item.value);
            }
        }
    }

    return QVariant();
}

bool StratParamsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    int indexColumn = index.column();
    int indexRow = index.row();
    if (indexRow >= itemsList.size() || indexColumn != 1) {
        return false;
    }

    strcpy(itemsList[indexRow].param.value, value.toString().toStdString().c_str());
    itemsList[indexRow].touch = true;

    return true;
}

Qt::ItemFlags StratParamsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 1) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant StratParamsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();

    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int StratParamsModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.count();
}

int StratParamsModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

void StratParamsModel::updateParamItem(const AlgoSE::StrategyParam& param)
{
    if (param.category != paramCategory) {
        return;
    }

    int i = 0;
    int size = itemsList.size();
    for (i = 0; i < size; i++) {
        if (itemsList[i].param.strategyId == param.strategyId &&
            !stricmp(itemsList[i].param.name, param.name)) {
            break;
        }
    }

    if (i == size) {
        int currentIndex = size;
        beginInsertRows(QModelIndex(), currentIndex, currentIndex);
        StratParamItem item;
        item.touch = false;
        memcpy(&item.param, &param, sizeof(StrategyParam));
        itemsList.push_back(item);
        endInsertRows();
        emit dataChanged(index(itemsList.size() - 1, 0), index(itemsList.size() - 1, headerLabels.size() - 1));
    } else {
        itemsList[i].param = param;
        emit dataChanged(index(i, 0), index(i, headerLabels.size() - 1));
    }
}

void StratParamsModel::getTouchedParams(std::vector<StrategyParam>& params)
{
    for (int i = 0; i < itemsList.size(); i++) {
        if (itemsList[i].touch) {
            params.push_back(itemsList[i].param);
        }
    }
}