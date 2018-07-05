#include <QLineEdit>
#include "main.h"
#include "stratposmodel.h"

StratPosDelegate::StratPosDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}

QWidget *StratPosDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex &/* index */) const
{
    QLineEdit *editor = new QLineEdit(parent);
    editor->setFrame(false);

    return editor;
}

void StratPosDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void StratPosDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    QString text = lineEdit->text();

    model->setData(index, text, Qt::EditRole);
}

void StratPosDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

////////////////////////////////////////////////////////////////////////////////
StratPosModel::StratPosModel(vector<StrategyPosition>& position, QObject *parent)
    : QAbstractItemModel(parent)
{
    for (size_t i = 0; i < position.size(); i++) {
        itemsList.append(position[i]);
    }

    headerLabels << tr("Instrument");
    headerLabels << tr("Quantity");
    headerLabels << tr("Price");
}

StratPosModel::~StratPosModel()
{
}

QModelIndex StratPosModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return createIndex(row, column);
}

QModelIndex StratPosModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant StratPosModel::data(const QModelIndex &index, int role) const
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
            const StrategyPosition& item = itemsList[indexRow];
            switch (indexColumn) {
            case 0:
                return item.instrument;
            case 1:
                return item.quantity;
            case 2:
                return item.avgPx;
            }
        }
    }

    return QVariant();
}

bool StratPosModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    int indexColumn = index.column();
    int indexRow = index.row();
    if (indexRow >= itemsList.size()) {
        return false;
    }

    if (indexColumn == 0) {
        strncpy(itemsList[indexRow].instrument, value.toString().toStdString().c_str(), sizeof(itemsList[indexRow].instrument));
    } else if (indexColumn == 1) {
        itemsList[indexRow].quantity = value.toInt();
    } else if (indexColumn == 2) {
        itemsList[indexRow].avgPx = value.toDouble();
    }

    return true;
}

Qt::ItemFlags StratPosModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0) {
        int row = index.row();
        if (row < itemsList.count()) {
            if (string(DefaultInstrumentName) == itemsList[row].instrument) {
                return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
            } else {
                return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
            }
        }
    } else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant StratPosModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();

    if (role != Qt::DisplayRole) return QVariant();
    if (section >= headerLabels.count()) return QVariant();

    return headerLabels.at(section);
}

int StratPosModel::rowCount(const QModelIndex &parent) const
{
    return itemsList.size();
}

int StratPosModel::columnCount(const QModelIndex &parent) const
{
    return headerLabels.count();
}

void StratPosModel::insertNewRow()
{
    for (size_t i = 0; i < itemsList.size(); i++) {
        if (string("instrument") == itemsList[i].instrument) {
            return;
        }
    }

    int currentIndex = itemsList.size();
    beginInsertRows(QModelIndex(), currentIndex, currentIndex);
    StrategyPosition pos = { 0 };
    strcpy(pos.instrument, DefaultInstrumentName);
    itemsList.push_back(pos);
    endInsertRows();
}

void StratPosModel::removeRow(int row)
{
    if (row < 0 || row >= itemsList.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    itemsList.removeAt(row);
    endRemoveRows();
}

QList<StrategyPosition>& StratPosModel::getAllPositions()
{
    return itemsList;
}