#-------------------------------------------------
#
# Project created by QtCreator 2014-10-31T21:23:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS += PowerGate.ts

TARGET = PowerGate
TEMPLATE = app

INCLUDEPATH += e:/Workshop/YuanLang/Solution/ATS-SDK/Inc

LIBS += e:/Workshop/YuanLang/Solution/ATS-SDK/Lib/AlgoSE.lib

SOURCES += main.cpp \
    apptheme.cpp \
    dock_host.cpp \
    logmodel.cpp \
    posmodel.cpp \
    ordermodel.cpp \
    stratmodel.cpp \
    quotemodel.cpp \
    stratparamsdialog.cpp \
    stratparamsmodel.cpp \
    stratposmodel.cpp \
    stratposdialog.cpp \
    stratlogmodel.cpp \
    powergate.cpp \
    authdialog.cpp \
    quotedialog.cpp \
    stratcmddialog.cpp \
    subscribedialog.cpp \
    logindialog.cpp \
    loginsettingsdialog.cpp \
    editbkrdialog.cpp \
    tradepanel.cpp \
    main.cpp

HEADERS  += apptheme.h \
    dock_host.h \
    logmodel.h \
    posmodel.h \
    ordermodel.h \
    stratmodel.h \
    quotemodel.h \
    stratparamsdialog.h \
    stratparamsmodel.h \
    stratposdialog.h \
    powergate.h \
    authdialog.h \
    quotedialog.h \
    stratcmddialog.h \
    subscribedialog.h

FORMS    += \
    powergate.ui \
    stratparams.ui \
    stratpos.ui \
    authdialog.ui \
    quotedialog.ui \
    stratcmd.ui \
    subscribedialog.ui \
    stratlog.ui \
    logindialog.ui \
    loginsettingsdialog.ui \
    editbkrdialog.ui \
    tradepanel.ui

RESOURCES = PowerGate.qrc
