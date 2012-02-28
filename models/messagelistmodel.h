#ifndef MESSAGELISTMODEL_H
#define MESSAGELISTMODEL_H


#include <qmfclient/qmailmessagelistmodel.h>  // QMailMessageListModel
#include <qmfclient/qmailserviceaction.h>  // QMailServiceAction

#include "progressinfo.h"



namespace models {

/**
 MessageListModel provides support for accessing progress info

 # listen service action manager for progress and update view if needed

 # for that it have to maintain few caches:

   * service action operations (serials) mapped to ids.
     this is for filtering progress emitted by service action manager

   * ids mapped to progress info

*/

class MessageListModel : public QMailMessageListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        ProgressInfoRole = Qt::UserRole + 32
    };

    MessageListModel(QObject* parent = 0);
    virtual QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

private:
    void connectCache() const;
    void disconnectCache() const;

private slots:
    void on_modelReset();
    void on_rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void on_progressChanged(quint64 serial, uint value, uint total);
    void on_activityChanged(quint64, QMailServiceAction::Activity);

private:
    mutable QHash<QMailMessageId, ProgressInfo> mProgressInfoCache;
    mutable QHash<quint64, QMailMessageId> mIdsCache;
};



}  // namespace models

#endif // MESSAGELISTMODEL_H
