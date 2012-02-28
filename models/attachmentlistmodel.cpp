// project
#include "serviceactionmanager.h"

#include "attachmentlistmodel.h"



#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }
#define DISCONNECT(a,b,c,d) if (!QObject::disconnect(a,b,c,d)) { Q_ASSERT (false); }


namespace models {



AttachmentList::AttachmentList(QObject *parent)
  : QAbstractItemModel (parent)
{
    CONNECT (ServiceActionManager::instance(), SIGNAL(activityChanged(quint64,QMailServiceAction::Activity)),
             this, SLOT(on_activityChanged(quint64,QMailServiceAction::Activity)));
}


MessageModel * AttachmentList::sourceModel()
{
    return mModel;
}


void AttachmentList::setSourceModel(MessageModel *model)
{
    if (!mModel.isNull()) {
        DISCONNECT (mModel, SIGNAL(modelReset()), this, SLOT(on_messageReset()));
        DISCONNECT (mModel, SIGNAL(updated()), this, SLOT(on_messageUpdated()));
    }

    mModel = model;
    CONNECT (mModel, SIGNAL(modelReset()), this, SLOT(on_messageReset()));
    CONNECT (mModel, SIGNAL(updated()), this, SLOT(on_messageUpdated()));

    on_messageReset();
}


int AttachmentList::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    return mItems.count();
}


QVariant AttachmentList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Item *item = static_cast<const Item *>(index.internalPointer());
    Q_ASSERT (item);
    Q_ASSERT (!mModel.isNull());

    switch (role) {

    case Qt::DisplayRole:
        return mModel->message().partAt(item->location).displayName();

    case MimeTypeRole:
        return QString(mModel->message().partAt(item->location).contentType().content());

    case SizeRole: {
        const QMailMessagePart &part = mModel->message().partAt(item->location);

        if (part.contentDisposition().size() != -1)
            return part.contentDisposition().size();

        // If size is -1 (unknown) try finding out attachment's body size
        if (part.contentAvailable())
            return part.hasBody() ? part.body().length() : 0;

        return -1;
    }
    case LocationAsStringRole:
        return item->location.toString(true);

    case LocationRole:
        return qVariantFromValue(item->location);

    case ProgressInfoRole: {

        if (mProgressInfoCache.contains(item))
            return mProgressInfoCache[item];

        const QList<quint64> &operations = ServiceActionManager::instance()->operations(item->location);
        if (operations.isEmpty())
            return QVariant();

        if (mIndexCache.isEmpty())
            connectCache();

        foreach (quint64 serial, operations)
            mIndexCache[serial] = item;

        return mProgressInfoCache[item] = ProgressInfo(operations[0]);
    }
    default:
        return QVariant();
    }
}


QModelIndex AttachmentList::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED (column);
    Q_UNUSED (parent);
    return mItems[row]->index;
}


void AttachmentList::connectCache() const
{
    CONNECT (ServiceActionManager::instance(), SIGNAL(progressChanged(quint64,uint,uint)),
             this, SLOT(on_progressChanged(quint64,uint,uint)));
    CONNECT (this, SIGNAL(modelReset()),
             this, SLOT(on_modelReset()));
    CONNECT (this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
             this, SLOT(on_rowsAboutToBeRemoved(QModelIndex,int,int)));
}


void AttachmentList::disconnectCache() const
{
    DISCONNECT (ServiceActionManager::instance(), SIGNAL(progressChanged(quint64,uint,uint)),
               this, SLOT(on_progressChanged(quint64,uint,uint)));
    DISCONNECT (this, SIGNAL(modelReset()),
               this, SLOT(on_modelReset()));
    DISCONNECT (this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
               this, SLOT(on_rowsAboutToBeRemoved(QModelIndex,int,int)));
}


void AttachmentList::on_messageReset()
{
    beginResetModel();

    while (!mItems.isEmpty())
         delete mItems.takeFirst();

    const QList<QMailMessagePart::Location> &locations = mModel->message().findAttachmentLocations();
    for (int i=0; i < locations.count(); ++i) {
        Item *item = new Item;
        item->location = locations[i];
        item->index = createIndex(i, 0, item);
        mItems << item;
    }

    endResetModel();
}


void AttachmentList::on_messageUpdated()
{
    /** TODO:
     * beginInsertRows/insertRows
     * beginRemoveRows/removeRows
     * dataChanged
     */
}


void AttachmentList::on_modelReset()
{
    if (!mIndexCache.isEmpty())
        disconnectCache();
    mIndexCache.clear();
    mProgressInfoCache.clear();
}


void AttachmentList::on_rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED (parent);
    Q_ASSERT (!mModel.isNull());
    Q_ASSERT (0 <= first && last < mItems.count());

    for (int i = first; i <= last; i++) {

        const Item *item = mItems[i];
        quint64 serial = mProgressInfoCache.value(item).serial();
        mProgressInfoCache.remove(item);
        mIndexCache.remove(serial);
    }

    if (mIndexCache.isEmpty())
        disconnectCache();
}


void AttachmentList::on_progressChanged(quint64 serial, uint value, uint total)
{
    if (!mIndexCache.contains(serial))
        return;

    const Item *item = mIndexCache[serial];
    mProgressInfoCache[item].setInfo(serial, value, total);
    emit dataChanged(item->index, item->index);
}


void AttachmentList::on_activityChanged(quint64 serial, QMailServiceAction::Activity activity)
{
    switch (activity) {

    case QMailServiceAction::Pending: {

        if (mIndexCache.contains(serial))
            return;

        const auto *operation = ServiceActionManager::instance()->operationInfo(serial);
        /// FIXME: Q_ASSERT (operation);
        if (!operation)
            return;

        const QMailMessagePart::Location &location = operation->messagePartLocation();
        if (!location.isValid() || location.containingMessageId() != mModel->message().id())
            return;

        const QString &location_str = location.toString(true);
        foreach (const Item *item, mItems) {
            if (item->location.toString(true) == location_str) {
                connectCache();
                mIndexCache[serial] = item;
                emit dataChanged(item->index, item->index);
                return;
            }
        }
    }   return;

    case QMailServiceAction::Successful:
    case QMailServiceAction::Failed: {

        if (!mIndexCache.contains(serial))
            return;

        const Item *item = mIndexCache[serial];
        mProgressInfoCache.remove(item);
        mIndexCache.remove(serial);

        if (mIndexCache.isEmpty())
            disconnectCache();

        emit dataChanged(item->index, item->index);
    }   return;

    default: ;
    }
}

}  // namespace models
