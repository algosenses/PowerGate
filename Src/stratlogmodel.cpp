#include <QApplication>
#include <QFontMetrics>
#include <QDateTime>
#include <QDate>
#include "main.h"
#include "stratlogmodel.h"

StratLogModel::StratLogModel(QObject* parent)
	: QAbstractItemModel(parent)
{
	headerLabels << tr("DateTime");
	headerLabels << tr("Level");
	headerLabels << tr("Text");

	dateWidth = qMax(QApplication::fontMetrics().width("2000/12/30 23:59:59.999"), QApplication::fontMetrics().width("2000/12/30 12:59:59.999")) + 10;
}

StratLogModel::~StratLogModel()
{
	itemsList.clear();
}

QModelIndex StratLogModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))return QModelIndex();
	return createIndex(row, column);
}

QModelIndex StratLogModel::parent(const QModelIndex &index) const
{
	return QModelIndex();
}

QString StratLogModel::getLogLevelStr(int level) const
{
	switch (level) {
	default:
	case AlgoSE::LogLevel::LOG_INFO:
		return "INFO";

	case AlgoSE::LogLevel::LOG_DEBUG:
		return "DBG";

	case AlgoSE::LogLevel::LOG_WARN:
		return "WARN";

	case AlgoSE::LogLevel::LOG_ERROR:
		return "ERR";

	case AlgoSE::LogLevel::LOG_FATAL:
		return "FATAL";
	}

	return "";
}

QVariant StratLogModel::data(const QModelIndex &index, int role) const
{
	int currentRow = itemsList.count() - index.row() - 1;
	if (currentRow < 0 || currentRow >= itemsList.count()) return QVariant();

	if (role == Qt::ToolTipRole) {
		return QString(itemsList.at(currentRow).datetime) + "\t" + getLogLevelStr(itemsList.at(currentRow).log.level) + "\t" + QString().fromLocal8Bit(itemsList.at(currentRow).log.text);
	}

	if (role != Qt::DisplayRole &&
		role != Qt::ToolTipRole &&
		role != Qt::ForegroundRole &&
		role != Qt::TextAlignmentRole &&
		role != Qt::BackgroundRole) {
		return QVariant();
	}

	int indexRow = index.row();
	if (role == Qt::BackgroundRole)
	{
		if (!(indexRow % 2)) {
			return appGlobalData.appTheme.altRowColor;
		}
		else {
			return QVariant();
		}
	}

	if (role == Qt::ForegroundRole) {
		if (currentRow < itemsList.size()) {
			int level = itemsList.at(currentRow).log.level;
			if (level == AlgoSE::LogLevel::LOG_WARN ||
				level == AlgoSE::LogLevel::LOG_ERROR ||
				level == AlgoSE::LogLevel::LOG_FATAL) {
				return appGlobalData.appTheme.red;
			}
		}

		return appGlobalData.appTheme.black;
	}

	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft | Qt::AlignVCenter;
	}

	if (role == Qt::DisplayRole) {
		int indexColumn = index.column();

		switch (indexColumn) {
		case 0:
			return itemsList.at(currentRow).datetime;
		case 1:
			return getLogLevelStr(itemsList.at(currentRow).log.level);
		case 2:
			return QString().fromLocal8Bit(itemsList.at(currentRow).log.text);

		default:
			break;
		}
	}

	return QVariant();
}

Qt::ItemFlags StratLogModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant StratLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)return QVariant();

	if (role == Qt::SizeHintRole)
	{
		switch (section)
		{
		case 0: return QSize(dateWidth, defaultHeightForRow);//DateTime
		}
		return QVariant();
	}

	if (role != Qt::DisplayRole) return QVariant();
	if (section >= headerLabels.count()) return QVariant();

	return headerLabels.at(section);
}

int StratLogModel::rowCount(const QModelIndex &parent) const
{
	return itemsList.count();
}

int StratLogModel::columnCount(const QModelIndex &parent) const
{
	return headerLabels.count();
}

void StratLogModel::clear()
{
	beginResetModel();
	itemsList.clear();
	endResetModel();
}

void StratLogModel::insertLogItem(const AlgoSE::StrategyLog& log)
{
	int currentIndex = itemsList.size();
	beginInsertRows(QModelIndex(), currentIndex, currentIndex);
	LogItem item;
	item.log = log;
    int year, month, day, hour, minute, sec, ms;
    DateTime(log.timestamp).getFields(year, month, day, hour, minute, sec, ms);
    sprintf(item.datetime, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
        year, month, day, hour, minute, sec, ms);
	itemsList.push_front(item);
	endInsertRows();
}