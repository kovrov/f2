QT += core gui

LIBS += -lqmfclient

TARGET = desktop
TEMPLATE = app

SOURCES += \
    application.cpp \
    main.cpp\
    serviceactionmanager.cpp \
    uimanager.cpp \
    view.cpp \
    models/folderlistmodel.cpp \
    models/folderstreemodel.cpp \
    widgets/combobox.cpp \
    widgets/inputdialog.cpp \
    widgets/progressindicator.cpp \
    models/messagemodel.cpp \
    widgets/messagelistdelegate.cpp \
    debug.cpp \
    models/attachmentlistmodel.cpp \
    widgets/attachmentlistdelegate.cpp \
    models/messagelistmodel.cpp \
    widgets/messagewidget.cpp \
    utils.cpp

HEADERS  += \
    application.h \
    backendstrategies.h \
    context.h \
    serviceactionmanager.h \
    uimanager.h \
    uistrategies.h \
    view.h \
    models/folderlistmodel.h \
    models/folderstreemodel.h \
    widgets/combobox.h \
    widgets/inputdialog.h \
    widgets/progressindicator.h \
    models/messagemodel.h \
    widgets/messagelistdelegate.h \
    models/progressinfo.h \
    debug.h \
    models/attachmentlistmodel.h \
    widgets/attachmentlistdelegate.h \
    models/messagelistmodel.h \
    widgets/messagewidget.h \
    utils.h

FORMS += \
    mainview.ui \
    attachmentview.ui

QMAKE_CXXFLAGS += -std=c++0x

profile {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS += -pg
}

CONFIG += warn_on
