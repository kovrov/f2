#include "application.h"

//Qt
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <qdebug.h>

//QMF
//#include <qmfclient/qmailid.h> // QMailMessageId



Application::Application(int &argc, char**argv)
  : QApplication (argc, argv)
{
    setOrganizationName("f2");
}


Application::~Application()
{
}


int Application::exec()
{
    const QStringList &args = QCoreApplication::arguments();
    if (!launchRequest(args.size() > 1 ? args.mid(1) : QStringList())) {
        // TODO: report error
    }

    return super::exec();
}


bool Application::launchRequest(const QStringList& parameters)
{
    Q_UNUSED (parameters);
//    if (parameters.isEmpty()) {
        emit displayAppRequest();
        return true;
//    }

    return false;
}
