#ifndef ATTACHMENTLISTMODEL_H
#define ATTACHMENTLISTMODEL_H

// qt
#include <QPointer>
#include <QAbstractListModel>

// qmf
#include <qmfclient/qmailmessage.h>

// project
#include "messagemodel.h"
#include "progressinfo.h"


namespace models {


/**
 * Provides QAbstractItemModel interface to a MessageModel
 * (see http://en.wikipedia.org/wiki/Adapter_pattern)
 */
class AttachmentList : public QAbstractItemModel/*QAbstractListModel*/
{
    Q_OBJECT

public:
    enum Roles {
        SizeRole = Qt::UserRole,
        MimeTypeRole,
        IsDownloadedRole,
        LocationAsStringRole,
        LocationRole,
        SaveFolderRole,
        DownloaderRole,
        ProgressInfoRole
    };

    explicit AttachmentList(QObject *parent = 0);
    virtual ~AttachmentList() {}

    MessageModel * sourceModel();
    void setSourceModel(MessageModel *model);
//    void detach();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &/*child*/) const { return QModelIndex(); }
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const { return parent.isValid() ? 0 : 1; }

signals:

private:
    void connectCache() const;
    void disconnectCache() const;

private slots:
    void on_messageReset();
    void on_messageUpdated();

    void on_modelReset();
    void on_rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void on_progressChanged(quint64 serial, uint value, uint total);
    void on_activityChanged(quint64, QMailServiceAction::Activity);

private:
    QPointer<MessageModel>  mModel;
    struct Item { QModelIndex index; QMailMessagePart::Location location; };
    QList<const Item*> mItems;
    mutable QHash<const Item*, ProgressInfo> mProgressInfoCache;
    mutable QHash<quint64, const Item*> mIndexCache;
};



}  // namespace models

#endif // ATTACHMENTLISTMODEL_H
