
// project
#include "serviceactionmanager.h"

#include "messagelistmodel.h"



#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }
#define DISCONNECT(a,b,c,d) if (!QObject::disconnect(a,b,c,d)) { Q_ASSERT (false); }




models::MessageListModel::MessageListModel(QObject* parent)
  : QMailMessageListModel (parent)
{
    CONNECT (ServiceActionManager::instance(), SIGNAL(activityChanged(quint64,QMailServiceAction::Activity)),
             this, SLOT(on_activityChanged(quint64,QMailServiceAction::Activity)));
}


QVariant models::MessageListModel::data(const QModelIndex &index, int role) const
{
    switch (role) {

    case ProgressInfoRole: {

        const QMailMessageId &id = idFromIndex(index);
        Q_ASSERT (id.isValid());

        if (mProgressInfoCache.contains(id))
            return mProgressInfoCache[id];

        const QList<quint64> &operations = ServiceActionManager::instance()->operations(id);
        if (operations.isEmpty())
            return QVariant();

        if (mIdsCache.isEmpty())
            connectCache();

        foreach (quint64 serial, operations)
            mIdsCache[serial] = id;

        return mProgressInfoCache[id] = ProgressInfo(operations[0]);
    }

    default:
        return QMailMessageListModel::data(index, role);
    }
}


void models::MessageListModel::connectCache() const
{
    CONNECT (ServiceActionManager::instance(), SIGNAL(progressChanged(quint64,uint,uint)),
             this, SLOT(on_progressChanged(quint64,uint,uint)));
    CONNECT (this, SIGNAL(modelReset()),
             this, SLOT(on_modelReset()));
    CONNECT (this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
             this, SLOT(on_rowsAboutToBeRemoved(QModelIndex,int,int)));
}


void models::MessageListModel::disconnectCache() const
{
    DISCONNECT (ServiceActionManager::instance(), SIGNAL(progressChanged(quint64,uint,uint)),
               this, SLOT(on_progressChanged(quint64,uint,uint)));
    DISCONNECT (this, SIGNAL(modelReset()),
               this, SLOT(on_modelReset()));
    DISCONNECT (this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
               this, SLOT(on_rowsAboutToBeRemoved(QModelIndex,int,int)));
}


void models::MessageListModel::on_modelReset()
{
    if (!mIdsCache.isEmpty())
        disconnectCache();
    mIdsCache.clear();
    mProgressInfoCache.clear();
}


void models::MessageListModel::on_rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    for (int i = first; i <= last; i++) {

        const QMailMessageId &id = idFromIndex(index(i, 0, parent));
        Q_ASSERT (id.isValid());
        quint64 serial = mProgressInfoCache.value(id).serial();
        mProgressInfoCache.remove(id);
        mIdsCache.remove(serial);
    }

    if (mIdsCache.isEmpty())
        disconnectCache();
}


void models::MessageListModel::on_progressChanged(quint64 serial, uint value, uint total)
{
    if (!mIdsCache.contains(serial))
        return;

    const QMailMessageId &id = mIdsCache[serial];
    mProgressInfoCache[id].setInfo(serial, value, total);

    const QModelIndex &index = indexFromId(id);
    emit dataChanged(index, index);
}


void models::MessageListModel::on_activityChanged(quint64 serial, QMailServiceAction::Activity activity)
{
    switch (activity) {

    case QMailServiceAction::Pending: {

        if (mIdsCache.contains(serial))
            return;

        /// FIXME: how to get message_id from serial?
        const ServiceActionManager::OperationInfo *operation = ServiceActionManager::instance()->operationInfo(serial);
        if (!operation)
            return;

        const auto messageIds = operation->messageIds();
        if (messageIds.isEmpty())
            return;

        const QMailMessageId &id = messageIds.first();  /// FIXME!!!!!
        if (!id.isValid())
            return;

        const QModelIndex &index = indexFromId(id);
        if (!index.isValid())
            return;

        connectCache();
        mIdsCache[serial] = id;
        emit dataChanged(index, index);
    }   break;

    case QMailServiceAction::Successful:
    case QMailServiceAction::Failed: {

        if (!mIdsCache.contains(serial))
            return;

        mProgressInfoCache.remove(mIdsCache[serial]);
        mIdsCache.remove(serial);

        if (mIdsCache.isEmpty())
            disconnectCache();

        const QModelIndex &index = indexFromId(mIdsCache[serial]);
        emit dataChanged(index, index);
    }   break;

    default:
        return;
    }
}
