#ifndef BACKEND_STRATEGIES_H
#define BACKEND_STRATEGIES_H

// QMF
#include <qmfclient/qmailstore.h>  // QMailStore
#include <qmfclient/qmailserviceaction.h> // QMailRetrievalAction
#include <qmfclient/qmailfolder.h> // QMailFolder

#include "utils.h"

#include "serviceactionmanager.h"



namespace backend_strategy {



class InitFolder
{
public:
    void operator()(const QMailFolderId &id)
    {
        Q_ASSERT (id.isValid());
        QMailFolder folder(id);
        Q_ASSERT (folder.id().isValid());
        qDebug() << "@backend_strategy::InitFolder:"
                 << "retrieving folders for account" << folder.id();
        ServiceActionManager::instance()->retrieveMessageList(folder.parentAccountId(), folder.id(), 20);
    }
};



class InitAccount
{
public:
    void operator()(const QMailAccountId &id)
    {
        Q_ASSERT (id.isValid());
        qDebug() << "@backend_strategy::InitAccount:"
                 << "retrieving folders for account" << id;
        ServiceActionManager::instance()->retrieveFolderList(id, QMailFolderId());
    }
};



class SyncFolder
{
public:
    void operator()(const QMailFolderId &id)
    {
        //qWarning() << "### SyncFolder";
        Q_ASSERT (id.isValid());
        QMailFolder folder(id);
        Q_ASSERT (folder.id().isValid());

        qDebug() << "@backend_strategy::SyncFolder:"
                 << "retrieving messages for folder" << folder.id();
        ServiceActionManager::instance()->retrieveMessageList(folder.parentAccountId(), folder.id(), 20);
        // exportUpdates() ?
    }
};



class DownloadMessageBody
{
public:
    void operator()(const QMailMessage &message)
    {
        Q_ASSERT (message.id().isValid());

        const QMailMessagePartContainer *body_container = find::messageBody(message);
        if (NULL == body_container || check::isDownloaded(body_container)) {
            qWarning() << "@backend_strategy::DownloadMessageBody:"
                       << "DownloadMessageBody: nothing to download";
            return;
        }

        ServiceActionManager *manager = ServiceActionManager::instance();
        if (const QMailMessagePart *part = dynamic_cast<const QMailMessagePart*>(body_container)) {
            qDebug() << "@backend_strategy::DownloadMessageBody:"
                     << "Downloading part" << part->location().toString(true);
            manager->retrieveMessagePart(part->location());
        }
        else {
            qDebug() << "@backend_strategy::DownloadMessageBody:"
                     << "Downloading message" << message.id();
            manager->retrieveMessages(QMailMessageIdList() << message.id(),
                                      QMailRetrievalAction::Content);
        }
    }
};



class DownloadMessagePart
{
public:
    void operator()(const QMailMessagePart::Location &location)
    {
        qDebug() << "@backend_strategy::DownloadMessagePart:"
                 << "Downloading part" << location.toString(true);
        ServiceActionManager::instance()->retrieveMessagePart(location);
    }
};



class StopDownloadMessageBody
{
public:
    void operator()(const QMailMessage &message)
    {
        Q_ASSERT (message.id().isValid());

        const QMailMessagePartContainer *body_container = find::messageBody(message);
        if (NULL == body_container) {
            qWarning() << "@backend_strategy::StopDownloadMessageBody:"
                       << "body container not found.";
            return;
        }

        ServiceActionManager *manager = ServiceActionManager::instance();
        const QMailMessagePart *part = dynamic_cast<const QMailMessagePart*>(body_container);
        const QList<quint64> &serials = part ? manager->operations(part->location()) :
                                               manager->operations(message.id());
        if (serials.isEmpty()) {
            qWarning() << "@backend_strategy::StopDownloadMessageBody:"
                       << "nothing to stop.";
            return;
        }

        foreach (quint64 serial, serials) {
            qDebug() << "@backend_strategy::StopDownloadMessageBody:"
                     << "canceling" << serial;
            manager->cancelOperation(serial);
        }
    }
};



}



#endif // BACKEND_STRATEGIES_H
