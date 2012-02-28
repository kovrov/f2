// Qt
#include <QMap>
#include <QtAlgorithms>

// QMF
#include <qmfclient/qmailstore.h>  // QMailStore

// project
#include "folderlistmodel.h"


#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }


namespace models {
    namespace internal
    {
        struct TreeItem
        {
            TreeItem(const QMailFolderId &folder_id, const QString &folder_name, TreeItem *parent_ptr)
              : parent (parent_ptr),
                id (folder_id),
                name (folder_name)
            {}

            ~TreeItem() { qDeleteAll(children); }

            QList<TreeItem*> children;
            TreeItem *parent;

            QMailFolderId id;
            QString name;
        };
    }
}

using namespace models::internal;



namespace
{
    TreeItem* find_node(TreeItem *tree_item, const QMailFolderId &id)
    {
        Q_ASSERT (NULL != tree_item);

        if (tree_item->id == id)
            return tree_item;

        foreach (TreeItem *child_item, tree_item->children) {
            if (TreeItem *res = find_node(child_item, id))
                return res;
        }

        return NULL;
    }

    void recursive_setup(const QMailAccountId &account_id, TreeItem *parent)
    {
        QMailFolderKey key
                = QMailFolderKey(QMailFolderKey::parentAccountId(account_id))
                & QMailFolderKey(QMailFolderKey::parentFolderId(parent->id))
                & QMailFolderKey(QMailFolderKey::status(QMailFolder::NonMail,
                                                        QMailDataComparator::Excludes));

        foreach (const QMailFolderId &id, QMailStore::instance()->queryFolders(key)) {

            const QMailFolder folder(id);
            TreeItem *item = new TreeItem(folder.id(), folder.displayName(), parent);
            parent->children.append(item);
            recursive_setup(account_id, item);
        }
    }
}



models::FolderListModel::FolderListModel(QObject *parent)
  : QAbstractItemModel(parent),
    mRootItem (NULL)
{
    QMailStore *store = QMailStore::instance();

    CONNECT (store, SIGNAL(accountsRemoved(QMailAccountIdList)),
              this, SLOT(onAccountsRemoved(QMailAccountIdList)));
    CONNECT (store, SIGNAL(foldersAdded(QMailFolderIdList)),
              this, SLOT(onFoldersAdded(QMailFolderIdList)));
    CONNECT (store, SIGNAL(foldersUpdated(QMailFolderIdList)),
              this, SLOT(onFoldersUpdated(QMailFolderIdList)));
    CONNECT (store, SIGNAL(foldersRemoved(QMailFolderIdList)),
              this, SLOT(onFoldersRemoved(QMailFolderIdList)));
}


models::FolderListModel::~FolderListModel()
{
    delete mRootItem;
}


void models::FolderListModel::setAccountId(const QMailAccountId &id)
{
    // WTF?
    {
        if (mRootItem) {
            Q_ASSERT (QMailFolder(mRootItem->id).parentAccountId() == QMailAccountId());
        }
        if (mRootItem && QMailFolder(mRootItem->id).parentAccountId() == id)
            return;
    }

    QMailAccount account(id);
    if (!account.id().isValid()) {
        if (mRootItem) {
            delete mRootItem;
            mRootItem = NULL;
            emit layoutChanged();
        }
        return;
    }

    emit layoutAboutToBeChanged();
    if (mRootItem)
        delete mRootItem;
    mRootItem = new TreeItem(QMailFolderId(), account.name(), NULL);
    recursive_setup(id, mRootItem);
    emit layoutChanged();
}


QMailFolderId models::FolderListModel::idFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QMailFolderId();

    return static_cast<TreeItem*>(index.internalPointer())->id;
}


QModelIndex	models::FolderListModel::indexFromId(const QMailFolderId &id) const
{
    if (TreeItem *item = find_node(mRootItem, id))
        return createIndex(item->parent? item->parent->children.indexOf(item) : 0,
                           0,
                           item);
    return QModelIndex();
}


int models::FolderListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (NULL == mRootItem)
        return 0;

    TreeItem *parent_item = parent.isValid()
            ? static_cast<TreeItem*>(parent.internalPointer())
            : mRootItem;

    return parent_item->children.count();
}


int models::FolderListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    return 1;
}


QVariant models::FolderListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {

    case Qt::DisplayRole:
        return static_cast<TreeItem*>(index.internalPointer())->name;

    case FolderIdRole:
        return static_cast<TreeItem*>(index.internalPointer())->id;

    default:
        return QVariant();
    }
}


Qt::ItemFlags models::FolderListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QModelIndex models::FolderListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parent_item = parent.isValid()
            ? static_cast<TreeItem*>(parent.internalPointer())
            : mRootItem;

    Q_ASSERT (parent_item);

    if (TreeItem *tree_item = parent_item->children.value(row))
        return createIndex(row, column, tree_item);

    return QModelIndex();
}


QModelIndex models::FolderListModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *parent_item = static_cast<TreeItem*>(index.internalPointer())->parent;
    if (parent_item == mRootItem)
        return QModelIndex();

    int row = parent_item->parent
            ? parent_item->parent->children.indexOf(parent_item)
            : 0;
    return createIndex(row, 0, parent_item);
}


void models::FolderListModel::onAccountsRemoved(const QMailAccountIdList &list)
{
    //qWarning() << "### models::FolderListModel::onAccountsRemoved:" << list;

    if (NULL == mRootItem)
        return;

    // WTF?
    {
        //qWarning() << "### mRootItem.id -> parentAccountId" << QMailFolder(mRootItem->id).parentAccountId();
        Q_ASSERT (list.indexOf(QMailFolder(mRootItem->id).parentAccountId()) == -1);
        if (list.indexOf(QMailFolder(mRootItem->id).parentAccountId()) == -1)
            return;
    }

    //qWarning() << "  # models::FolderListModel::onAccountsRemoved:" << mRootItem->id << mRootItem->name;
    delete mRootItem;
    mRootItem = NULL;
    emit layoutChanged();
}


void models::FolderListModel::onFoldersAdded(const QMailFolderIdList &list)
{
    //qWarning() << "### models::FolderListModel::onFoldersAdded:" << list;

    QMap<TreeItem*, QList<TreeItem*> > new_folders;

    foreach (const QMailFolderId &id, list) {

        const QMailFolder folder(id);
        TreeItem *parent_item = find_node(mRootItem, folder.parentFolderId());
        if (NULL == parent_item)
            continue;

        new_folders[parent_item] << new TreeItem(folder.id(), folder.displayName(), parent_item);
    }

    foreach (TreeItem *parent_item, new_folders.keys()) {

        //qWarning() << "  # models::FolderListModel::onFoldersAdded: parent:" << parent_item->id << parent_item->name;

        const QModelIndex &parent_index = (parent_item->parent)
                ? createIndex(parent_item->parent->children.indexOf(parent_item), 0, parent_item)
                : QModelIndex();
        int first = parent_item->children.count();
        int last = first + new_folders[parent_item].count();
        beginInsertRows(parent_index, first, last);

        const QMailAccountId &folder_id = QMailFolder(mRootItem->id).parentAccountId();
        Q_ASSERT (folder_id == QMailAccountId());
        foreach (TreeItem *item, new_folders[parent_item]) {
            //qWarning() << "    models::FolderListModel::onFoldersAdded:" << item->id << item->name;
            Q_ASSERT (item->parent == parent_item);
            item->parent->children.append(item);
            recursive_setup(folder_id, item);
        }

        endInsertRows();
    }
}


namespace
{
    void find_all_in(const QMailFolderIdList &list, TreeItem *parent, QList<TreeItem*> *res)
    {
        if (list.indexOf(parent->id) != -1)
            res->append(parent);

        foreach (TreeItem *item, parent->children)
            find_all_in(list, item, res);
    }

    void find_roots_in(const QMailFolderIdList &list, TreeItem *parent, QList<TreeItem*> *res)
    {
        if (list.indexOf(parent->id) != -1) {
            res->append(parent);
            return;
        }

        foreach (TreeItem *item, parent->children)
            find_roots_in(list, item, res);
    }
}


void models::FolderListModel::onFoldersUpdated(const QMailFolderIdList &list)
{
    //qWarning() << "### models::FolderListModel::onFoldersUpdated:" << list;

    QList<TreeItem*> found;
    find_all_in(list, mRootItem, &found);

    foreach (TreeItem *item, found) {

        const QMailFolder folder(item->id);
        if (folder.displayName() != item->name) {

            //qWarning() << "  # models::FolderListModel::onFoldersUpdated:" << item->id << item->name << ">" << folder.displayName();
            item->name = folder.displayName();

            const QModelIndex &index = (item->parent)
                    ? createIndex(item->parent->children.indexOf(item), 0, item)
                    : QModelIndex();
            emit dataChanged(index, index);
        }
    }
}



void models::FolderListModel::onFoldersRemoved(const QMailFolderIdList &list)
{
    //qWarning() << "### models::FolderListModel::onFoldersRemoved:" << list;
    if (NULL == mRootItem)
        return;

    QList<TreeItem*> found;
    find_roots_in(list, mRootItem, &found);
    if (found.isEmpty())
        return;


    QMap<TreeItem*, QList<TreeItem*> > folders;
    foreach (TreeItem *item, found) {
        folders[item->parent] << item;
    }

    foreach (TreeItem *parent_item, folders.keys()) {

        const QModelIndex &parent_index = (parent_item->parent)
                ? createIndex(parent_item->parent->children.indexOf(parent_item), 0, parent_item)
                : QModelIndex();

        QList<int> indices;
        foreach (TreeItem *item, folders[parent_item]) {
            Q_ASSERT (item->parent == parent_item);
            indices << parent_item->children.indexOf(item);
        }
        qSort(indices);
        //qWarning() << "  # models::FolderListModel::onFoldersRemoved: qSort'ed:" << indices;

        int begin = indices.takeLast();
        int end = begin;
        do {
            if (!indices.isEmpty() && begin - 1 == indices.last()) {
                begin = indices.takeLast();
                if (!indices.isEmpty())
                    continue;
            }

            //qWarning() << "  # models::FolderListModel::onFoldersRemoved: beginRemoveRows" << begin << end;
            beginRemoveRows(parent_index, begin, end);
            auto it = parent_item->children.begin();
            parent_item->children.erase(it + begin, it + end + 1);
            endRemoveRows();

            if (!indices.isEmpty()) {
                begin = indices.takeLast();
                end = begin;
            }
        }
        while (!indices.isEmpty());
    }

    qDeleteAll(found);
}
