#ifndef POWER_GATE_H
#define POWER_GATE_H

#include <winsock.h>
#include <windows.h>

#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <set>
#include <QMainWindow>
#include <QMenu>
#include <QSplitter>
#include <QSettings>
#include <QCloseEvent>
#include "ui_powergate.h"
#include "quotemodel.h"
#include "quotedialog.h"
#include "logmodel.h"
#include "stratmodel.h"
#include "ordermodel.h"
#include "posmodel.h"
#include "stratlogdialog.h"
#include "logindialog.h"
#include "tradepanel.h"
#include "Semaphore.h"

#include "Defines.h"
#include "Engine.h"
#include "Service.h"
#include "Strategy.h"

using namespace AlgoSE;

using std::vector;
using std::unordered_map;

class DockHost;

#define APP_CONFIG_FILE_NAME            ("PowerGate.ini")
#define APP_ENCRYPT_KEY                 "APP_ENCRYPT_KEY" 
#define SCHED_TASKS_TIMER_INTERVAL      (2000)  // ms

enum {
    STRATEGY_SCHED_START,
    STRATEGY_SCHED_PAUSE,
    STRATEGY_SCHED_RESUME,
    STRATEGY_SCHED_STOP,
};

class SrvStLabelCtxMenu : public QMenu
{
    Q_OBJECT
public:
    explicit SrvStLabelCtxMenu(int srvType, int srvId, QWidget *parent = 0);
    int getSrvType() const { return srvType; }
    int getSrvId() const { return srvId; }

public slots :
    void showMenu(const QPoint &pos);

private:
    int srvType;
    int srvId;
};

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(const QString& text = "", QWidget* parent = 0)
        : QLabel(parent)
    {
        setText(text);
    }

    ~ClickableLabel() {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event)
    {
        emit clicked();
    }
};

////////////////////////////////////////////////////////////////////////////////
class AlertManager
{
public:
    AlertManager();
    static AlertManager* getInstance();
    int  generateAlertId();
    void addAlert(int alertId);
    void cancelAlert(int alertId);
    void stop();

private:
    void alert();

private:
    unsigned long m_nextAlertId;

    std::mutex m_alertsLock;
    std::set<int> m_alerts;

    std::thread* m_alertThread;
    bool m_alertStop;
    Utils::DefaultSemaphoreType m_signal;

    static AlertManager* m_instance;
};

////////////////////////////////////////////////////////////////////////////////
class Mailer
{
public:
    Mailer();
    bool readConfig(const char* file);
    bool sendMail(const char* subject, const std::vector<std::string>& body);

private:
    bool sendMail(const char* subject, char* body);

private:
    bool m_enable;
    std::string m_sender;
    std::string m_from;
    std::string m_to;
    std::string m_username;
    std::string m_passwd;
    std::string m_smtpSrv;
    int m_smtpPort;
    std::string m_computerName;
    std::string m_computerUser;
};

////////////////////////////////////////////////////////////////////////////////
class PowerGate : public QMainWindow, AlgoSE::IEngine::Listener
{
    Q_OBJECT

public:
    PowerGate();
    ~PowerGate();

    void closeEvent(QCloseEvent *event);

    // callbacks
    void onSystemLog(const SystemLog& log);
    void onServiceStatus(const ServiceStatus& state);
    void onAccountDetails(const AccountDetails& account);
    void onAccountPosition(const AccountPosition& position);
    void onStrategyParameter(const StrategyParam& param);
    void onStrategyPosition(const StrategyPosition& position);
    void onStrategyLog(const StrategyLog& log);
    void onTick(const Tick& tick);
    void onOrderStatus(const Order& order);
    void onExecutionReport(const Execution& exec);
    void onStrategyStatus(const StrategyStatus& state);

    bool eventFilter(QObject *target, QEvent *event);
    
private:
    void moveWidgetsToSplitter();
    void fixTableViews(QWidget *wid);
    void setColumnResizeMode(QTableView*, int, QHeaderView::ResizeMode);
    void setColumnResizeMode(QTableView*, QHeaderView::ResizeMode);
    void saveAppLayout();
    bool confirmExitApp();
    bool writeUILog(int level, const char* msgFmt, ...);

private slots:
    void restoreAppLayout();
    void startLogin();
    void initAlgoSE();
    void updateSystemLog(SystemLog log);
    void updateServiceStatus(ServiceStatus status);
    void updateAccountDetails(AccountDetails acct);
    void updateStratParams(StrategyParam param);
    void updateStratPositions(StrategyPosition pos);
    void updateStratLog(StrategyLog log);
    void updateOrderStatus(Order order);
    void updateStratStatus(StrategyStatus state);
    void updateExecution(Execution exec);
    void updatePosition(AccountPosition pos);
    void updateQuote(Tick tick);
    void sendReportMail();
    void closeApp();
    void exitApp();
    void onStratTableViewClicked(const QModelIndex &index);
    void onStratTableViewDoubleClicked(const QModelIndex &index);
    void onQuoteTableViewDoubleClicked(const QModelIndex &index);
    void onPosTableViewDoubleClicked(const QModelIndex &index);
    void onOrderTableViewDoubleClicked(const QModelIndex &index);
    void onOrderTableViewMenuRequested(const QPoint& pos);
    void onQuoteTableViewMenuRequested(QPoint pos);
    void onQuoteDlgExit(const string& instrumet);
    void onStartAllStrategiesBtnClicked();
    void onStopAllStrategiesBtnClicked();
    void onTradePanelBtnClicked();
    void onCancelAllOrdersBtnClicked();
    void onCloseAllPosBtnClicked();
    void onVerboseLogCBClicked(bool toggled);
    void onStratLogDlgExit(int stratId);
    void timeElapsed();
    void reloadService();
    void subscribeQuote();
    void unsubscribeQuote();
    void popupDepthQuoteDialog();
    void popupTradePanelFromQuoteTableView();
    void exportOrderList();

signals:
    void signalSystemLog(SystemLog);
    void signalServiceStatus(ServiceStatus);
    void signalAccountDetails(AccountDetails);
    void signalStratParams(StrategyParam);
    void signalStratPositions(StrategyPosition);
    void signalStratLog(StrategyLog);
    void signalOrderStatus(Order);
    void signalStratStatus(StrategyStatus);
    void signalExecutionReport(Execution);
    void signalPosition(AccountPosition);
    void signalQuote(Tick);


private:
    void updateQuoteDialogs(const Tick& tick);

    void onParamCellClicked(const QModelIndex& index);
    void onStartCellClicked(const QModelIndex& index);
    void onPauseCellClicked(const QModelIndex& index);
    void onLongPosCellClicked(const QModelIndex& index);
    void onShortPosCellClicked(const QModelIndex& index);
    void onShowLogCellClicked(const QModelIndex& index);
    void onStratLogCellClicked(const QModelIndex& index);
    void onStratCmdCellClicked(const QModelIndex& index);

    void showDepthQuoteDialog(const string& instrument);
    void showStratLogDlg(int stratId, QString& stratName);
    void showTradePanel(const string& instrument, int side, int quantity);
    void installSchedTask();
    void checkSchedTask();
    void readRestartTime();
    bool checkRestartTask();
    void readStrategiesSchedTime();
    void readSendReportMailTime();
    bool checkStrategiesSchedTask();
    bool checkSendMailSchedTask();

private:
    Engine*        pAlgoSE;

    QuoteModel*    quoteModel;
    LogModel*      logModel;
    StratModel*    stratModel;
    OrderModel*    orderModel;
    PositionModel* posModel;
    AlgoSE::AccountDetails acctDetails;

    std::unordered_map<std::string, Tick> lastQuotes;
    typedef struct {
        int time;
        double price;
        int volume;
    } TradeItem;
    std::unordered_map<std::string, circular_buffer<Tick> > lastTrades;

    std::unordered_map<std::string, QuoteDialog*> depthQuoteDialogs;

    TradePanel* tradePanel;

    QWidget *windowWidget;
    QSplitter* splitterMain;
    QSplitter* splitterCenter;
    QSplitter* splitterBottom;

    typedef struct {
        int id;
        StrategyStatus status;
        vector<StrategyPosition> pos;
        vector<StrategyParam> params;
        int alertId;
    } StratItem;

    std::vector<StratItem> strategies;

    std::unordered_map<int, vector<StrategyLog> > strategiesLog;
    std::unordered_map<int, StratLogDialog*> stratLogDlgs;

    typedef struct {
        ServiceStatus status;
        ClickableLabel* label;
        int alertId;
    } SrvStatus;

    vector<SrvStatus> srvStatus;

private:
    QString appDir;

    Ui::PowerGate ui;

    bool titleShowed;
    bool verboseLog;
    bool subscribeDlgShowed;

    AlertManager* m_alertMgr;

    std::string m_initMailSubject;
    std::vector<std::string> m_initMailLogs;
    bool m_reportMailSent;
    Mailer m_mailer;

    QTimer* m_timer;


    unsigned long m_schedLastSec;
    unsigned long m_schedCurrSec;
    unsigned long m_restartSecs;

    std::vector<long>  m_restartTime;
    std::vector<long>  m_stopTime;

    std::vector<long> m_sendReportMailTime;
    
    typedef struct {
        bool executed;
        int  type;
        unsigned long execSecs;
    } StrategySchedTask;

    vector<StrategySchedTask> m_strategiesSchedTasks;
};

#endif // POWER_GATE_H