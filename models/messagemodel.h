#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H



#include <QObject>

#include <qmfclient/qmailmessage.h>
#include <qmfclient/qmailmessagekey.h>
#include <qmfclient/qmailserviceaction.h>


namespace models {


class MessageModel : public QObject
{
    Q_OBJECT
public:
    explicit MessageModel(QObject *parent = 0);
    void setMessageId(const QMailMessageId &id);
    const QMailMessage & message() const { return mMessage; }
    QMailMessage & message() { return mMessage; }

signals:
    void modelReset();
    void updated();

private slots:
    void on_messageContentsModified(const QMailMessageIdList &ids);
    void on_messageDataUpdated(const QMailMessageMetaDataList &list);
    void on_messagePropertyUpdated(const QMailMessageIdList &ids, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data);
    void on_messageStatusUpdated(const QMailMessageIdList &ids, quint64, bool);
    void on_messagesUpdated(const QMailMessageIdList &ids);
    void on_activityChanged(quint64 serial, QMailServiceAction::Activity a);

private:
    QMailMessage mMessage;
    QList<quint64> mOperations;
};



}  // namespace models

#endif // MESSAGEMODEL_H
