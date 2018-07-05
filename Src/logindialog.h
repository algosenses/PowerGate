#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include "Defines.h"
#include "Engine.h"
#include "ui_logindialog.h"

using namespace AlgoSE;

typedef struct {
    std::string address;
    int port;
} ServerItem;

typedef struct {
    std::string name;
    std::vector<ServerItem> tradingSrvs;
    std::vector<ServerItem> marketdataSrvs;
} BrokerServer;

typedef struct {
    std::string ID;
    std::string name;
    std::vector<BrokerServer> servers;
} Broker;

typedef struct {
    std::string displayName;
    std::string brokerName;
    std::string brokerID;
    std::string bkrSrvName;
} BrokerSrvDisplayName;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    LoginDialog(Engine* engine, QWidget* parent = 0);
    ~LoginDialog();

private slots:
    void recvSystemLog(SystemLog log);
    void btnLoginClicked();
    void btnExitClicked();
    void btnEditBkrClicked();
    void btnSettingsClicked();
    void runAlgoSE();

private:
    bool parseBrokerXml(const char* file);
    void saveBrokerXml(const char* file);
    void readSettingsXml(const char* file);
    void saveSettingsXml(const char* file);
    void generateBkrDisplayNames();
    void parseLogText(const char* log);
    void loadLocalStrategies();

private:
    Ui::LoginDlg ui;

    Engine* pAlgoSE;
    bool inprogress;

    const char* settingsFile;
    const char* bkrXmlFile;

    typedef struct {
        std::string brokerid;
        std::string username;
        std::string password;
        std::string mdfront;
        std::string tradefront;
        bool remoteMgrEnabled;
        int remotePort;
    } LoginData;

    LoginData loginData;

    std::list<Broker> brokers;

    std::vector<BrokerSrvDisplayName> bkrDispalyNames;
};

#endif // LOGIN_DIALOG_H