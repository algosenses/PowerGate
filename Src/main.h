#ifndef MAIN_H
#define MAIN_H

#include <QFontMetrics>
#include "apptheme.h"
#include "powergate.h"

#define mainWindow (*appGlobalData_->mainWindow_)
#define defaultHeightForRow appGlobalData_->defaultHeightForRow_

typedef struct {
    void Construct();
    
    QString osStyle;

    int currentTheme;
    AppTheme appTheme;
    int defaultHeightForRow_;
    QFontMetrics *fontMetrics_;
    QString themeFolder;
    QString timeFormat;
    QString dateTimeFormat;
    QString appDataDir_;
    QByteArray appVerStr;
    QString iniFileName;
    QString desktopLocation;
    QString tempLocation;
    bool translateLang;
    bool requestLogin;
    bool requestAuth;
    bool localLoading;

    PowerGate* mainWindow_;

} AppGlobalData;

#define appGlobalData (*appGlobalData_)
extern AppGlobalData *appGlobalData_;

#endif // MAIN_H