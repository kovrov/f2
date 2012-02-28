#ifndef FOLDERSTREEMODEL_H
#define FOLDERSTREEMODEL_H



#include <QAbstractItemModel>

#include <qmfclient/qmailid.h>


namespace models
{
    namespace internal { struct TreeNode; }

    class FoldersTree : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        enum Roles {
            FolderIdRole = Qt::UserRole
        };

        explicit FoldersTree(QObject *parent = 0);
        virtual ~FoldersTree();

        QMailFolderId idFromIndex(const QModelIndex& index) const;
        QModelIndex	indexFromId(const QMailFolderId &id) const;

        QVariant data(const QModelIndex &index, int role) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;

    private slots:
        void onAccountsRemoved(const QMailAccountIdList &);
        void onFoldersAdded(const QMailFolderIdList &);
        void onFoldersUpdated(const QMailFolderIdList &);
        void onFoldersRemoved(const QMailFolderIdList &);

    private:
        internal::TreeNode *mRootItem;
    };
}


#endif // FOLDERSTREEMODEL_H
