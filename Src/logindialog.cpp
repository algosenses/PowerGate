#include <QTimer>
#include "config.h"
#include "tinyxml2.h"
#include "ServiceConfig.h"
#include "Service.h"
#include "Engine.h"
#include "main.h"
#include "logindialog.h"
#include "editbkrdialog.h"
#include "loginsettingsdialog.h"

using namespace tinyxml2;

LoginDialog::LoginDialog(Engine* engine, QWidget* parent)
    : QDialog(parent)
{
    pAlgoSE = engine;
    inprogress = false;
    ui.setupUi(this);

    settingsFile = "PowerGate.xml";
    bkrXmlFile = "broker.xml";
    loginData.remotePort = 5501;
    loginData.remoteMgrEnabled = true;

    parseBrokerXml(bkrXmlFile);

    generateBkrDisplayNames();

    connect(ui.loginBtn, SIGNAL(clicked()), this, SLOT(btnLoginClicked()));
    connect(ui.exitBtn, SIGNAL(clicked()), this, SLOT(btnExitClicked()));
    connect(ui.editBkrBtn, SIGNAL(clicked()), this, SLOT(btnEditBkrClicked()));
    connect(ui.settingsBtn, SIGNAL(clicked()), this, SLOT(btnSettingsClicked()));

    ui.userLineEdit->setFocus();

    readSettingsXml(settingsFile);
}

LoginDialog::~LoginDialog()
{

}

void LoginDialog::recvSystemLog(SystemLog log)
{
    parseLogText(log.text);
}

void LoginDialog::btnLoginClicked()
{
    if (inprogress) {
        return;
    }
    
    if (bkrDispalyNames.empty()) {
        return;
    }

    int idx = ui.brokerComboBox->currentIndex();
    if (idx >= bkrDispalyNames.size()) {
        return;
    }

    loginData.mdfront.clear();
    loginData.tradefront.clear();
    loginData.brokerid.clear();

    std::string bkrName = bkrDispalyNames[idx].brokerName;
    std::string bkrSrvName = bkrDispalyNames[idx].bkrSrvName;
    for (auto& bkr : brokers) {
        if (bkr.name == bkrName) {
            loginData.brokerid = bkr.ID;
            for (auto& srv : bkr.servers) {
                if (srv.name == bkrSrvName) {
                    for (auto& trading : srv.tradingSrvs) {
                        if (!loginData.tradefront.empty()) {
                            loginData.tradefront += ";";
                        }
                        char addr[256];
                        sprintf(addr, "%s:%d", trading.address.c_str(), trading.port);
                        loginData.tradefront += std::string(addr);
                    }
                    for (auto& md : srv.marketdataSrvs) {
                        if (!loginData.mdfront.empty()) {
                            loginData.mdfront += ";";
                        }
                        char addr[256];
                        sprintf(addr, "%s:%d", md.address.c_str(), md.port);
                        loginData.mdfront += std::string(addr);
                    }
                }
            }
        }
    }

    loginData.username = ui.userLineEdit->text().toStdString();
    loginData.password = ui.passwdLineEdit->text().toStdString();

    if (loginData.username.empty()) {
        ui.loginStatus->setText(tr("Please input user name"));
        ui.userLineEdit->setFocus();
        return;
    }

    if (loginData.password.empty()) {
        ui.loginStatus->setText(tr("Please input password"));
        ui.passwdLineEdit->setFocus();
        return;
    }

    saveSettingsXml(settingsFile);
    inprogress = true;
    ui.loginStatus->setText(tr("Start..."));
    ui.userLineEdit->setDisabled(true);
    ui.passwdLineEdit->setDisabled(true);
    ui.settingsBtn->setDisabled(true);
    ui.editBkrBtn->setDisabled(true);

    ui.loginBtn->setDisabled(true);

    QTimer::singleShot(100, this, SLOT(runAlgoSE()));
}

void LoginDialog::btnExitClicked()
{
    reject();
}

void LoginDialog::btnEditBkrClicked()
{
    std::string bkrName, bkrID, srvName;

    int idx = ui.brokerComboBox->currentIndex();
    if (idx < bkrDispalyNames.size()) {
        bkrName = bkrDispalyNames[idx].brokerName;
        bkrID = bkrDispalyNames[idx].brokerID;
        srvName = bkrDispalyNames[idx].bkrSrvName;
    }

    EditBkrDialog dlg(brokers, bkrName, bkrID, srvName, this);
    
    if (dlg.exec() == QDialog::Accepted) {
        bkrName = dlg.getCurrBkrName();
        bkrID   = dlg.getCurrBkrID();
        srvName = dlg.getCurrSrvName();
        
        generateBkrDisplayNames();

        if (bkrName.empty() || srvName.empty()) {
            if (ui.brokerComboBox->count() > 0) {
                ui.brokerComboBox->setCurrentIndex(0);
            }
        } else {
            for (size_t i = 0; i < bkrDispalyNames.size(); i++) {
                if (bkrDispalyNames[i].brokerName == bkrName &&
                    bkrDispalyNames[i].bkrSrvName == srvName) {
                    ui.brokerComboBox->setCurrentIndex(i);
                    break;
                }
            }
        }

        saveBrokerXml(bkrXmlFile);
    }

    ui.userLineEdit->setFocus();
}

void LoginDialog::btnSettingsClicked()
{
    LoginSettingsDialog dlg(this);
    dlg.setRemoteMgrEnabled(loginData.remoteMgrEnabled);
    dlg.setPort(loginData.remotePort);

    if (dlg.exec() == QDialog::Accepted) {
        loginData.remoteMgrEnabled = dlg.isRemoteMgrEnabled();
        loginData.remotePort = dlg.getPort();
    }
}

void LoginDialog::parseLogText(const char* log)
{
    int stepNum = 8;
    int step = 100 / stepNum;
    
    const char* match = NULL;
    if (strstr(log, "Fut_Exec: User login succeed!") != NULL) {
        ui.loginStatus->setText(tr("Connect..."));
        ui.progressBar->setValue(step);
    } else if (strstr(log, "Fut_Exec: Query investor...") != NULL) {
        ui.loginStatus->setText(tr("Query investor..."));
        ui.progressBar->setValue(2 * step);
    } else if (strstr(log, "Fut_Exec: Query investor fund") != NULL) {
        ui.loginStatus->setText(tr("Query fund..."));
        ui.progressBar->setValue(3 * step);
    } else if (strstr(log, "Fut_Exec: Query instrument") != NULL) {
        ui.loginStatus->setText(tr("Query instrument..."));
        ui.progressBar->setValue(4 * step);
    } else if (strstr(log, "Fut_Exec: Query order") != NULL) {
        ui.loginStatus->setText(tr("Query order..."));
        ui.progressBar->setValue(5 * step);
    } else if (strstr(log, "Fut_Exec: Query trade") != NULL) {
        ui.loginStatus->setText(tr("Query trade..."));
        ui.progressBar->setValue(6 * step);
    } else if (strstr(log, "Fut_Exec: Query investor position") != NULL) {
        ui.loginStatus->setText(tr("Query position..."));
        ui.progressBar->setValue(7 * step);
    } else if (strstr(log, "Execution service 'Fut_Exec' ready") != NULL) {
        ui.loginStatus->setText(tr("Login succeed"));
        ui.progressBar->setValue(8 * step);
        
        accept();
    } else if ((match = strstr(log, "'Fut_Exec' connect error, reason: CTP:")) != NULL) {
        const char* err = match + strlen("'Fut_Exec' connect error, reason: CTP:");
        ui.loginStatus->setText(QString(tr("Error: ")) + QString().fromLocal8Bit(err));
        pAlgoSE->stop();

        ui.loginBtn->setEnabled(false);
        ui.exitBtn->setFocus();
        ui.userLineEdit->setEnabled(true);
        ui.passwdLineEdit->setEnabled(true);
        ui.settingsBtn->setEnabled(true);
        ui.editBkrBtn->setEnabled(true);
    }
}

bool LoginDialog::parseBrokerXml(const char* file)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(file) != XML_NO_ERROR) {
        return false;
    }

    tinyxml2::XMLElement* rootElem = doc.FirstChildElement("root");
    if (!rootElem) {
        return false;
    }

    tinyxml2::XMLElement* bkrElem = rootElem->FirstChildElement("broker");
    while (bkrElem) {
        const char* brokerID = bkrElem->Attribute("BrokerID");
        const char* brokerName = bkrElem->Attribute("BrokerName");
        if (!brokerID || brokerID[0] == '\0' || !brokerName || brokerName[0] == '\0') {
            bkrElem = bkrElem->NextSiblingElement();
            continue;
        }

        Broker broker;
        broker.ID = brokerID;
        broker.name = brokerName;
        bool integrated = false;
        tinyxml2::XMLElement* srvsElem = bkrElem->FirstChildElement("Servers");
        if (srvsElem) {
            tinyxml2::XMLElement* srvElem = srvsElem->FirstChildElement("Server");
            while (srvElem) {
                tinyxml2::XMLElement* srvNameElem = srvElem->FirstChildElement("Name");
                if (!srvNameElem) {
                    srvElem = srvElem->NextSiblingElement();
                    continue;
                }
                const char* srvName = srvNameElem->GetText();
                if (!srvName || srvName[0] == '\0') {
                    srvElem = srvElem->NextSiblingElement();
                    continue;
                }

                BrokerServer server;
                server.name = srvName;
                tinyxml2::XMLElement* tradingElem = srvElem->FirstChildElement("Trading");
                tinyxml2::XMLElement* mdElem = srvElem->FirstChildElement("MarketData");
                if (!tradingElem || !mdElem) {
                    srvElem = srvElem->NextSiblingElement();
                    continue;
                }

                tinyxml2::XMLElement* itemElem = tradingElem->FirstChildElement("item");
                while (itemElem) {
                    const char* addr = itemElem->GetText();
                    if (addr && addr[0] != '\0') {
                        char tmp[256] = { 0 };
                        strncpy(tmp, addr, sizeof(tmp) - 1);
                        char* p = strchr(tmp, ':');
                        if (p) {
                            *p = '\0';
                            p++;
                            ServerItem item;
                            item.address = tmp;
                            item.port = atoi(p);
                            server.tradingSrvs.push_back(item);
                        }
                    }

                    itemElem = itemElem->NextSiblingElement();
                }

                itemElem = mdElem->FirstChildElement("item");
                while (itemElem) {
                    const char* addr = itemElem->GetText();
                    if (addr && addr[0] != '\0') {
                        char tmp[256] = { 0 };
                        strncpy(tmp, addr, sizeof(tmp)-1);
                        char* p = strchr(tmp, ':');
                        if (p) {
                            *p = '\0';
                            p++;
                            ServerItem item;
                            item.address = tmp;
                            item.port = atoi(p);
                            server.marketdataSrvs.push_back(item);
                        }
                    }

                    itemElem = itemElem->NextSiblingElement();
                }

                if (server.marketdataSrvs.size() > 0 && server.tradingSrvs.size() > 0) {
                    broker.servers.push_back(server);
                    integrated = true;
                }

                srvElem = srvElem->NextSiblingElement();
            }
        }

        if (integrated) {
            brokers.push_back(broker);
        }

        bkrElem = bkrElem->NextSiblingElement();
    }

    return true;
}

void LoginDialog::saveBrokerXml(const char* file)
{
    tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument();
    if (doc == nullptr) {
        return;
    }

    XMLDeclaration *dec = nullptr;
    dec = doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    if (dec == nullptr) {
        delete doc;
        return;
    }

    doc->LinkEndChild(dec);

    // root
    XMLElement* rootElem = doc->NewElement("root");
    if (rootElem == nullptr) {
        delete doc;
    }
    doc->LinkEndChild(rootElem);

    for (auto& bkr : brokers) {
        // broker
        XMLElement* bkrElem = doc->NewElement("broker");
        bkrElem->SetAttribute("BrokerID", bkr.ID.c_str());
        bkrElem->SetAttribute("BrokerName", bkr.name.c_str());

        XMLElement* srvsElem = doc->NewElement("Servers");
        for (auto& srv : bkr.servers) {
            XMLElement* srvElem = doc->NewElement("Server");
            XMLElement* nameElem = doc->NewElement("Name");
            XMLText* text = doc->NewText(srv.name.c_str());
            nameElem->LinkEndChild(text);
            srvElem->LinkEndChild(nameElem);


            XMLElement* tradingElem = doc->NewElement("Trading");
            for (auto& tradingSrv : srv.tradingSrvs) {
                XMLElement* itemElem = doc->NewElement("item");
                char addr[512];
                sprintf(addr, "%s:%d", tradingSrv.address.c_str(), tradingSrv.port);
                XMLText* text = doc->NewText(addr);
                itemElem->LinkEndChild(text);

                tradingElem->LinkEndChild(itemElem);
            }
            srvElem->LinkEndChild(tradingElem);

            XMLElement* mdElem = doc->NewElement("MarketData");
            for (auto& mdSrv : srv.marketdataSrvs) {
                XMLElement* itemElem = doc->NewElement("item");
                char addr[512];
                sprintf(addr, "%s:%d", mdSrv.address.c_str(), mdSrv.port);
                XMLText* text = doc->NewText(addr);
                itemElem->LinkEndChild(text);

                mdElem->LinkEndChild(itemElem);
            }
            srvElem->LinkEndChild(mdElem);

            srvsElem->LinkEndChild(srvElem);
        }

        bkrElem->LinkEndChild(srvsElem);
        rootElem->LinkEndChild(bkrElem);
    }

    doc->SaveFile(file);

    delete doc;
}

void LoginDialog::readSettingsXml(const char* file)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(file) != XML_NO_ERROR) {
        return;
    }

    tinyxml2::XMLElement* rootElem = doc.FirstChildElement("root");
    if (!rootElem) {
        return;
    }

    tinyxml2::XMLElement* loginElem = rootElem->FirstChildElement("login");
    if (loginElem) {
        tinyxml2::XMLElement* userElem = loginElem->FirstChildElement("user");
        if (userElem) {
            const char* text = userElem->GetText();
            if (text && text[0] != '\0') {
                ui.userLineEdit->setText(text);
            }
        }

        tinyxml2::XMLElement* bkrElem = loginElem->FirstChildElement("broker");
        if (bkrElem) {
            const char* bkrIdx = bkrElem->GetText();
            if (bkrIdx && bkrIdx[0]) {
                int idx = atoi(bkrIdx);
                if (idx >= 0 && idx < bkrDispalyNames.size()) {
                    ui.brokerComboBox->setCurrentIndex(idx);
                }
            }
        }
    }

    tinyxml2::XMLElement* settingsElem = rootElem->FirstChildElement("settings");
    if (settingsElem) {
        tinyxml2::XMLElement* remoteMgrEnabledElem = settingsElem->FirstChildElement("manager");
        if (remoteMgrEnabledElem) {
            const char* text = remoteMgrEnabledElem->GetText();
            if (text && text[0] != '\0') {
                loginData.remoteMgrEnabled = !_stricmp(text, "true");
            }
        }

        tinyxml2::XMLElement* portElem = settingsElem->FirstChildElement("port");
        if (portElem) {
            const char* text = portElem->GetText();
            if (text && text[0] != '\0') {
                loginData.remotePort = atoi(text);
            }
        }
    }
}

void LoginDialog::saveSettingsXml(const char* file)
{
    tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument();
    if (doc == nullptr) {
        return;
    }

    XMLDeclaration *dec = nullptr;
    dec = doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    if (dec == nullptr) {
        delete doc;
        return;
    }

    doc->LinkEndChild(dec);

    // root
    XMLElement* rootElem = doc->NewElement("root");
    if (rootElem == nullptr) {
        delete doc;
    }
    doc->LinkEndChild(rootElem);

    // login
    XMLElement* loginElem = doc->NewElement("login");

    XMLElement* userElem = doc->NewElement("user");
    XMLText* text;
    if (ui.remCheckBox->isChecked()) {
        text = doc->NewText(ui.userLineEdit->text().toStdString().c_str());
        userElem->LinkEndChild(text);
    }
    loginElem->LinkEndChild(userElem);

    int idx = ui.brokerComboBox->currentIndex();
    if (idx < bkrDispalyNames.size()) {
        XMLElement* bkrElem = doc->NewElement("broker");
        char idxStr[16];
        sprintf(idxStr, "%d", idx);
        text = doc->NewText(idxStr);
        bkrElem->LinkEndChild(text);
        loginElem->LinkEndChild(bkrElem);
    }

    rootElem->LinkEndChild(loginElem);

    XMLElement* settingsElem = doc->NewElement("settings");
    XMLElement* remoteEnabledElem = doc->NewElement("manager");
    if (loginData.remoteMgrEnabled) {
        text = doc->NewText("true");
    } else {
        text = doc->NewText("false");
    }
    remoteEnabledElem->LinkEndChild(text);
    settingsElem->LinkEndChild(remoteEnabledElem);

    XMLElement* remoteElem = doc->NewElement("port");
    char port[64];
    sprintf(port, "%d", loginData.remotePort);
    text = doc->NewText(port);
    remoteElem->LinkEndChild(text);

    settingsElem->LinkEndChild(remoteElem);

    rootElem->LinkEndChild(settingsElem);

    doc->SaveFile(file);

    delete doc;
}

void LoginDialog::generateBkrDisplayNames()
{
    bkrDispalyNames.clear();

    for (auto& bkr : brokers) {
        for (auto& srv : bkr.servers) {
            BrokerSrvDisplayName display;
            display.brokerName = bkr.name;
            display.brokerID = bkr.ID;
            display.bkrSrvName = srv.name;
            display.displayName = bkr.name + "-" + srv.name;
            bkrDispalyNames.push_back(display);
        }
    }

    ui.brokerComboBox->clear();
    for (auto& elem : bkrDispalyNames) {
        ui.brokerComboBox->addItem(QString().fromLocal8Bit(elem.displayName.c_str()));
    }
}

void LoginDialog::runAlgoSE()
{
    ServiceConfig mdSrvConf;
    mdSrvConf.setType(ServiceType::SERVICE_MARKET_DATA);
    mdSrvConf.setName("Fut_MD");
    mdSrvConf.setSharedLibrary("Ctp.dll");
    mdSrvConf.setParameter("frontend", loginData.mdfront.c_str());

    ServiceConfig execSrvConf;
    execSrvConf.setType(ServiceType::SERVICE_ORDER_EXECUTION);
    execSrvConf.setName("Fut_Exec");
    execSrvConf.setSharedLibrary("Ctp.dll");
    //    execSrvConf.setConfigFile("ctp_future.xml");
    execSrvConf.setParameter("brokerid", loginData.brokerid.c_str());
    execSrvConf.setParameter("user",     loginData.username.c_str());
    execSrvConf.setParameter("password", loginData.password.c_str());
    execSrvConf.setParameter("frontend", loginData.tradefront.c_str());

    pAlgoSE->registerService(mdSrvConf);
    pAlgoSE->registerService(execSrvConf);

    pAlgoSE->setRemoteMgrUnderlyingServiceName(ServiceType::SERVICE_MARKET_DATA, "Fut_MD");
    pAlgoSE->setRemoteMgrUnderlyingServiceName(ServiceType::SERVICE_ORDER_EXECUTION, "Fut_Exec");
    pAlgoSE->setRemoteMgrListenPort(loginData.remotePort);
    if (loginData.remoteMgrEnabled) {
        pAlgoSE->enableRemoteManager();
    }

    if (!pAlgoSE->initialize()) {
        return;
    }

    pAlgoSE->run();

    if (appGlobalData.localLoading) {
        loadLocalStrategies();
    }
}

void LoginDialog::loadLocalStrategies()
{
    rude::Config config;
    std::vector<std::string> strategies;
    if (config.load("PowerGate.ini")) {
        config.setSection("Strategies");
        int num = config.getNumDataMembers();
        if (num > 0) {
            for (int i = 0; i < num; i++) {
                const char *data = config.getDataNameAt(i);
                if (data && data[0] != '\0') {
                    strategies.push_back(data);
                }
            }
        }
    }

    for (size_t i = 0; i < strategies.size(); i++) {
        pAlgoSE->loadStrategy(strategies[i].c_str());
    }
}