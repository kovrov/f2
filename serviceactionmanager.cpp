#include <qmfclient/qmailmessage.h>

#include <qmfclient/qmailmessageserver.h>


#include "debug.h"
#include "serviceactionmanager.h"

#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }



class OperationContext : public ServiceActionManager::OperationInfo
{
public:
    enum Priority {
        Low = 0,
        Normal,
        High
    };
    virtual ~OperationContext() {}
    virtual void exec(QMailMessageServer *server) = 0;
    void cancelOperation(QMailMessageServer *server) { server->cancelTransfer(serial); }
    quint64 serial;
    QMailServiceAction::Status status;
    Priority priority;
    OperationContext() : serial (0), priority (Normal) {}
//    bool operator==(const quint64 serial) const { return serial == serial ? true : false; }
    virtual QMailMessageIdList messageIds() const { return QMailMessageIdList(); }
    virtual QMailMessagePart::Location messagePartLocation() const { return QMailMessagePart::Location(); }
};




ServiceActionManager::ServiceActionManager(QObject *parent)
  : QObject (parent),
    mServer (new QMailMessageServer(this)),
    mCurrent (NULL),
    mSerial (0)
{
    CONNECT (mServer, SIGNAL(activityChanged(quint64,QMailServiceAction::Activity)),
             this, SLOT(on_activityChanged(quint64,QMailServiceAction::Activity)));
    CONNECT (mServer, SIGNAL(connectivityChanged(quint64,QMailServiceAction::Connectivity)),
             this, SLOT(on_connectivityChanged(quint64,QMailServiceAction::Connectivity)));
    CONNECT (mServer, SIGNAL(progressChanged(quint64,uint,uint)),
             this, SLOT(on_progressChanged(quint64,uint,uint)));
    CONNECT (mServer, SIGNAL(statusChanged(quint64,QMailServiceAction::Status)),
             this, SLOT(on_statusChanged(quint64,QMailServiceAction::Status)));
}


ServiceActionManager * ServiceActionManager::instance()
{
    static ServiceActionManager *self = NULL;
    if (NULL == self)
        self = new ServiceActionManager();
    return self;
}


void ServiceActionManager::cancelOperation(quint64 serial)
{
    if (mCurrent && mCurrent->serial == serial) {
        mCurrent->cancelOperation(mServer);
        return;
    }

    if (mQueue.isEmpty())
        return;

    // qFind
    QList<OperationContext*>::iterator it = mQueue.begin();
    while (mQueue.end() != it && (*it)->serial != serial) ++it;
    if (mQueue.end() == it)
        return;

    OperationContext *operation(*it);
    _removeFromMessageIdsCache(operation->serial);
    mQueue.erase(it);
    emit activityChanged(operation->serial, QMailServiceAction::Failed); // QMailServiceAction::Successful?
    delete operation;
}


quint64 ServiceActionManager::retrieveFolderList(const QMailAccountId &account_id, const QMailFolderId &folder_id, bool is_descending)
{
    class Operation : public OperationContext
    {
        QMailAccountId accountId;
        QMailFolderId folderId;
        bool descending;
    public:
        Operation(const QMailAccountId &account_id, const QMailFolderId &folder_id, bool is_descending)
          : accountId (account_id), folderId (folder_id), descending (is_descending) { priority = Low; }
        void exec(QMailMessageServer *server)
        {
            server->retrieveFolderList(serial, accountId, folderId, descending);
        }
    };

    auto op = new Operation(account_id, folder_id, is_descending);
    op->serial = ++mSerial;
    _enqueue(op);
    return op->serial;
}


quint64 ServiceActionManager::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    class Operation : public OperationContext
    {
        QMailAccountId accountId;
        QMailFolderId folderId;
        uint minimum;
        QMailMessageSortKey sort;
    public:
        Operation(const QMailAccountId &account_id, const QMailFolderId &folder_id, uint minimum_count, const QMailMessageSortKey &sort_key)
          : accountId (account_id), folderId (folder_id), minimum (minimum_count), sort (sort_key) { priority = Low; }
        void exec(QMailMessageServer *server)
        {
            server->retrieveMessageList(serial, accountId, folderId, minimum, sort);
        }
    };

    auto op = new Operation(accountId, folderId, minimum, sort);
    op->serial = ++mSerial;
    _enqueue(op);
    return op->serial;
}


quint64 ServiceActionManager::retrieveMessagePart(const QMailMessagePart::Location &location)
{
    class Operation : public OperationContext
    {
        QMailMessagePart::Location partLocation;
    public:
        Operation(const QMailMessagePart::Location &location)
          : partLocation (location) { priority =  High; }

        QMailMessageIdList messageIds() const { return QMailMessageIdList() << partLocation.containingMessageId(); }

        virtual QMailMessagePart::Location messagePartLocation() const { return partLocation; }

        void exec(QMailMessageServer *server)
        {
            server->retrieveMessagePart(serial, partLocation);
        }
    };

    Q_ASSERT (location.isValid());
    auto op = new Operation(location);
    op->serial = ++mSerial;
    mMessageIdsCache[location.containingMessageId()] << op->serial;
    mMessageLocationsCache[location.toString(true)] << op->serial;
    _enqueue(op);
    return op->serial;
}


quint64 ServiceActionManager::retrieveMessages(const QMailMessageIdList &message_ids, QMailRetrievalAction::RetrievalSpecification retrival_spec)
{
    class Operation : public OperationContext
    {
        QMailMessageIdList _messageIds;
        QMailRetrievalAction::RetrievalSpecification spec;
    public:
        Operation(const QMailMessageIdList &message_ids, QMailRetrievalAction::RetrievalSpecification retrival_spec)
          : _messageIds (message_ids), spec (retrival_spec) { priority =  High; }

        virtual QMailMessageIdList messageIds() const { return _messageIds; }

        void exec(QMailMessageServer *server)
        {
            server->retrieveMessages(serial, _messageIds, spec);
        }
    };

    auto op = new Operation(message_ids, retrival_spec);
    op->serial = ++mSerial;
    foreach (const QMailMessageId &id, message_ids)
        mMessageIdsCache[id] << op->serial;
    _enqueue(op);
    return op->serial;
}


ServiceActionManager::OperationInfo * ServiceActionManager::operationInfo(quint64 serial) const
{
    if (mCurrent && mCurrent->serial == serial)
        return mCurrent;

    // qFind
    QList<OperationContext*>::const_iterator it = mQueue.constBegin();
    while (mQueue.constEnd() != it && (*it)->serial != serial) ++it;
    if (mQueue.constEnd() == it)
        return NULL;

    return (*it);
}


void ServiceActionManager::on_activityChanged(quint64 serial, QMailServiceAction::Activity activity)
{
    static const char *activity_str[] = { "Pending", "InProgress", "Successful", "Failed" };
    qDebug() << "@ServiceActionManager::on_activityChanged: [" << serial << "]"
             << activity_str[activity];

    if (NULL == mCurrent || mCurrent->serial != serial)
        return;

    switch (activity) {

    case QMailServiceAction::Successful:
    case QMailServiceAction::Failed:
        if (QMailServiceAction::Status::ErrInternalStateReset == mCurrent->status.errorCode) {
            qWarning() << "@ServiceActionManager::on_activityChanged:"
                       << "restarting operation [" << serial << "]"
                       << "after an ErrInternalStateReset";
            mCurrent->exec(mServer);
            Q_ASSERT (mCurrent->serial != 0);
        }
        else {
//            quint64 serial = mCurrent->serial;
            if (mQueue.contains(mCurrent)) {  // operation was rescheduled
                mCurrent = NULL;
                emit activityChanged(serial, QMailServiceAction::Pending);
            }
            else {
                _removeFromMessageIdsCache(mCurrent->serial);
                delete mCurrent;
                mCurrent = NULL;
                emit activityChanged(serial, activity);
            }

            if (!mQueue.empty()) {
                mCurrent = mQueue.takeFirst();
                mCurrent->exec(mServer);
                Q_ASSERT (mCurrent->serial != 0);
            }
        }
        break;

    case QMailServiceAction::Pending:
    case QMailServiceAction::InProgress:
        emit activityChanged(mCurrent->serial, activity);
        break;

    default:
        qCritical() << Q_FUNC_INFO << activity;
        Q_ASSERT (false);
    }
}


void ServiceActionManager::on_connectivityChanged(quint64 serial, QMailServiceAction::Connectivity c)
{
    if (NULL == mCurrent || mCurrent->serial != serial)
        return;

    emit connectivityChanged(serial, c);
}


void ServiceActionManager::on_progressChanged(quint64 serial, uint value, uint total)
{
    if (NULL == mCurrent || mCurrent->serial != serial)
        return;

    emit progressChanged(serial, value, total);
}


void ServiceActionManager::on_statusChanged(quint64 serial, const QMailServiceAction::Status &s)
{
    qDebug() << "@ServiceActionManager::on_statusChanged:" << "[" << serial << "]" << s;

    if (NULL == mCurrent || mCurrent->serial != serial)
        return;

    mCurrent->status = s;
    emit statusChanged(serial, s);
}


void ServiceActionManager::_enqueue(OperationContext *operation)
{
    /// TODO: check for duplicates, is it needed? here?

    Q_ASSERT (0 != operation->serial);

    if (NULL == mCurrent) {
        Q_ASSERT (mQueue.isEmpty());
        mCurrent = operation;
        mCurrent->exec(mServer);
        Q_ASSERT (mCurrent->serial != 0);
    }
    else {
        if (mCurrent->priority < operation->priority) {
            mCurrent->cancelOperation(mServer);
            mQueue.prepend(mCurrent);
        }

        for (int i=0; i < mQueue.count(); ++i) {
            if (mQueue[i]->priority < operation->priority) {
                mQueue.insert(i, operation);
                emit activityChanged(operation->serial, QMailServiceAction::Pending);
                return;
            }
        }

        mQueue.append(operation);
    }

    emit activityChanged(operation->serial, QMailServiceAction::Pending);
}


void ServiceActionManager::_removeFromMessageIdsCache(quint64 serial)
{
    foreach (const QMailMessageId &id, mMessageIdsCache.keys()) {
        mMessageIdsCache[id].removeAll(serial);
        if (mMessageIdsCache[id].isEmpty())
            mMessageIdsCache.remove(id);
    }

    foreach (const QMailMessagePartContainer::Location &location, mMessageLocationsCache.keys()) {
        const QString &location_str = location.toString(true);
        mMessageLocationsCache[location_str].removeAll(serial);
        if (mMessageLocationsCache[location_str].isEmpty())
            mMessageLocationsCache.remove(location_str);
    }
}
