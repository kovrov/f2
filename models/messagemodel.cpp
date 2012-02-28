#include <qdebug.h>
#include <qmfclient/qmailstore.h>

#include "debug.h"
#include "serviceactionmanager.h"
#include "messagemodel.h"


#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }



models::MessageModel::MessageModel(QObject *parent)
  : QObject (parent)
{
    CONNECT (QMailStore::instance(), SIGNAL(messageContentsModified(QMailMessageIdList)),
                             this, SLOT(on_messageContentsModified(QMailMessageIdList)));
    CONNECT (QMailStore::instance(), SIGNAL(messageDataUpdated(QMailMessageMetaDataList)),
                             this, SLOT(on_messageDataUpdated(QMailMessageMetaDataList)));
    CONNECT (QMailStore::instance(), SIGNAL(messagePropertyUpdated(QMailMessageIdList,QMailMessageKey::Properties,QMailMessageMetaData)),
                             this, SLOT(on_messagePropertyUpdated(QMailMessageIdList,QMailMessageKey::Properties,QMailMessageMetaData)));
    CONNECT (QMailStore::instance(), SIGNAL(messageStatusUpdated(QMailMessageIdList,quint64,bool)),
                             this, SLOT(on_messageStatusUpdated(QMailMessageIdList,quint64,bool)));
    CONNECT (QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)),
                             this, SLOT(on_messagesUpdated(QMailMessageIdList)));
    CONNECT (ServiceActionManager::instance(), SIGNAL(activityChanged(quint64,QMailServiceAction::Activity)),
                             this, SLOT(on_activityChanged(quint64,QMailServiceAction::Activity)));
}


void models::MessageModel::setMessageId(const QMailMessageId &id)
{
    mMessage = QMailMessage(id);
    mOperations = ServiceActionManager::instance()->operations(mMessage.id());
    emit modelReset();
    emit updated(); /// TODO: emit only modelReset
}


void models::MessageModel::on_messageContentsModified(const QMailMessageIdList &ids)
{
    Q_UNUSED (ids);
//    foreach (const QMailMessageId &id, ids) {
//        if (id != mMessage.id())
//            continue;
//        mMessage = QMailMessage(id);
//        emit updated();
//        return;
//    }
}


void models::MessageModel::on_messageDataUpdated(const QMailMessageMetaDataList &list)
{
    Q_UNUSED (list);
//    foreach (const QMailMessageMetaData &data, list) {
//        data.id();
//    }
}


void models::MessageModel::on_messagePropertyUpdated(const QMailMessageIdList &ids, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data)
{
    Q_UNUSED (ids);
    Q_UNUSED (properties);
    Q_UNUSED (data);
}


void models::MessageModel::on_messageStatusUpdated(const QMailMessageIdList &ids, quint64 status, bool set)
{
    Q_UNUSED (ids);
    Q_UNUSED (status);
    Q_UNUSED (set);
}


void models::MessageModel::on_messagesUpdated(const QMailMessageIdList &ids)
{
    if (mMessage.id().isValid() && ids.contains(mMessage.id())) {
        mMessage = QMailMessage(mMessage.id());
        emit updated();
    }
}


void models::MessageModel::on_activityChanged(quint64 serial, QMailServiceAction::Activity activity)
{
    if (!mMessage.id().isValid())
        return;

    switch (activity) {

    case QMailServiceAction::Pending: {
        // operation added, remembering
        const auto &operations = ServiceActionManager::instance()->operations(mMessage.id());
        if (!operations.contains(serial))
            return;

        mOperations = operations;
        emit updated();
    }   break;

    case QMailServiceAction::Successful:
    case QMailServiceAction::Failed:
        // operation removed
        if (!mOperations.contains(serial))
            return;

        emit updated();
        break;

    default: ;
    }
}
