#ifndef SERVICEACTIONMANAGER_H
#define SERVICEACTIONMANAGER_H



#include <QObject>
#include <qmfclient/qmailserviceaction.h>
//#include <qmfclient/qmailmessage.h>

class QMailMessageServer;
class OperationContext;



/**
 * Why ServiceActionManager:
 *
 * 1. Avoid concurent service actions (queue)
 *    1a. priotirized queue.
 * 2. Reuse of QMailServiceAction objects (pools)
 * 3. Removal of duplicated
 * 4. Ability to monitor states changes and progress far *all* service actions.
 *    Note, usefulness of QMailActionObserver/QMailActionInfo have to be
 *    investigated, but at least it doesn't allow to cacncel.
 *
 * QMailStorageAction, QMailRetrievalAction, QMailTransmitAction,
 */

class ServiceActionManager : public QObject
{
    Q_OBJECT

    explicit ServiceActionManager(QObject *parent=NULL);

public:
    class OperationInfo
    {
    public:
        virtual QMailMessageIdList messageIds() const = 0;
        virtual QMailMessagePart::Location messagePartLocation() const = 0;
    };
    static ServiceActionManager *instance();

signals:
    void activityChanged(quint64, QMailServiceAction::Activity a);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity c);
    void progressChanged(quint64, uint value, uint total);
    void statusChanged(quint64, const QMailServiceAction::Status &s);

public:
    /// QMailServiceAction
    void cancelOperation(quint64);
    /// QMailStorageAction
//    quint64 copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
//    quint64 createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
//    quint64 deleteFolder(const QMailFolderId &folderId);
//    quint64 deleteMessages(const QMailMessageIdList &ids);
//    quint64 discardMessages(const QMailMessageIdList &ids);
//    quint64 flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);
//    quint64 moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
//    quint64 renameFolder(const QMailFolderId &folderId, const QString &name);
    /// QMailRetrievalAction
//    quint64 exportUpdates(const QMailAccountId &accountId);
    quint64 retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending=true);
    quint64 retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum=0, const QMailMessageSortKey &sort=QMailMessageSortKey());
    quint64 retrieveMessagePart(const QMailMessagePart::Location &partLocation);
//    quint64 retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);
//    quint64 retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    quint64 retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec=QMailRetrievalAction::MetaData);
//    quint64 synchronize(const QMailAccountId &accountId, uint minimum);
    /// QMailTransmitAction
//    quint64 transmitMessages(const QMailAccountId &accountId);

    /// for progress display
    inline QList<quint64> operations(const QMailMessageId &message_id) const
    {
        static const QList<quint64> NULL_OPLIST;
        return mMessageIdsCache.value(message_id, NULL_OPLIST);
    }

    inline QList<quint64> operations(const QMailMessagePartContainer::Location &location) const
    {
        static const QList<quint64> NULL_OPLIST;
        return mMessageLocationsCache.value(location.toString(true), NULL_OPLIST);
    }

    OperationInfo * operationInfo(quint64 serial) const;

private slots:
    void on_activityChanged(quint64, QMailServiceAction::Activity);
    void on_connectivityChanged(quint64, QMailServiceAction::Connectivity);
    void on_progressChanged(quint64, uint,uint);
    void on_statusChanged(quint64, const QMailServiceAction::Status &);

private:
    QMailMessageServer *mServer;
    OperationContext *mCurrent;
    QList<OperationContext *> mQueue;
    QHash<QMailMessageId, QList<quint64> > mMessageIdsCache;
    QHash<QString, QList<quint64> > mMessageLocationsCache;
    quint64 mSerial;

    void _enqueue(OperationContext *);
    void _removeFromMessageIdsCache(quint64 serial);
};



#endif // SERVICEACTIONMANAGER_H
