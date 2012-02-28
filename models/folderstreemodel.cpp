// Qt
#include <QMap>
#include <QtAlgorithms>
#include <QIcon>

// QMF
#include <qmfclient/qmailstore.h>  // QMailStore

// project
#include "folderstreemodel.h"


#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }

namespace models
{

namespace internal
{
    class TreeNode
    {
    public:
        TreeNode(TreeNode *parent_ptr)
          : parent (parent_ptr)
        {}
        virtual ~TreeNode() { qDeleteAll(children); }

        QList<TreeNode*> children;
        TreeNode *parent;
    };

    struct FolderNode : public TreeNode
    {
        FolderNode(const QMailFolderId &folder_id, const QString &name, TreeNode *parent_ptr)
          : TreeNode (parent_ptr),
            displayName (name),
            id (folder_id)
        {}

        QString displayName;
        QMailFolderId id;
    };

    struct AccountNode : public TreeNode
    {
        AccountNode(const QMailAccountId &account_id, const QString &account_name, TreeNode *parent_ptr)
          : TreeNode (parent_ptr),
            name (account_name),
            id (account_id)
        {}

        QString name;
        QMailAccountId id;
    };
}

using namespace internal;


namespace
{
    AccountNode* find_node(TreeNode *item, const QMailAccountId &id)
    {
        Q_ASSERT (NULL != item);

        if (AccountNode *res = dynamic_cast<AccountNode *>(item)) {
            if (res->id == id)
                return res;
        }

        foreach (TreeNode *child, item->children) {
            if (AccountNode *res = dynamic_cast<AccountNode *>(child)) {
                if (res->id == id)
                    return res;
            }
        }

        return NULL;
    }
    FolderNode* find_node(TreeNode *item, const QMailFolderId &id)
    {
        Q_ASSERT (NULL != item);

        if (FolderNode *res = dynamic_cast<FolderNode *>(item)) {
            if (res->id == id)
                return res;
        }

        foreach (TreeNode *child_item, item->children) {
            if (FolderNode *res = find_node(child_item, id))
                return res;
        }

        return NULL;
    }

    void find_all_in(const QMailFolderIdList &list, TreeNode *parent_node, QList<FolderNode*> *res)
    {
        FolderNode *parent = dynamic_cast<FolderNode*>(parent_node);
        if (parent && list.indexOf(parent->id) != -1)
            res->append(parent);

        foreach (TreeNode *item, parent_node->children)
            find_all_in(list, item, res);
    }

    void find_roots_in(const QMailFolderIdList &list, TreeNode *parent_node, QList<FolderNode*> *res)
    {
        FolderNode *parent = dynamic_cast<FolderNode*>(parent_node);
        if (parent && list.indexOf(parent->id) != -1) {
            res->append(parent);
            return;
        }

        foreach (TreeNode *item, parent->children)
            find_roots_in(list, item, res);
    }

    void recursive_setup(const QMailAccountId &account_id, const QMailFolderId &folder_id, TreeNode *parent)
    {
        QMailFolderKey key
                = QMailFolderKey(QMailFolderKey::parentAccountId(account_id))
                & QMailFolderKey(QMailFolderKey::parentFolderId(folder_id))
                & QMailFolderKey(QMailFolderKey::status(QMailFolder::NonMail,
                                                        QMailDataComparator::Excludes));

        foreach (const QMailFolderId &id, QMailStore::instance()->queryFolders(key)) {

            const QMailFolder folder(id);
            FolderNode *item = new FolderNode(folder.id(), folder.displayName(), parent);
            parent->children.append(item);
            recursive_setup(account_id, folder.id(), item);
        }
    }
}



FoldersTree::FoldersTree(QObject *parent)
  : QAbstractItemModel (parent),
    mRootItem (new TreeNode(NULL))
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

    /// TODO: exclude disabled accounts
    foreach (const QMailAccountId &id, store->queryAccounts()) {

        const QMailAccount account(id);
        AccountNode *item = new AccountNode(account.id(), account.name(), mRootItem);
        mRootItem->children.append(item);
        recursive_setup(account.id(), QMailFolderId(), item);
    }
}


FoldersTree::~FoldersTree()
{
    delete mRootItem;
}


QMailFolderId FoldersTree::idFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QMailFolderId();

    TreeNode *node = static_cast<TreeNode*>(index.internalPointer());
    FolderNode *folder_node = dynamic_cast<FolderNode*>(node);
    if (NULL == folder_node)
        return QMailFolderId();

    return folder_node->id;
}


QModelIndex	FoldersTree::indexFromId(const QMailFolderId &id) const
{
    if (FolderNode *item = find_node(mRootItem, id))
        return createIndex(item->parent? item->parent->children.indexOf(item) : 0,
                           0,
                           item);
    return QModelIndex();
}


int FoldersTree::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (NULL == mRootItem)
        return 0;

    TreeNode *parent_item = parent.isValid()
            ? static_cast<TreeNode*>(parent.internalPointer())
            : mRootItem;

    return parent_item->children.count();
}


int FoldersTree::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    return 1;
}


QVariant FoldersTree::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeNode *base_node = static_cast<TreeNode*>(index.internalPointer());

    if (FolderNode *node = dynamic_cast<FolderNode*>(base_node)) {

        switch (role) {

        case Qt::DisplayRole:
            return node->displayName;
        case Qt::DecorationRole:
            return QIcon::fromTheme("folder");
        case FolderIdRole:
            return node->id;
        default:
            return QVariant();
        }
    }

    if (AccountNode *node = dynamic_cast<AccountNode*>(base_node)) {

        switch (role) {

        case Qt::DisplayRole:
            return node->name;
        default:
            return QVariant();
        }
    }

    return QVariant();
}


Qt::ItemFlags FoldersTree::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags res = 0;
    TreeNode* item = static_cast<TreeNode*>(index.internalPointer());
    if (dynamic_cast<const FolderNode*>(item))
        res |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return res;
}


QModelIndex FoldersTree::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeNode *parent_item = parent.isValid()
            ? static_cast<TreeNode*>(parent.internalPointer())
            : mRootItem;

    Q_ASSERT (parent_item);

    if (TreeNode *tree_item = parent_item->children.value(row))
        return createIndex(row, column, tree_item);

    return QModelIndex();
}


QModelIndex FoldersTree::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeNode *parent_item = static_cast<TreeNode*>(index.internalPointer())->parent;
    if (parent_item == mRootItem)
        return QModelIndex();

    int row = parent_item->parent
            ? parent_item->parent->children.indexOf(parent_item)
            : 0;
    return createIndex(row, 0, parent_item);
}


void FoldersTree::onAccountsRemoved(const QMailAccountIdList &list)
{
    //qWarning() << "### FoldersTreeModel::onAccountsRemoved:" << list;

    if (NULL == mRootItem)
        return;

    foreach (TreeNode *item, mRootItem->children) {
        AccountNode *account_item = dynamic_cast<AccountNode*>(item);
        Q_ASSERT (account_item);
        if (!account_item || list.indexOf(account_item->id) == -1)
            continue;

        mRootItem->children.removeOne(account_item);
        delete account_item;
        emit layoutChanged();
    }
}


void FoldersTree::onFoldersAdded(const QMailFolderIdList &list)
{
    /// FIXME: multi-root nodes (one per account)

    QMap<TreeNode*, QList<FolderNode*> > new_folders;

    //qWarning() << "### FoldersTreeModel::onFoldersAdded:";
    foreach (const QMailFolderId &id, list) {

        const QMailFolder folder(id);
        //qWarning() << "  #" << folder.displayName() << "(" << folder.parentFolderId() << ")";
        TreeNode *parent_item = folder.parentFolderId().isValid()
                ? static_cast<TreeNode *>(find_node(mRootItem, folder.parentFolderId()))
                : static_cast<TreeNode *>(find_node(mRootItem, folder.parentAccountId()));
        if (NULL == parent_item)
            continue;

        new_folders[parent_item] << new FolderNode(folder.id(), folder.displayName(), parent_item);
    }

    foreach (TreeNode *parent_item, new_folders.keys()) {

        //qWarning() << "  # FoldersTreeModel::onFoldersAdded: parent:" << parent_item/*->id << parent_item->displayName*/;

        const QModelIndex &parent_index = (parent_item->parent)
                ? createIndex(parent_item->parent->children.indexOf(parent_item), 0, parent_item)
                : QModelIndex();
        int first = parent_item->children.count();
        int last = first + new_folders[parent_item].count();
        beginInsertRows(parent_index, first, last);

        foreach (FolderNode *item, new_folders[parent_item]) {
            //qWarning() << "    FoldersTreeModel::onFoldersAdded:" << item->id << item->displayName;
            Q_ASSERT (item->parent == parent_item);
            item->parent->children.append(item);
            QMailFolder folder(item->id);
            recursive_setup(folder.parentAccountId(), folder.id(), item);
        }

        endInsertRows();
    }
}


void FoldersTree::onFoldersUpdated(const QMailFolderIdList &list)
{
    //qWarning() << "### FoldersTreeModel::onFoldersUpdated:" << list;

    QList<FolderNode*> found;
    find_all_in(list, mRootItem, &found);

    foreach (FolderNode *item, found) {

        const QMailFolder folder(item->id);
        if (folder.displayName() != item->displayName) {

            //qWarning() << "  # FoldersTreeModel::onFoldersUpdated:" << item->id << item->displayName << ">" << folder.displayName();
            item->displayName = folder.displayName();

            const QModelIndex &index = (item->parent)
                    ? createIndex(item->parent->children.indexOf(item), 0, item)
                    : QModelIndex();
            emit dataChanged(index, index);
        }
    }
}


void FoldersTree::onFoldersRemoved(const QMailFolderIdList &list)
{
    //qWarning() << "### FoldersTreeModel::onFoldersRemoved:" << list;
    if (NULL == mRootItem)
        return;

    QList<FolderNode*> found;
    find_roots_in(list, mRootItem, &found);
    if (found.isEmpty())
        return;

    QMap<TreeNode*, QList<FolderNode*> > folders;
    foreach (FolderNode *item, found) {
        folders[item->parent] << item;
    }

    foreach (TreeNode *parent_item, folders.keys()) {

        const QModelIndex &parent_index = (parent_item->parent)
                ? createIndex(parent_item->parent->children.indexOf(parent_item), 0, parent_item)
                : QModelIndex();

        QList<int> indices;
        foreach (FolderNode *item, folders[parent_item]) {
            Q_ASSERT (item->parent == parent_item);
            indices << parent_item->children.indexOf(item);
        }
        qSort(indices);
        //qWarning() << "  # FoldersTreeModel::onFoldersRemoved: qSort'ed:" << indices;

        int begin = indices.takeLast();
        int end = begin;
        do {
            if (!indices.isEmpty() && begin - 1 == indices.last()) {
                begin = indices.takeLast();
                if (!indices.isEmpty())
                    continue;
            }

            //qWarning() << "  # FoldersTreeModel::onFoldersRemoved: beginRemoveRows" << begin << end;
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



}  // namespace models
