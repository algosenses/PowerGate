#include <QApplication>
#include <QDesktopWidget>
#include <QSplitter>
#include <QTimer>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include "config.h"
#include "quickmail.h"
#include "main.h"
#include "powergate.h"
#include "stratparamsdialog.h"
#include "stratposdialog.h"
#include "stratcmddialog.h"
#include "subscribedialog.h"
#include "quotedialog.h"
#include "RestartAPI.h"
#include "TinyCrypt.h"

#pragma comment(lib, "Winmm.lib")

SrvStLabelCtxMenu::SrvStLabelCtxMenu(int srvType, int srvId, QWidget *parent) :
    QMenu(parent)
{
    this->srvType = srvType;
    this->srvId = srvId;
}

void SrvStLabelCtxMenu::showMenu(const QPoint &pos)
{
    exec(QCursor::pos());
}

////////////////////////////////////////////////////////////////////////////////
AlertManager* AlertManager::m_instance = nullptr;

AlertManager* AlertManager::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new AlertManager();
    }

    return m_instance;
}

AlertManager::AlertManager()
{
    m_nextAlertId = 0;
    m_alertThread = nullptr;
    m_alertStop = false;
    m_alertThread = new std::thread(std::bind(&AlertManager::alert, this));
}

int AlertManager::generateAlertId()
{
    return ++m_nextAlertId;
}

void AlertManager::addAlert(int alertId)
{
    if (alertId <= 0) {
        return;
    }

    m_alertsLock.lock();
    m_alerts.insert(alertId);
    m_alertsLock.unlock();

#if defined(_WIN32) || defined(_WIN64)
    FLASHWINFO fInfo;
    fInfo.cbSize = sizeof(FLASHWINFO);
    fInfo.hwnd = (HWND)mainWindow.winId();
    fInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
    fInfo.uCount = ~0;
    fInfo.dwTimeout = 0;
    ::FlashWindowEx(&fInfo);
    
    m_signal.signal();
#else
    QApplication::alert(QApplication::activeWindow(), 1000);
#endif
}

void AlertManager::cancelAlert(int alertId)
{
    m_alertsLock.lock();
    m_alerts.erase(alertId);
    m_alertsLock.unlock();
}

void AlertManager::alert()
{
    while (!m_alertStop) {
        m_signal.wait();

        while (!m_alertStop) {
            if (m_alertStop) {
                return;
            }

            m_alertsLock.lock();
            int size = m_alerts.size();
            m_alertsLock.unlock();

            if (size == 0) {
                break;
            }

            // 523 hertz (C5) for 1000 milliseconds
            // Beep(523, 1000);
            PlaySound(L"warning.wav", NULL, SND_FILENAME);
            Sleep(500);
        }
    }
}

void AlertManager::stop()
{
    if (m_alertThread) {
        m_alertStop = true;
        m_signal.signal();
        m_alertThread->join();
        delete m_alertThread;
    }
}

////////////////////////////////////////////////////////////////////////////////
Mailer::Mailer()
{
    m_enable = false;
}

bool Mailer::readConfig(const char* file)
{
    rude::Config config;
    if (config.load(file)) {
        config.setSection("Mail");
        m_enable = config.getBoolValue("Enable");
        if (!m_enable) {
            return false;
        }

        const char* username = config.getStringValue("UserName");
        if (username && username[0] != '\0') {
            m_username = username;
        } else {
            m_enable = false;
            return false;
        }

        const char* passwd = config.getStringValue("Password");
        if (passwd && passwd[0] != '\0') {
            const unsigned char k[] = APP_ENCRYPT_KEY;
            std::string key((const char*)k);
            TinyCrypt::CryptObject cryptObj((unsigned char*)key.c_str());
            std::string pwd = cryptObj.decrypt(passwd);
            m_passwd = pwd;
        } else {
            m_enable = false;
            return false;
        }

        const char* smtpSrv = config.getStringValue("Server");
        if (smtpSrv && smtpSrv[0] != '\0') {
            m_smtpSrv = smtpSrv;
        } else {
            m_enable = false;
            return false;
        }

        m_smtpPort = config.getIntValue("Port");

        char* computerName = NULL;
        char* computerUser = NULL;
        computerName = getenv("COMPUTERNAME");
        if (computerName) {
            m_computerName = computerName;
        }
        computerUser = getenv("USERNAME");
        if (computerUser) {
            m_computerUser = computerUser;
        }
        m_sender = std::string("PG@") + computerName;
    }

    return true;
}

bool Mailer::sendMail(const char* subject, char* body)
{
    if (!m_enable) {
        return false;
    }

    int ret = -1;
    quickmail mailobj;
    ret = quickmail_initialize();
    mailobj = quickmail_create(NULL, NULL);
    quickmail_set_from(mailobj, m_username.c_str());
    quickmail_set_sender(mailobj, m_sender.c_str());
    quickmail_add_to(mailobj, m_username.c_str());
    quickmail_set_subject(mailobj, subject);
    quickmail_add_body_memory(mailobj, NULL, body, strlen(body), 0);
    const char* msg = quickmail_send_secure(mailobj, m_smtpSrv.c_str(), 465, m_username.c_str(), m_passwd.c_str());
    quickmail_destroy(mailobj);
    ret = quickmail_cleanup();

    return ret == 0;
}

bool Mailer::sendMail(const char* subject, const std::vector<std::string>& body)
{
    if (!m_enable) {
        return false;
    }

    if (body.size() == 0) {
        return false;
    }

    std::string content;

    char header[1024];
    sprintf(header, "Machine: %s\r\n", m_computerName.empty() ? "N/A" : m_computerName.c_str());
    content += header;

    sprintf(header, "User: %s\r\n", m_computerUser.empty() ? "N/A" : m_computerUser.c_str());
    content += header;

    SYSTEMTIME lt;
    GetLocalTime(&lt);
    char datetime[64];
    sprintf(datetime, "%04d/%02d/%02d %02d:%02d:%02d.%03d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
    sprintf(header, "DateTime: %s\r\n\r\n", datetime);
    content += header;

    for (auto& line : body) {
        content += line;
        content += "\r\n";
    }

    return sendMail(subject, (char*)content.c_str());
}

////////////////////////////////////////////////////////////////////////////////
PowerGate::PowerGate()
    : QMainWindow()
{
    windowWidget = this;
    appDir = QApplication::applicationDirPath()+"/";
    tradePanel = nullptr;
    titleShowed = false;
    verboseLog = false;

    ui.setupUi(this);

    logModel = new LogModel(this);
    ui.logsTableView->setModel(logModel);
    setColumnResizeMode(ui.logsTableView, 0, QHeaderView::ResizeToContents);
    setColumnResizeMode(ui.logsTableView, 1, QHeaderView::ResizeToContents);
    setColumnResizeMode(ui.logsTableView, 2, QHeaderView::Stretch);
    ui.logsTableView->horizontalHeader()->setMinimumSectionSize(0);
    QObject::connect(logModel, &LogModel::rowsInserted, [this]()
    {
        if (ui.logsTableView->indexAt(ui.logsTableView->rect().bottomLeft()).row() == logModel->rowCount()-1) {
            QTimer::singleShot(0, ui.logsTableView, SLOT(scrollToBottom()));
        }
    });

    int i;

    quoteModel = new QuoteModel(this);
    ui.quoteTableView->setModel(quoteModel);
    setColumnResizeMode(ui.quoteTableView, 11, QHeaderView::ResizeToContents);
    setColumnResizeMode(ui.quoteTableView, 12, QHeaderView::Stretch);
    QObject::connect(ui.quoteTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onQuoteTableViewDoubleClicked(const QModelIndex&)));
    ui.quoteTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui.quoteTableView, SIGNAL(customContextMenuRequested(QPoint)),
        SLOT(onQuoteTableViewMenuRequested(QPoint)));

    stratModel = new StratModel(this);
    ui.modelsTableView->setModel(stratModel);
    for (i = 0; i < stratModel->getHeaderLabels().size()-1; i++) {
        setColumnResizeMode(ui.modelsTableView, i, QHeaderView::ResizeToContents);
    }
//    setColumnResizeMode(ui.modelsTableView, i, QHeaderView::Stretch);
    ui.modelsTableView->horizontalHeader()->setStretchLastSection(true);

    QObject::connect(ui.modelsTableView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onStratTableViewClicked(const QModelIndex&)));
    QObject::connect(ui.modelsTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onStratTableViewDoubleClicked(const QModelIndex&)));

    orderModel = new OrderModel(this);
    ui.orderTableView->setModel(orderModel);
//    setColumnResizeMode(ui.orderTableView, i, QHeaderView::Stretch);
    ui.orderTableView->setColumnWidth(10, 200);
    ui.orderTableView->setColumnWidth(11, 500);
    ui.orderTableView->horizontalHeader()->setStretchLastSection(true);
    QObject::connect(ui.orderTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onOrderTableViewDoubleClicked(const QModelIndex&)));
    ui.orderTableView->resizeColumnsToContents();
    QObject::connect(ui.orderTableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onOrderTableViewMenuRequested(const QPoint&)));

    posModel = new PositionModel(this);
    ui.posTableView->setModel(posModel);
    for (i = 0; i < posModel->getHeaderLabels().size() - 1; i++) {
//        setColumnResizeMode(ui.posTableView, i, QHeaderView::ResizeToContents);
    }
//    setColumnResizeMode(ui.posTableView, i, QHeaderView::Stretch);
    ui.posTableView->horizontalHeader()->setStretchLastSection(true);

    QObject::connect(ui.posTableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onPosTableViewDoubleClicked(const QModelIndex&)));

    QObject::connect(ui.startAllBtn, SIGNAL(clicked()), this, SLOT(onStartAllStrategiesBtnClicked()));
    QObject::connect(ui.stopAllBtn, SIGNAL(clicked()), this, SLOT(onStopAllStrategiesBtnClicked()));
    QObject::connect(ui.manualPanelBtn, SIGNAL(clicked()), this, SLOT(onTradePanelBtnClicked()));
    QObject::connect(ui.cancelAllBtn, SIGNAL(clicked()), this, SLOT(onCancelAllOrdersBtnClicked()));
    QObject::connect(ui.closeAllBtn, SIGNAL(clicked()), this, SLOT(onCloseAllPosBtnClicked()));
    QObject::connect(ui.verboseCheckBox, SIGNAL(toggled(bool)), this, SLOT(onVerboseLogCBClicked(bool)));

    fixTableViews(this);

    moveWidgetsToSplitter();


    /* With queued connections, Qt must store a copy of the arguments that were passed to 
     * the signal so that it can pass them to the slot later on. Qt knows how to take of copy 
     * of many C++ and Qt types, but "Message" isn't one of them. We must therefore call 
     * the template function qRegisterMetaType() before we can use "Message" as parameter 
     * in queued connections.
     */
    /* When using signals and slots across threads, we must register argument's type of slots */
    /* For example, Engine.dll callback function emits signal, this signal connect to Quote model's
     * slot which in Qt's main thread, in this case signal-slot connection types is 'Queued Connection',
     * so we need to register "AlgoSE::Message" which is one argument type of signal atsMsgReceived().
     */
    /* Reference: http://doc.qt.io/qt-5/threads-qobject.html#signals-and-slots-across-threads */
//    qRegisterMetaType<ATS::Message>("Message");
    qRegisterMetaType<AlgoSE::SystemLog>("SystemLog");
    qRegisterMetaType<AlgoSE::ServiceStatus>("ServiceStatus");
    qRegisterMetaType<AlgoSE::AccountDetails>("AccountDetails");
    qRegisterMetaType<AlgoSE::StrategyParam>("StrategyParam");
    qRegisterMetaType<AlgoSE::StrategyPosition>("StrategyPosition");
    qRegisterMetaType<AlgoSE::StrategyLog>("StrategyLog");
    qRegisterMetaType<AlgoSE::Order>("Order");
    qRegisterMetaType<AlgoSE::StrategyStatus>("StrategyStatus");
    qRegisterMetaType<AlgoSE::Execution>("Execution");
    qRegisterMetaType<AlgoSE::AccountPosition>("AccountPosition");
    qRegisterMetaType<AlgoSE::Tick>("Tick");

    QObject::connect(this, SIGNAL(signalSystemLog(SystemLog)), this, SLOT(updateSystemLog(SystemLog)));
    QObject::connect(this, SIGNAL(signalServiceStatus(ServiceStatus)), this, SLOT(updateServiceStatus(ServiceStatus)));
    QObject::connect(this, SIGNAL(signalAccountDetails(AccountDetails)), this, SLOT(updateAccountDetails(AccountDetails)));
    QObject::connect(this, SIGNAL(signalStratParams(StrategyParam)), this, SLOT(updateStratParams(StrategyParam)));
    QObject::connect(this, SIGNAL(signalStratPositions(StrategyPosition)), this, SLOT(updateStratPositions(StrategyPosition)));
    QObject::connect(this, SIGNAL(signalStratLog(StrategyLog)), this, SLOT(updateStratLog(StrategyLog)));
    QObject::connect(this, SIGNAL(signalOrderStatus(Order)), this, SLOT(updateOrderStatus(Order)));
    QObject::connect(this, SIGNAL(signalStratStatus(StrategyStatus)), this, SLOT(updateStratStatus(StrategyStatus)));
    QObject::connect(this, SIGNAL(signalExecutionReport(Execution)), this, SLOT(updateExecution(Execution)));
    QObject::connect(this, SIGNAL(signalPosition(AccountPosition)), this, SLOT(updatePosition(AccountPosition)));
    QObject::connect(this, SIGNAL(signalQuote(Tick)), this, SLOT(updateQuote(Tick)));

    subscribeDlgShowed = false;

    QTimer::singleShot(0, this, SLOT(restoreAppLayout()));

    pAlgoSE = Engine::getInstance();
    pAlgoSE->registerListener(this);

    if (!appGlobalData.requestLogin) {
        QTimer::singleShot(300, this, SLOT(initAlgoSE()));
    } else {
        QTimer::singleShot(0, this, SLOT(startLogin()));
    }

    m_alertMgr = AlertManager::getInstance();

    m_reportMailSent = false;
    m_mailer.readConfig(APP_CONFIG_FILE_NAME);
}

PowerGate::~PowerGate()
{
    m_alertMgr->stop();
    delete m_alertMgr;
}

void PowerGate::fixTableViews(QWidget *wid)
{
    Q_FOREACH(QTableView* tables, wid->findChildren<QTableView*>())
    {
        QFont tableFont = tables->font();
        tableFont.setFixedPitch(true);
        tables->setFont(tableFont);
        tables->setMinimumWidth(200);
        tables->setMinimumHeight(200);
        tables->verticalHeader()->setDefaultSectionSize(22);
    }
}

void PowerGate::setColumnResizeMode(QTableView *table, int column, QHeaderView::ResizeMode mode)
{
#if QT_VERSION < 0x050000
    table->horizontalHeader()->setResizeMode(column, mode);
#else
    table->horizontalHeader()->setSectionResizeMode(column, mode);
#endif
}

void PowerGate::setColumnResizeMode(QTableView *table, QHeaderView::ResizeMode mode)
{
#if QT_VERSION < 0x050000
    table->horizontalHeader()->setResizeMode(mode);
#else
    table->horizontalHeader()->setSectionResizeMode(mode);
#endif
}

void PowerGate::saveAppLayout()
{
    QSettings settings;
    settings.beginGroup("layout");

    QByteArray geodata = appGlobalData.mainWindow_->saveGeometry();
    settings.setValue("Geometry", geodata);
    settings.setValue("windowState", appGlobalData.mainWindow_->saveState());

    settings.setValue("splitterMain", splitterMain->saveGeometry());
    settings.setValue("splitterMainState", splitterMain->saveState());
    settings.setValue("splitterCenter", splitterCenter->saveGeometry());
    settings.setValue("splitterCenterState", splitterCenter->saveState());
    settings.setValue("splitterBottom", splitterBottom->saveGeometry());
    settings.setValue("splitterBottomState", splitterBottom->saveState());

    settings.endGroup();
}

void PowerGate::restoreAppLayout()
{
    QSettings settings;
    settings.beginGroup("layout");

    QByteArray geodata = settings.value("Geometry").toByteArray();
    appGlobalData.mainWindow_->restoreGeometry(geodata);
    appGlobalData.mainWindow_->restoreState(settings.value("windowState").toByteArray());

    splitterMain->restoreGeometry(settings.value("splitterMain").toByteArray());
    splitterMain->restoreState(settings.value("splitterMainState").toByteArray());

    splitterCenter->restoreGeometry(settings.value("splitterCenter").toByteArray());
    splitterCenter->restoreState(settings.value("splitterCenterState").toByteArray());


    splitterBottom->restoreGeometry(settings.value("splitterBottom").toByteArray());
    splitterBottom->restoreState(settings.value("splitterBottomState").toByteArray());
    
    settings.endGroup();

    ///////
#if 0
    ui.orderLogsTableFrame->hide();
    ui.groupModels->hide();
    ui.positionsTableFrame->hide();
    ui.widgetTop->hide();
#endif

    char log[1024];

    SYSTEMTIME lt;
    GetLocalTime(&lt);
    char datetime[64];
    sprintf(datetime, "%04d/%02d/%02d %02d:%02d:%02d.%03d\r\n", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
    sprintf(log, "DateTime: %s", datetime);

    m_initMailSubject = "Start PowerGate at ";
    m_initMailSubject += datetime;

    writeUILog(LogLevel::LOG_INFO, "Starting up algorithmic strategy engine, please wait....");
}

void PowerGate::moveWidgetsToSplitter()
{
    splitterMain = new QSplitter(Qt::Vertical, this);
    splitterMain->addWidget(ui.widgetTop);
    splitterMain->addWidget(ui.quoteTableFrame);

    splitterCenter = new QSplitter(Qt::Horizontal, splitterMain);
    splitterCenter->addWidget(ui.groupModels);
    splitterCenter->addWidget(ui.orderLogsTableFrame);

    splitterBottom = new QSplitter(Qt::Horizontal, splitterCenter);
    splitterBottom->addWidget(ui.groupLogs);
    splitterBottom->addWidget(ui.positionsTableFrame);
    splitterMain->addWidget(splitterBottom);

    this->setCentralWidget(splitterMain);
}

bool PowerGate::confirmExitApp()
{
    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tr("Are you sure you want to close Application?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    return msgBox.exec() == QMessageBox::Yes;
}

bool PowerGate::writeUILog(int level, const char* msgFmt, ...)
{
    AlgoSE::SystemLog sysLog;
    sysLog.level = level;

    int bufLen = sizeof(sysLog);

    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sysLog.timestamp = DateTime().toTimeStamp();

    int start = 0;

    va_list args;
    va_start(args, msgFmt);
    int len = vsnprintf(sysLog.text + start, bufLen, msgFmt, args);
    va_end(args);

    if (len > 0) {
        updateSystemLog(sysLog);
        return true;
    }

    return false;
}

void PowerGate::startLogin()
{
    LoginDialog dlg(pAlgoSE);
    dlg.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);

    QObject::connect(this, SIGNAL(signalSystemLog(SystemLog)), &dlg, SLOT(recvSystemLog(SystemLog)));

    if (dlg.exec() != QDialog::Accepted) {
        exitApp();
    } else {
        show();
    }
}

void PowerGate::initAlgoSE()
{
    if (!pAlgoSE->initialize(DEFAULT_ENGINE_CONFIG_FILE)) {
        pAlgoSE->release();
        pAlgoSE = nullptr;

        m_initMailLogs.push_back("Initialize engine failed!");
        m_mailer.sendMail(m_initMailSubject.c_str(), m_initMailLogs);

        return;
    }

    pAlgoSE->run();

    rude::Config config;
    vector<string> subscriptions;
    vector<string> strategies;
    if (config.load(APP_CONFIG_FILE_NAME)) {
        config.setSection("Subscriptions");
        int num = config.getNumDataMembers();
        if (num > 0) {
            for (int i = 0; i < num; i++) {
                const char *data = config.getDataNameAt(i);
                if (data != nullptr) {
                    subscriptions.push_back(data);
                }
            }
        }
        
        config.setSection("Strategies");
        num = config.getNumDataMembers();
        if (num > 0) {
            for (int i = 0; i < num; i++) {
                const char *data = config.getDataNameAt(i);
                if (data != nullptr) {
                    strategies.push_back(data);
                }
            }
        }
    }

    if (strategies.size() > 0) {
        m_initMailLogs.push_back("Strategies List:");
    }

    char line[1024];
    for (size_t i = 0; i < strategies.size(); i++) {
        pAlgoSE->loadStrategy(strategies[i].c_str());

        sprintf(line, "%d. %s", i+1, strategies[i].c_str());
        m_initMailLogs.push_back(line);
    }

    int size = subscriptions.size();
    char** instruments = new char*[size];
    for (size_t i = 0; i < size; i++) {
        instruments[i] = (char*)subscriptions[i].data();
    }

    pAlgoSE->getMarketDataAPI()->subscribe((const char**)instruments, size);

    delete[]instruments;

    installSchedTask();

    m_mailer.sendMail(m_initMailSubject.c_str(), m_initMailLogs);
}

void PowerGate::onSystemLog(const SystemLog& log)
{
    emit signalSystemLog(log);
}

void PowerGate::onServiceStatus(const ServiceStatus & state)
{
    emit signalServiceStatus(state);
}

void PowerGate::onAccountDetails(const AccountDetails & account)
{
    emit signalAccountDetails(account);
}

void PowerGate::onAccountPosition(const AccountPosition & position)
{
    emit signalPosition(position);
}

void PowerGate::onStrategyParameter(const StrategyParam & param)
{
    emit signalStratParams(param);
}

void PowerGate::onStrategyPosition(const StrategyPosition & position)
{
    emit signalStratPositions(position);
}

void PowerGate::onStrategyLog(const StrategyLog & log)
{
    emit signalStratLog(log);
}

void PowerGate::onTick(const Tick& tick)
{
    emit signalQuote(tick);
}

void PowerGate::onOrderStatus(const Order& order)
{
    emit signalOrderStatus(order);
}

void PowerGate::onExecutionReport(const Execution& e)
{
    emit signalExecutionReport(e);
}

void PowerGate::onStrategyStatus(const StrategyStatus& state)
{
    emit signalStratStatus(state);
}

void PowerGate::updateSystemLog(const AlgoSE::SystemLog log)
{
    if (!verboseLog) {
        // filter out trading log.
        const char *p = log.text;
        const char* feature1 = "] Algo";
        const char* feature2 = "] Raw";
        if (strstr(p, feature1) || strstr(p, feature2)) {
            return;
        }
    }

    logModel->insertLogItem(log);
}

void PowerGate::updateServiceStatus(const AlgoSE::ServiceStatus state)
{
    int type = state.type;
    int stateCode = state.state;
    
    QLabel* label = nullptr;
    bool found = false;
    int srvIdx = -1;
    for (size_t i = 0; i < srvStatus.size(); i++) {
        if (srvStatus[i].status.type == state.type && 
            srvStatus[i].status.srvId == state.srvId) {
            found = true;
            label = srvStatus[i].label;
            srvIdx = i;
            break;
        }
    }


    if (!found) {
        SrvStatus srvSt;
        srvSt.status = state;
        srvSt.label = new ClickableLabel("", statusBar());
        srvSt.alertId = m_alertMgr->generateAlertId();
        SrvStLabelCtxMenu* menu = new SrvStLabelCtxMenu(state.type, state.srvId, srvSt.label);
        QAction* act = new QAction("Reinitialize", menu);
        menu->addAction(act);
        srvSt.label->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(srvSt.label, SIGNAL(customContextMenuRequested(QPoint)), menu, SLOT(showMenu(QPoint)));
        connect(act, SIGNAL(triggered()), this, SLOT(reloadService()));

        statusBar()->addPermanentWidget(srvSt.label);
        label = srvSt.label;
        srvStatus.push_back(srvSt);
        srvIdx = srvStatus.size() - 1;
    }

    QObject::connect(srvStatus[srvIdx].label, &ClickableLabel::clicked, [this, srvIdx]()
    {
        int alertId = srvStatus[srvIdx].alertId;
        m_alertMgr->cancelAlert(alertId);
    });

    if (type == ServiceType::SERVICE_MARKET_DATA) {
        switch (stateCode) {
        case MarketDataServiceState::CONNECTED:
        case MarketDataServiceState::INITIALIZE_DONE: {
            QPixmap pixmap(QPixmap(":/icon/connect.png").scaledToHeight(16));
            label->setPixmap(pixmap);

            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("MarketData");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("MD connected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);

            m_alertMgr->cancelAlert(srvStatus[srvIdx].alertId);

            break;
        }
        case MarketDataServiceState::LOADED:
        case MarketDataServiceState::CONNECT_ERROR:
        case MarketDataServiceState::DISCONNECTED: {
            QPixmap pixmap(QPixmap(":/icon/disconnect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("MarketData");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("MD disconnected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);
            if (found && (stateCode == MarketDataServiceState::CONNECT_ERROR || stateCode == MarketDataServiceState::DISCONNECTED)) {
                m_alertMgr->addAlert(srvStatus[srvIdx].alertId);
            }

            break;
        }
        }
    } else if (type == ServiceType::SERVICE_ORDER_EXECUTION) {
        switch (stateCode) {
        case ExecutionServiceState::CONNECTED: 
        case ExecutionServiceState::INITIALIZE_DONE:
        case ExecutionServiceState::READY: {
            QPixmap pixmap(QPixmap(":/icon/connect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("Execution");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("Trade connected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);

            m_alertMgr->cancelAlert(srvStatus[srvIdx].alertId);

            break;
        }
        case ExecutionServiceState::LOADED:
        case ExecutionServiceState::CONNECT_ERROR:
        case ExecutionServiceState::DISCONNECTED: {
            QPixmap pixmap(QPixmap(":/icon/disconnect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("Execution");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("Trade disconnected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);
            
            if (found && (stateCode == ExecutionServiceState::CONNECT_ERROR || stateCode == ExecutionServiceState::DISCONNECTED)) {
                m_alertMgr->addAlert(srvStatus[srvIdx].alertId);
            }

            break;
        }
        }
    } else if (type == ServiceType::SERVICE_HISTORICAL_DATA) {
        switch (stateCode) {
        case HistoricalDataServiceState::CONNECTED:
        case HistoricalDataServiceState::INITIALIZE_DONE: {
            QPixmap pixmap(QPixmap(":/icon/connect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("HistoricalData");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("HistData connected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);

            m_alertMgr->cancelAlert(srvStatus[srvIdx].alertId);

            break;
        }
        case HistoricalDataServiceState::LOADED:
        case HistoricalDataServiceState::CONNECT_ERROR:
        case HistoricalDataServiceState::DISCONNECTED: {
            QPixmap pixmap(QPixmap(":/icon/disconnect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("HistoricalData");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("HistData disconnected");
            }
            tip += "</table></html>";
            label->setToolTip(tip);

            if (found && (stateCode == HistoricalDataServiceState::CONNECT_ERROR || stateCode == HistoricalDataServiceState::DISCONNECTED)) {
                m_alertMgr->addAlert(srvStatus[srvIdx].alertId);
            }

            break;
        }
        }
    } else if (type == ServiceType::SERVICE_TIMER) {
        switch (stateCode) {
        case TimerServiceState::LOADED: {
            QPixmap pixmap(QPixmap(":/icon/connect.png").scaledToHeight(16));
            label->setPixmap(pixmap);
            QString tip;
            tip += "<html><table>";
            tip += QString("<tr><td>[Type]</td><td>: %1</td></tr>").arg("Timer");
            tip += QString("<tr><td>[ID]</td><td>: %1</td></tr>").arg(state.srvId);
            tip += QString("<tr><td>[Name]</td><td>: %1</td></tr>").arg(state.name);
            if (state.desc[0] != '\0') {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg(QString::fromLocal8Bit(state.desc));
            } else {
                tip += QString("<tr><td>[Status]</td><td>: %1</td></tr>").arg("Timer service loaded");
            }
            tip += "</table></html>";
            label->setToolTip(tip);
            break;
        }
        }
    }
}

void PowerGate::updateAccountDetails(const AlgoSE::AccountDetails acct)
{
    acctDetails = acct;

    if (!titleShowed) {
        setWindowTitle(QString("PowerGate-") + acct.userId);
        titleShowed = true;
    }

    char str[128];
    ui.accountID->setText(acct.userId);
    
    sprintf(str, "%.02f", acct.equity);
    ui.balance->setText(str);

    sprintf(str, "%.02f", acct.realizedPnL);
    ui.realizedPnL->setText(str);

    sprintf(str, "%.02f", acct.unrealizedPnL);
    ui.mtmPnL->setText(str);

    sprintf(str, "%.02f", acct.currMargin);
    ui.margin->setText(str);

    sprintf(str, "%.02f", acct.availFunds);
    ui.fundavail->setText(str);

    sprintf(str, "%.01f%%", acct.riskLevel * 100);
    ui.riskdegree->setText(str);
}

void PowerGate::updateStratParams(const AlgoSE::StrategyParam param)
{
    if (param.strategyId <= 0) {
        return;
    }

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (param.strategyId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    vector<StrategyParam>& params = strategies[i].params;
    for (i = 0; i < params.size(); i++) {
        if (param.name == params[i].name && param.category == params[i].category) {
            params[i] = param;
            break;
        }
    }
    if (i == params.size()) {
        params.push_back(param);
    }
}

void PowerGate::updateStratPositions(const AlgoSE::StrategyPosition pos)
{
    if (pos.strategyId <= 0) {
        return;
    }

    stratModel->updateStratPositions(pos);

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (pos.strategyId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    vector<StrategyPosition>& positions = strategies[i].pos;
    size_t j = 0;
    for (j = 0; j < positions.size(); j++) {
        if (!strcmp(pos.instrument, positions[j].instrument) &&
            pos.side == positions[j].side) {
            positions[j] = pos;
            break;
        }
    }

    if (j == positions.size()) {
        positions.push_back(pos);
    }

    for (auto it = positions.begin(); it != positions.end();) {
        if (it->quantity == 0) {
            it = positions.erase(it);
        } else {
            it++;
        }
    }
}

void PowerGate::updateStratLog(AlgoSE::StrategyLog log)
{
    if (log.level == LogLevel::LOG_ERROR || log.level == LogLevel::LOG_FATAL) {
        stratModel->updateStratLog(log);
        for (size_t i = 0; i < strategies.size(); i++) {
            if (strategies[i].id == log.strategyId) {
                m_alertMgr->addAlert(strategies[i].alertId);
            }
        }
    }

    int stratId = log.strategyId;
    if (stratId >= 0) {
        auto it = strategiesLog.find(stratId);
        if (it == strategiesLog.end()) {
            it = strategiesLog.insert(std::make_pair(stratId, std::vector<StrategyLog>())).first;
        }

        it->second.push_back(log);
    }

    // write strategy's normal log to system log window.
    if (log.level != LogLevel::LOG_ERROR && log.level != LogLevel::LOG_FATAL) {
        SystemLog sysLog;
        sysLog.timestamp = log.timestamp;
        sysLog.level = log.level;
        strcpy(sysLog.text, log.text);
        updateSystemLog(sysLog);
    }
}

void PowerGate::updateOrderStatus(AlgoSE::Order order)
{
    orderModel->updateOrderStatus(order);
}

void PowerGate::updateStratStatus(AlgoSE::StrategyStatus state)
{
    if (state.strategyId <= 0) {
        return;
    }

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (state.strategyId == strategies[i].id) {
            strategies[i].status = state;
            break;
        }
    }

    if (i == strategies.size()) {
        StratItem item;
        item.id = state.strategyId;
        item.status = state;
        item.params.clear();
        item.alertId = m_alertMgr->generateAlertId();
        strategies.push_back(item);
    }

    if (state.state == Strategy::UNLOADED) {
        m_alertMgr->cancelAlert(strategies[i].alertId);
    }
        
    stratModel->updateStratStatus(state);
    orderModel->updateStratStatus(state);
}

void PowerGate::updateExecution(AlgoSE::Execution exec)
{
    orderModel->updateExecution(exec);
}

void PowerGate::updatePosition(AlgoSE::AccountPosition pos)
{
    posModel->updatePosition(pos);
}

void PowerGate::updateQuoteDialogs(const Tick& tick)
{
    lastQuotes[tick.instrument] = tick;

    auto& it = depthQuoteDialogs.find(tick.instrument);
    if (it != depthQuoteDialogs.end()) {
        QuoteDialog* dlg = it->second;
        if (dlg) {
            if (dlg->getInstrument() == tick.instrument) {
                dlg->notifyNewQuote(tick);
            }
        }
    }
}

void PowerGate::updateQuote(Tick tick)
{
    if (tick.type == TickType::SNAPSHOT) {
        quoteModel->notifyNewQuote(tick);
    } else if (tick.type == TickType::TRADES) {
        auto& it = lastTrades.find(tick.instrument);
        if (it == lastTrades.end()) {
            circular_buffer<Tick> trades(MAX_TRADE_ITEM_SIZE);
            it = lastTrades.insert(std::make_pair(tick.instrument, trades)).first;
        }
        it->second.push_back(tick);
    }

    if (tradePanel && tradePanel->isVisible()) {
        tradePanel->updateMarketData(tick);
    }

    updateQuoteDialogs(tick);
}

void PowerGate::reloadService()
{
    SrvStLabelCtxMenu* menu = (SrvStLabelCtxMenu*)((QAction*)sender())->parentWidget();
    if (menu) {
        int srvType = menu->getSrvType();
        int srvId = menu->getSrvId();

        SrvStatus* st = nullptr;
        for (auto& srvSt : srvStatus) {
            if (srvSt.status.type == srvType && srvSt.status.srvId == srvId) {
                st = &srvSt;
                break;
            }
        }

        if (st) {
            QString strType;
            bool allowed = false;
            if (srvType == ServiceType::SERVICE_MARKET_DATA) {
                strType = "Market Data Service";
                if (st->status.state == MarketDataServiceState::DISCONNECTED ||
                    st->status.state == MarketDataServiceState::CONNECT_ERROR) {
                    allowed = true;
                }
            } else if (srvType == ServiceType::SERVICE_ORDER_EXECUTION) {
                strType = "Execution Service";
                if (st->status.state == ExecutionServiceState::DISCONNECTED ||
                    st->status.state == ExecutionServiceState::CONNECT_ERROR) {
                    allowed = true;
                }
            } else if (srvType == ServiceType::SERVICE_HISTORICAL_DATA) {
                strType = "Historical Data Service";
            }

            QMessageBox msgBox(windowWidget);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("Are you sure you want to reinitialize %1 '%2'?").arg(strType, st->status.name));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
            msgBox.setButtonText(QMessageBox::Yes, "Yes");
            msgBox.setButtonText(QMessageBox::No, "No");
            if (msgBox.exec() == QMessageBox::Yes && allowed) {
                pAlgoSE->reinitializeService(srvType, srvId);
            }
        }
    }
}

void PowerGate::exportOrderList()
{
    QString dt = QDate::currentDate().toString("yyyyMMdd");
    dt += "_";
    dt += QTime::currentTime().toString("hhmmss");

    QString filename = QFileDialog::getSaveFileName(this, tr("Save to CSV"), QString(tr("orders_")) + dt + ".csv",
        "Comma Separated Values Spreadsheet (*.csv);;"
        "All Files (*)");

    if (!filename.isEmpty()) {
        QString textData;
        int rows = orderModel->rowCount();
        int columns = orderModel->columnCount();
        int headerNum = orderModel->getHeaderNumber();

        const QStringList& headers = orderModel->getHeaderLabels();
        for (int i = 0; i < headers.size() && i < headerNum; i++) {
            textData += headers[i];
            if (i < headers.size() - 1 && i < headerNum - 1) {
                textData += ",";
            }
        }
        textData += "\n";

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < columns && j < headerNum; j++) {
                textData += orderModel->data(orderModel->index(i, j)).toString();
                if (j < columns - 1 && j < headerNum - 1) {
                    textData += ",";
                }
            }
            textData += "\n";
        }

        QFile csvFile(filename);
        if (csvFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&csvFile);
            out << textData;

            csvFile.close();
        }
    }
}

void PowerGate::subscribeQuote()
{
    if (!subscribeDlgShowed) {
        SubscribeDialog dialog(this);
        subscribeDlgShowed = true;
        if (dialog.exec() == QDialog::Accepted) {
            string instrument = dialog.getInstrument().toStdString();
            if (!instrument.empty()) {
                pAlgoSE->getMarketDataAPI()->subscribe(instrument.c_str());
            }
        }
        subscribeDlgShowed = false;
        return;
    }
}

void PowerGate::unsubscribeQuote()
{
}

void PowerGate::popupDepthQuoteDialog()
{
    QModelIndex idx = quoteModel->index(ui.quoteTableView->currentIndex().row(), 0);
    if (idx.isValid()) {
        string instrument = idx.model()->data(idx).toString().toStdString();

        if (!instrument.empty()) {
            showDepthQuoteDialog(instrument);
        }
    }
}

void PowerGate::popupTradePanelFromQuoteTableView()
{
    QModelIndex idx = quoteModel->index(ui.quoteTableView->currentIndex().row(), 0);
    if (idx.isValid()) {
        string instrument = idx.model()->data(idx).toString().toStdString();
        if (!instrument.empty()) {
            PositionItem item;
            if (posModel->getPosItem(instrument.c_str(), item)) {
                showTradePanel(instrument.c_str(), item.side, item.closable);
            } else {
                showTradePanel(instrument.c_str(), PositionSide::POSITION_SIDE_LONG, 0);
            }
        }
    } else {
        showTradePanel("", PositionSide::POSITION_SIDE_LONG, 0);
    }
}

void PowerGate::sendReportMail()
{
    std::vector<std::string> content;
    char line[1024] = { 0 };
    char* splitLine = "===========================================================";


    content.push_back("Account Details:");
    content.push_back(splitLine);
    content.push_back("Account, Equity, RealizedPnL, UnRealizedPnL, Margin, Risk");
    sprintf(line, "%s, %.2f, %.2f, %.2f, %.2f, %.2f",
        acctDetails.userId, acctDetails.equity, acctDetails.realizedPnL, acctDetails.unrealizedPnL, acctDetails.currMargin, acctDetails.riskLevel);
    content.push_back(line);
    content.push_back(splitLine);

    content.push_back("\r\nStrategies List:");
    std::vector<StratModel::StratItem> strategies;
    stratModel->getAllStratItems(strategies);
    if (strategies.size() > 0) {
        content.push_back(splitLine);
        content.push_back("ID, Strategy, Status, Submittion, Cancellation, Long, Short");
        for (auto& item : strategies) {
            int posLong = 0;
            for (auto& pos : item.posLongList) {
                posLong += pos.size;
            }

            int posShort = 0;
            for (auto& pos : item.posShortList) {
                posShort += pos.size;
            }
            sprintf(line, "%d, %s, %s, %d, %d, %d, %d",
                item.strategyId, item.name, 
                item.status == Strategy::STOPPED ?  "Stop" : "Start",
                item.submittion, item.cancellation, posLong, posShort);

            content.push_back(line);
        }
        content.push_back(splitLine);
    }

    content.push_back("\r\nOrder List:");
    std::vector<OrderStatusItem> orders;
    orderModel->getAllOrderItems(orders);
    if (orders.size() > 0) {
        content.push_back(splitLine);
        content.push_back("ID, Strategy, Instrument, Side, Action, Status, Price, TradePrice, Qty, TradeQty, DateTime");
        for (auto& item : orders) {
            char *status;
            switch (item.status) {
            case OrderState::ACCEPTED:
                status = ("Accepted");
                break;
            case OrderState::CANCELED:
                status = ("Canceled");
                break;
            case OrderState::FILLED:
                status = ("Filled");
                break;
            case OrderState::PARTIALLY_FILLED:
                status = ("Part_Filled");
                break;
            case OrderState::PENDING_CANCEL:
                status = ("Pending_Cancel");
                break;
            case OrderState::PENDING_NEW:
                status = ("Pending_New");
                break;
            case OrderState::REJECTED:
                status = ("Rejected");
                break;
            case OrderState::SUBMITTED:
                status = ("Submitted");
                break;
            default:
            case OrderState::UNKNOWN:
                status = ("Unknown");
                break;
            }

            sprintf(line, "%s, %s, %s, %s, %s, %s, %.2f, %.2f, %d, %d, %s",
                item.ordId, item.stratName, item.instrument,
                (item.action == OrderAction::BUY || item.action == OrderAction::BUY_TO_COVER) ? "Buy" : "Sell",
                (item.action == OrderAction::BUY || item.action == OrderAction::SELL_SHORT) ? ("Open") :
                ((item.closeEffect == CloseEffect::CLOSE_YESTERDAY) ? ("CloseYd") :
                (item.closeEffect == CloseEffect::CLOSE_TODAY) ? ("CloseToday") : ("Close")),
                status, item.price, item.avgPrice, item.quantity, item.traded, item.datetime);
            content.push_back(line);
        }
        content.push_back(splitLine);
    }

    content.push_back("\r\nPosition List:");
    std::vector<PositionItem> posItems;
    posModel->getAllPosItems(posItems);
    if (posItems.size() > 0) {
        content.push_back(splitLine);
        content.push_back("SrvID, Instrument, Side, Price, Total, Yesterday, Today, Closable, PnL");

        for (auto& item : posItems) {
            sprintf(line, "%d, %s, %s, %.2f, %d, %d, %d, %d, %.2f",
                item.srvId, item.instrument, 
                item.side == PositionSide::POSITION_SIDE_LONG ? "Buy" : "Sell",
                item.price, item.total, item.yesterday, item.today, item.closable, item.posPnL);
            content.push_back(line);
        }
        content.push_back(splitLine);
    }

    char subject[1024] = { 0 };
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sprintf(subject, "Report PowerGate at %04d/%02d/%02d %02d:%02d:%02d.%03d\r\n", 
        lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);

    m_mailer.sendMail(subject, content);
}

void PowerGate::closeApp()
{
    if (confirmExitApp()) {
        exitApp();
    }
}

void PowerGate::exitApp()
{
    if (pAlgoSE) {
        pAlgoSE->stop();
        pAlgoSE->release();
    }

    saveAppLayout();

    if (!m_reportMailSent) {
        sendReportMail();
        m_reportMailSent = true;
    }

    QCoreApplication::quit();
}

void PowerGate::onParamCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();
    idx = stratModel->index(index.row(), 1);
    QString stratName = index.model()->data(idx).toString();

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    vector<StrategyParam>& params = strategies[i].params;
    vector<StrategyParam> touchedParams;
    StratParamsDialog* dialog = new StratParamsDialog(stratName, params, touchedParams, this);
    int retCode = dialog->exec();
    if (retCode == QDialog::Accepted) {
        int size = touchedParams.size();
        for (int i = 0; i < size; i++) {
            if (i == size - 1) {
                touchedParams[i].isLast = true;
            } else {
                touchedParams[i].isLast = false;
            }

            // Write back touched params.
            for (int j = 0; j < params.size(); j++) {
                if (!strcmp(params[j].name, touchedParams[i].name) &&
                    params[j].category == touchedParams[i].category &&
                    params[j].type == touchedParams[i].type) {
                    params[j] = touchedParams[i];
                }
            }
            pAlgoSE->setStrategyParameter(touchedParams[i].strategyId, touchedParams[i]);
        }
    }
}

void PowerGate::onStartCellClicked(const QModelIndex& index)
{
    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);

    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    QString stratName = strategies[i].status.name;
    QString msg;
    bool stopStrat = false;
    if (strategies[i].status.state == Strategy::STOPPED) {
        msg = QString(tr("Do you want to start strategy '%1'?")).arg(stratName);
    } else {
        msg = QString(tr("Do you want to stop strategy '%1'?")).arg(stratName);
        stopStrat = true;
    }

    msgBox.setText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() == QMessageBox::Yes) {
        if (stopStrat) {
            pAlgoSE->stopStrategy(stratId);
        } else {
            pAlgoSE->launchStrategy(stratId);
        }
    }
}

void PowerGate::onPauseCellClicked(const QModelIndex& index)
{
    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);

    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    QString stratName = strategies[i].status.name;
    QString msg;
    bool pauseStrat = false;
    if (strategies[i].status.state != Strategy::PAUSED) {
        msg = QString(tr("Do you want to pause strategy '%1'?")).arg(stratName);
        pauseStrat = true;
    } else {
        msg = QString(tr("Do you want to resume strategy '%1'?")).arg(stratName);
    }

    msgBox.setText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() == QMessageBox::Yes) {
        if (pauseStrat) {
            pAlgoSE->pauseStrategy(stratId);
        } else {
            pAlgoSE->resumeStrategy(stratId);
        }
    }
}

void PowerGate::onLongPosCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();
    idx = stratModel->index(index.row(), 1);
    QString stratName = index.model()->data(idx).toString();

    idx = stratModel->index(index.row(), 7);

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    vector<StrategyPosition>& position = strategies[i].pos;
    vector<StrategyPosition> longPos;
    for (size_t j = 0; j < position.size(); j++) {
        if (position[j].side == PositionSide::POSITION_SIDE_LONG) {
            longPos.push_back(position[j]);
        }
    }

    vector<StrategyPosition> origLongPos = longPos;
    StratPosDialog dialog(stratId, longPos, this);
    dialog.setWindowTitle(QString(tr("Long Position (") + stratName + ")"));
    if (dialog.exec() == QDialog::Accepted) {
        for (size_t j = 0; j < longPos.size(); j++) {
            pAlgoSE->assignStrategyPosition(
                stratId, 
                longPos[j].instrument, 
                PositionSide::POSITION_SIDE_LONG,
                longPos[j].quantity, 
                longPos[j].avgPx);
        }

        vector<StrategyPosition> removedPos;
        for (size_t i = 0; i < origLongPos.size(); i++) {
            size_t j = 0;
            for (; j < longPos.size(); j++) {
                if (!_stricmp(origLongPos[i].instrument, longPos[j].instrument)) {
                    break;
                }
            }
            if (j == longPos.size()) {
                removedPos.push_back(origLongPos[i]);
            }
        }

        for (size_t j = 0; j < removedPos.size(); j++) {
            pAlgoSE->assignStrategyPosition(
                stratId,
                removedPos[j].instrument,
                PositionSide::POSITION_SIDE_LONG,
                0,
                0);
        }
    }
    
    return;
}

void PowerGate::onShortPosCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();
    idx = stratModel->index(index.row(), 1);
    QString stratName = index.model()->data(idx).toString();

    idx = stratModel->index(index.row(), 8);

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    vector<StrategyPosition>& position = strategies[i].pos;
    vector<StrategyPosition> shortPos;
    for (size_t j = 0; j < position.size(); j++) {
        if (position[j].side == PositionSide::POSITION_SIDE_SHORT) {
            shortPos.push_back(position[j]);
        }
    }

    vector<StrategyPosition> origShortPos = shortPos;
    StratPosDialog dialog(stratId, shortPos, this);
    dialog.setWindowTitle(QString(tr("Short Position (") + stratName + ")"));
    if (dialog.exec() == QDialog::Accepted) {
        for (size_t j = 0; j < shortPos.size(); j++) {
            pAlgoSE->assignStrategyPosition(
                stratId, 
                shortPos[j].instrument, 
                PositionSide::POSITION_SIDE_SHORT,
                shortPos[j].quantity, 
                shortPos[j].avgPx);
        }

        vector<StrategyPosition> removedPos;
        for (size_t i = 0; i < origShortPos.size(); i++) {
            size_t j = 0;
            for (; j < shortPos.size(); j++) {
                if (!_stricmp(origShortPos[i].instrument, shortPos[j].instrument)) {
                    break;
                }
            }
            if (j == shortPos.size()) {
                removedPos.push_back(origShortPos[i]);
            }
        }

        for (size_t j = 0; j < removedPos.size(); j++) {
            pAlgoSE->assignStrategyPosition(
                stratId,
                removedPos[j].instrument,
                PositionSide::POSITION_SIDE_SHORT,
                0,
                0);
        }
    }

    return;
}

void PowerGate::onShowLogCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();
    idx = stratModel->index(index.row(), 1);
    QString stratName = index.model()->data(idx).toString();

    size_t i = 0;
    for (i = 0; i < strategies.size(); i++) {
        if (stratId == strategies[i].id) {
            break;
        }
    }

    if (i == strategies.size()) {
        return;
    }

    bool show = strategies[i].status.verbose;
    if (!show) {
        pAlgoSE->showVerboseMsg(strategies[i].id);
    } else {
        pAlgoSE->hideVerboseMsg(strategies[i].id);
    }

#if 0
    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);
    if (!show) {
        msgBox.setText(QString("Do you want to show verbose message of strategy '%1'?").arg(stratName));
    } else {
        msgBox.setText(QString("Do you want to hide verbose message of strategy '%1'?").arg(stratName));
    }
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, "Yes");
    msgBox.setButtonText(QMessageBox::No, "No");
    if (msgBox.exec() == QMessageBox::Yes) {
        if (!show) {
            engineInstance->showVerboseMsg(strategies[i].id);
        } else {
            engineInstance->hideVerboseMsg(strategies[i].id);
        }
    }
#endif
}

void PowerGate::onStratCmdCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();

    StratCmdDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        string cmd = dialog.getCommand().toStdString();
        if (!cmd.empty()) {
            pAlgoSE->sendStrategyCommand(stratId, cmd.c_str());
        }
    }
}

void PowerGate::onStratLogCellClicked(const QModelIndex& index)
{
    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();

    stratModel->clearRedAlert(stratId);

    for (const auto& strat : strategies) {
        if (strat.id == stratId) {
            m_alertMgr->cancelAlert(strat.alertId);
        }
    }
}

void PowerGate::onStratLogDlgExit(int stratId)
{
    auto& it = stratLogDlgs.find(stratId);
    if (it != stratLogDlgs.end()) {
        it->second->close();
        it->second = nullptr;
    }
}

void PowerGate::onStratTableViewClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    if (index.column() == 2) {
        return onParamCellClicked(index);
    } else if (index.column() == 3) {
        return onStartCellClicked(index);
    } else if (index.column() == 4) {
        return onPauseCellClicked(index);
    } else if (index.column() == 7) {
        return onLongPosCellClicked(index);
    } else if (index.column() == 8) {
        return onShortPosCellClicked(index);
    } else if (index.column() == 9) {
        return onShowLogCellClicked(index);
    } else if (index.column() == 10) {
        return onStratCmdCellClicked(index);
    } else if (index.column() == 11) {
        return onStratLogCellClicked(index);
    }
}

void PowerGate::onStratTableViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    if (index.column() != 11) {
        return;
    }

    QModelIndex idx = stratModel->index(index.row(), 0);
    int stratId = index.model()->data(idx).toInt();
    idx = stratModel->index(index.row(), 1);
    QString stratName = index.model()->data(idx).toString();

    showStratLogDlg(stratId, stratName);
}

void PowerGate::onStartAllStrategiesBtnClicked()
{
    if (strategies.size() == 0) {
        return;
    }

    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tr("Are you sure you want to start all strategies?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() == QMessageBox::Yes) {
        size_t i = 0;
        for (i = 0; i < strategies.size(); i++) {
            pAlgoSE->launchStrategy(strategies[i].id);
        }
    }
}

void PowerGate::onStopAllStrategiesBtnClicked()
{
    if (strategies.size() == 0) {
        return;
    }

    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tr("Are you sure you want to stop all strategies?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() == QMessageBox::Yes) {
        size_t i = 0;
        for (i = 0; i < strategies.size(); i++) {
            pAlgoSE->stopStrategy(strategies[i].id);
        }
    }
}

void PowerGate::onTradePanelBtnClicked()
{
    QModelIndex idx = quoteModel->index(ui.quoteTableView->currentIndex().row(), 0);
    if (!idx.isValid()) {
        idx = quoteModel->index(0, 0);
    }

    if (idx.isValid()) {
        string instrument = idx.model()->data(idx).toString().toStdString();
        if (!instrument.empty()) {
            PositionItem item;
            if (posModel->getPosItem(instrument.c_str(), item)) {
                showTradePanel(instrument.c_str(), item.side, item.closable);
            } else {
                showTradePanel(instrument.c_str(), PositionSide::POSITION_SIDE_LONG, 0);
            }
        }
    } else {
        showTradePanel("", PositionSide::POSITION_SIDE_LONG, 0);
    }
}

void PowerGate::onCancelAllOrdersBtnClicked()
{
    std::vector<OrderStatusItem> orders;
    orderModel->getAllPendingOrders(orders);

    if (orders.size() > 0) {
        QMessageBox msgBox(windowWidget);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Are you sure you want to cancel all orders?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
        if (msgBox.exec() == QMessageBox::Yes) {
            for (auto& ord : orders) {
                Order order = { 0 };
                strncpy(order.clOrdId, ord.clOrdId, sizeof(order.clOrdId) - 1);
                strncpy(order.ordId, ord.ordId, sizeof(order.ordId) - 1);
                pAlgoSE->getTradingAPI()->cancelOrder(order);
            }
        }
    }
}

void PowerGate::onCloseAllPosBtnClicked()
{
    std::vector<PositionItem> items;
    posModel->getAllPosItems(items);

    if (items.size() > 0) {
        QMessageBox msgBox(windowWidget);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Are you sure you want to close all positions?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
        if (msgBox.exec() == QMessageBox::Yes) {
            pAlgoSE->getTradingAPI()->closeAllPos();
        }
    }
}

void PowerGate::onVerboseLogCBClicked(bool toggled)
{
    verboseLog = toggled;
}

void PowerGate::onQuoteTableViewDoubleClicked(const QModelIndex &index)
{
    QModelIndex idx = quoteModel->index(ui.quoteTableView->currentIndex().row(), 0);
    if (idx.isValid()) {
        string instrument = idx.model()->data(idx).toString().toStdString();
        if (!instrument.empty()) {
            PositionItem item;
            if (posModel->getPosItem(instrument.c_str(), item)) {
                showTradePanel(instrument.c_str(), item.side, item.closable);
            } else {
                showTradePanel(instrument.c_str(), PositionSide::POSITION_SIDE_LONG, 0);
            }
        }
    } else {
        showTradePanel("", PositionSide::POSITION_SIDE_LONG, 0);
    }
}

void PowerGate::onPosTableViewDoubleClicked(const QModelIndex &index)
{
    PositionItem item = { 0 };
    if (posModel->getPosItem(index.row(), item)) {
        showTradePanel(item.instrument, item.side, item.closable);
    }
}

void PowerGate::onOrderTableViewDoubleClicked(const QModelIndex &index)
{
    OrderStatusItem item = { 0 };
    if (!orderModel->getPendingOrder(index.row(), item)) {
        return;
    }

    QMessageBox msgBox(windowWidget);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(tr("Are you sure you want to cancel this order?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    if (msgBox.exec() == QMessageBox::Yes) {
        Order order = { 0 };
        strncpy(order.clOrdId, item.clOrdId, sizeof(order.clOrdId) - 1);
        strncpy(order.ordId, item.ordId, sizeof(order.ordId) - 1);
        pAlgoSE->getTradingAPI()->cancelOrder(order);
    }
}

void PowerGate::onOrderTableViewMenuRequested(const QPoint& pos)
{
    QMenu *menu = new QMenu(this);
    QAction* act = new QAction(tr("Export"), this);
    menu->addAction(act);
    connect(act, SIGNAL(triggered()), this, SLOT(exportOrderList()));

    menu->popup(ui.orderTableView->viewport()->mapToGlobal(pos));
}

void PowerGate::onQuoteTableViewMenuRequested(QPoint pos)
{
    QModelIndex index = ui.quoteTableView->indexAt(pos);

    QMenu *menu = new QMenu(this);
    QAction* subAct = new QAction(tr("Subscribe"), this);
    menu->addAction(subAct);
    connect(subAct, SIGNAL(triggered()), this, SLOT(subscribeQuote()));

    QAction* unsubAct = new QAction(tr("Unsubscribe"), this);
    menu->addAction(unsubAct);
    connect(unsubAct, SIGNAL(triggered()), this, SLOT(unsubscribeQuote()));

    menu->addSeparator();

    QAction* depthAct = new QAction(tr("Depth quote"), this);
    menu->addAction(depthAct);
    connect(depthAct, SIGNAL(triggered()), this, SLOT(popupDepthQuoteDialog()));

    menu->addSeparator();

    QAction* tpAct = new QAction(tr("Trade Panel"), this);
    menu->addAction(tpAct);
    connect(tpAct, SIGNAL(triggered()), this, SLOT(popupTradePanelFromQuoteTableView()));

    menu->popup(ui.quoteTableView->viewport()->mapToGlobal(pos));
}

void PowerGate::onQuoteDlgExit(const string& instrumet)
{
    auto& it = depthQuoteDialogs.find(instrumet);
    if (it != depthQuoteDialogs.end()) {
        it->second->close();
        it->second = nullptr;
    }
}

void PowerGate::showDepthQuoteDialog(const string& instrument)
{
    auto& itor = depthQuoteDialogs.find(instrument);
    QuoteDialog* dlg = nullptr;
    if (itor == depthQuoteDialogs.end()) {
        dlg = new QuoteDialog(instrument, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(dlg, SIGNAL(closeEvt(const string&)), this, SLOT(onQuoteDlgExit(const string&)));
        depthQuoteDialogs.insert(std::make_pair(instrument, dlg));
    } else {
        dlg = itor->second;
        if (dlg == nullptr) {
            dlg = new QuoteDialog(instrument, this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dlg, SIGNAL(closeEvt(const string&)), this, SLOT(onQuoteDlgExit(const string&)));
            itor->second = dlg;
        }
    }

    auto& it = lastQuotes.find(instrument);
    if (it != lastQuotes.end()) {
        Tick& tick = it->second;
        dlg->notifyNewQuote(tick);
    }

    auto& tradesIt = lastTrades.find(instrument);
    if (tradesIt != lastTrades.end()) {
        circular_buffer<Tick>& trades = tradesIt->second;
        for (size_t i = 0; i < trades.size(); i++) {
            dlg->notifyNewQuote(trades[i]);
        }
    }
    
    dlg->show();
    dlg->raise();
}

void PowerGate::showStratLogDlg(int stratId, QString& stratName)
{
    auto& it = stratLogDlgs.find(stratId);

    StratLogDialog* dlg = nullptr;
    if (it == stratLogDlgs.end()) {
        dlg = new StratLogDialog(stratId, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(dlg, SIGNAL(closeEvt(int)), this, SLOT(onStratLogDlgExit(int)));
        stratLogDlgs.insert(std::make_pair(stratId, dlg));
    }
    else {
        dlg = it->second;
        if (dlg == nullptr) {
            dlg = new StratLogDialog(stratId, this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dlg, SIGNAL(closeEvt(int)), this, SLOT(onStratLogDlgExit(int)));
            it->second = dlg;
        }
    }

    dlg->clear();
    auto& itor = strategiesLog.find(stratId);
    if (itor != strategiesLog.end()) {
        vector<StrategyLog>& logs = itor->second;
        for (size_t i = 0; i < logs.size(); i++) {
            dlg->appendLog(logs[i]);
        }
    }

    dlg->setWindowTitle(stratName);
    dlg->show();
    dlg->raise();
}

void PowerGate::showTradePanel(const string& instrument, int side, int quantity)
{
    if (tradePanel == nullptr) {
        tradePanel = new TradePanel(pAlgoSE, this);
    }

    tradePanel->setPosition(instrument.c_str(), side, quantity);

    tradePanel->show();
}

bool PowerGate::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        bool digit = key >= Qt::Key_0 && key <= Qt::Key_9;
        if (key == Qt::Key_F3 ) { //|| digit) {
            if (!subscribeDlgShowed) {
                SubscribeDialog dialog(this);
//                if (digit) {
//                    dialog.setInstrument(QString(key));
//                }
                subscribeDlgShowed = true;
                if (dialog.exec() == QDialog::Accepted) {
                    string instrument = dialog.getInstrument().toStdString();
                    if (!instrument.empty()) {
                        pAlgoSE->getMarketDataAPI()->subscribe(instrument.c_str());
                    }

                    showDepthQuoteDialog(instrument);
                }
                subscribeDlgShowed = false;
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(target, event);
}

void PowerGate::closeEvent(QCloseEvent *event)
{
    if (confirmExitApp()){
        exitApp();
        event->accept();
    } else {
        event->ignore();
    }
}

void PowerGate::installSchedTask()
{
    m_schedLastSec = 0;
    m_schedCurrSec = 0;
    m_restartSecs = 0;

    readRestartTime();
    readStrategiesSchedTime();
    readSendReportMailTime();

    SYSTEMTIME lt;
    GetLocalTime(&lt);
    m_schedCurrSec = lt.wHour * 3600 + lt.wMinute * 60 + lt.wSecond;
    m_schedLastSec = m_schedCurrSec;

    if (m_restartTime.size() > 0 || 
        m_strategiesSchedTasks.size() > 0 ||
        m_sendReportMailTime.size() > 0) {
        m_timer = new QTimer(this);
        QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(timeElapsed()));
        m_timer->start(1000);
    } else {
        m_timer = nullptr;
    }
}

void PowerGate::checkSchedTask()
{
    if (!checkRestartTask()) {
        checkStrategiesSchedTask();
        checkSendMailSchedTask();
    }
}

void PowerGate::readRestartTime()
{
    rude::Config config;
    if (config.load(APP_CONFIG_FILE_NAME)) {
        config.setSection("Restart");
        int num = config.getNumDataMembers();
        if (num > 0) {
            for (int i = 0; i < num; i++) {
                const char *data = config.getDataNameAt(i);
                if (data != NULL) {
                    if (strlen(data) != strlen("xx:xx:xx")) {
                        continue;
                    }
                    char temp[16];
                    strcpy_s(temp, data);
                    temp[2] = '\0';
                    temp[5] = '\0';
                    int secs = atoi(temp) * 3600 + atoi(temp + 3) * 60 + atoi(temp + 6);
                    m_restartTime.push_back(secs);
                }
            }
        }
    }
}

bool PowerGate::checkRestartTask()
{
    for (size_t i = 0; i < m_restartTime.size(); i++) {
        m_restartSecs = m_restartTime[i];
        if (m_schedLastSec < m_restartSecs && m_schedCurrSec >= m_restartSecs) {
            if (!RA_ActivateRestartProcess()) {
                // Handle restart error here
                return false;
            }

            exitApp();

            return true;
        }
    }

    return false;
}

void PowerGate::readStrategiesSchedTime()
{
    rude::Config config;
    if (config.load(APP_CONFIG_FILE_NAME)) {
        config.setSection("Schedule");
        int num = config.getNumDataMembers();
        m_strategiesSchedTasks.clear();
        if (num <= 0) {
            return;
        }

        for (int i = 0; i < num; i++) {
            const char *task = config.getDataNameAt(i);
            if (task != NULL) {
                if (strlen(task) != strlen("x@xx:xx:xx")) {
                    continue;
                }
                if ((task[0] != 'S' && task[0] != 'P') ||
                    task[1] != '@' || task[4] != ':' || task[7] != ':') {
                    continue;
                }
                StrategySchedTask item = { 0 };
                if (task[0] == 'S') {
                    item.type = STRATEGY_SCHED_START;
                }
                else {
                    item.type = STRATEGY_SCHED_STOP;
                }
                char second[9];
                strncpy(second, task + 2, sizeof(second));
                second[2] = '\0';
                second[5] = '\0';
                second[8] = '\0';
                item.execSecs = atoi(second) * 3600 + atoi(second + 3) * 60 + atoi(second + 6);

                m_strategiesSchedTasks.push_back(item);
            }
        }
    }
}

bool PowerGate::checkStrategiesSchedTask()
{
    for (int i = 0; i < m_strategiesSchedTasks.size(); i++) {
        if (m_schedLastSec < m_strategiesSchedTasks.at(i).execSecs &&
            m_schedCurrSec >= m_strategiesSchedTasks.at(i).execSecs) {
            if (m_strategiesSchedTasks.at(i).type == STRATEGY_SCHED_START) {
                size_t i = 0;
                for (i = 0; i < strategies.size(); i++) {
                    pAlgoSE->launchStrategy(strategies[i].id);
                }
            }
            else if (m_strategiesSchedTasks.at(i).type == STRATEGY_SCHED_STOP) {
                size_t i = 0;
                for (i = 0; i < strategies.size(); i++) {
                    pAlgoSE->stopStrategy(strategies[i].id);
                }
            }

            m_strategiesSchedTasks.at(i).executed = true;
        }
    }

    return true;
}

void PowerGate::readSendReportMailTime()
{
    rude::Config config;
    if (config.load(APP_CONFIG_FILE_NAME)) {
        config.setSection("Mail");
        const char* p = config.getStringValue("ReportTime");
        if (p == NULL || p[0] == '\0') {
            return;
        }

        m_sendReportMailTime.clear();

        char line[1024] = { 0 };
        strncpy(line, p, sizeof(line));
        char* s = strtok(line, ";");
        while (s) {
            while (*s == ' ' || *s == '\t') s++;
            char time[16];
            strncpy(time, s, sizeof(time));
            if (strlen(time) != strlen("xx:xx:xx")) {
                continue;
            }
            time[2] = '\0';
            time[5] = '\0';
            time[8] = '\0';
            int secs = atoi(time) * 3600 + atoi(time + 3) * 60 + atoi(time + 6);
            m_sendReportMailTime.push_back(secs);

            s = strtok(NULL, ",");
        }
    }
}

bool PowerGate::checkSendMailSchedTask()
{
    for (int i = 0; i < m_sendReportMailTime.size(); i++) {
        if (m_schedLastSec < m_sendReportMailTime[i] &&
            m_schedCurrSec >= m_sendReportMailTime[i]) {
            sendReportMail();
            m_reportMailSent = true;
        }
    }

    return true;
}

void PowerGate::timeElapsed()
{
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    m_schedCurrSec = lt.wHour * 3600 + lt.wMinute * 60 + lt.wSecond;

    checkSchedTask();

    m_schedLastSec = m_schedCurrSec;
}