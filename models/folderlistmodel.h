#ifndef FOLDERLISTMODEL_H
#define FOLDERLISTMODEL_H


#include <QAbstractItemModel>

#include <qmfclient/qmailid.h>

namespace models
{


namespace internal { struct TreeItem; }

class FolderListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        FolderIdRole = Qt::UserRole
    };

    explicit FolderListModel(QObject *parent = 0);
    virtual ~FolderListModel();

    void setAccountId(const QMailAccountId &id);
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
    internal::TreeItem *mRootItem;
};



}  // namespace models

#endif // FOLDERLISTMODEL_H
