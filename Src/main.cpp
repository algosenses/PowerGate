#include <QApplication>
#include <QStyleFactory>
#include <QStandardPaths>
#include <QTextCodec>
#include <QTranslator>
#include <QFontDatabase>
#include "main.h"
#include "authdialog.h"
#include "RestartAPI.h"

#ifdef _DEBUG
#if !_WIN64
#include "vld.h"
#endif
#endif

AppGlobalData* appGlobalData_;

void AppGlobalData::Construct()
{
    currentTheme = 0;
    timeFormat = QLocale().timeFormat(QLocale::LongFormat).replace(" ", "").replace("t", "");
    dateTimeFormat = QLocale().dateFormat(QLocale::ShortFormat) + " " + timeFormat;
    defaultHeightForRow_ = 22;
    translateLang = true;
    requestAuth = false;
    requestLogin = false;
    localLoading = true;
}

int main(int argc, char *argv[])
{
#if _DEBUG
    if (!AllocConsole()) {
        return 0;
    }

    freopen("CONIN$", "rb", stdin);
    freopen("CONOUT$", "wb", stdout);
    freopen("CONOUT$", "wb", stderr);
#endif

    // Initialize restart code
    // Check if this instance is restarted and 
    // wait while previous instance finish
    if (RA_CheckForRestartProcessStart()) {
        RA_WaitForPreviousProcessFinish();
    }

    // It is important to put QApplication into a local domain, thus ui widgets 
    // will be totally released before exit main function.
    int ret = 0;
    {

        appGlobalData_ = new AppGlobalData();
        appGlobalData.Construct();

        QTextCodec::setCodecForLocale(QTextCodec::codecForName("system"));

        QApplication app(argc, argv);
        QApplication::setOrganizationName("Algo.Trade");
        QApplication::setApplicationName("PowerGate");
        QApplication::setApplicationVersion("0.1");

        if (appGlobalData.translateLang) {
            QTranslator* translator = new QTranslator(&app);
            translator->load(":/trans/PowerGate.qm");
            app.installTranslator(translator);
        }

#if QT_VERSION >= 0x050000
        app.setStyle(QStyleFactory::create("Fusion"));
#endif

#ifdef Q_OS_WIN//DPI Fix
        QFont font = app.font();
        QFontDatabase database;
        QStringList fntList = database.families();
        if (fntList.contains("Consolas")) {
            font.setFamily("Consolas");
        }
        font.setPointSize(9);
        app.setFont(font);
#endif

        app.setApplicationName("PowerGate");
        appGlobalData.fontMetrics_ = new QFontMetrics(app.font());

#if QT_VERSION < 0x050000
        appGlobalData.tempLocation = QDesktopServices::storageLocation(QDesktopServices::TempLocation).replace('\\', '/') + "/";
        appGlobalData.desktopLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation).replace('\\', '/') + "/";
#else
        appGlobalData.tempLocation = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first().replace('\\', '/') + "/";
        appGlobalData.desktopLocation = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first().replace('\\', '/') + "/";
#endif

#if 1
        appGlobalData.appTheme.palette = app.palette();
        appGlobalData.appTheme.loadTheme("Dark");
        app.setPalette(appGlobalData.appTheme.palette);
        app.setStyleSheet(appGlobalData.appTheme.styleSheet);
#endif

        app.setWindowIcon(QIcon(":/icon/Letter-P-black-icon.png"));

        if (appGlobalData.requestAuth) {
            AuthDialog dlg;
            if (dlg.exec() != QDialog::Accepted) {
                app.exit();
                return 0;
            }
        }

        appGlobalData.mainWindow_ = new PowerGate();
        app.installEventFilter(appGlobalData.mainWindow_);

        if (!appGlobalData.requestLogin) {
            mainWindow.showMaximized();
        }

        ret = app.exec();

        delete appGlobalData.mainWindow_;
        delete appGlobalData.fontMetrics_;
        delete appGlobalData_;
    }

    // Finish restarting process if needed
    RA_DoRestartProcessFinish();

    return ret;
}