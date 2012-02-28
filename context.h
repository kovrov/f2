#ifndef CONTEXT_H
#define CONTEXT_H

#include <QObject>
#include <QPointer>
#include <QModelIndex>

// QMF
#include <qmfclient/qmailid.h>  // QMailMessageId
#include <qmfclient/qmailmessagemodelbase.h>  // QMailMessageModelBase
#include <qmfclient/qmailfolder.h>  // QMailFolder
#include <qmfclient/qmailserviceaction.h>  // QMailServiceAction

// project
#include "widgets/inputdialog.h"
#include "widgets/combobox.h"
#include "models/folderlistmodel.h"
#include "models/attachmentlistmodel.h"


namespace ctx {



class Base : public QObject
{
    Q_OBJECT
public:
    explicit Base(QObject *parent=0) : QObject (parent) {}
    virtual ~Base() {} // = default;
public slots:
    virtual void exec() = 0;
};


class ModelIndex : public QObject
{
    Q_OBJECT
public:
    explicit ModelIndex(QObject *parent=0) : QObject (parent) {}
    virtual ~ModelIndex() {} // = default;
public slots:
    virtual void exec(const QModelIndex &index) = 0;
};


class IntegralIndex : public QObject
{
    Q_OBJECT
public:
    explicit IntegralIndex(QObject *parent=0) : QObject (parent) {}
    virtual ~IntegralIndex() {} // = default;
public slots:
    virtual void exec(int index) = 0;
};


/** A special case... */
class ActionState : public QObject
{
    Q_OBJECT
protected:
    explicit ActionState(QObject *parent=0) : QObject (parent) {}
    virtual ~ActionState() {} // = default;
public slots:
    virtual void activityChanged(quint64,QMailServiceAction::Activity) {}
    virtual void connectivityChanged(quint64,QMailServiceAction::Connectivity) {}
    virtual void progressChanged(quint64,uint,uint) {}
    virtual void statusChanged(quint64,const QMailServiceAction::Status &) {}
};



/**
 *
 * TODO: rename
 */
template <typename StrategyType>
class Bind0 : public Base
{
public:
    Bind0(QObject *parent=0)
      : Base (parent)
    {}
    virtual ~Bind0() {} // = default;

    virtual void exec()
    {
        StrategyType strategy;
        strategy();
    }
};



template <typename StrategyType, typename T>
class Bind : public Base
{
public:
    Bind(T *arg, QObject *parent=0)
      : Base (parent),
        mArgument (arg)
    {}
    virtual ~Bind() {} // = default;

    virtual void exec()
    {
        Q_ASSERT (!mArgument.isNull());
        StrategyType strategy;
        strategy(mArgument.data());
    }

private:
    QPointer<T> mArgument;
};



/**
 *
 * TODO: rename
 */
template <typename StrategyType, typename T1, typename T2>
class Bind2 : public Base
{
public:
    Bind2(T1 *arg, T2 *arg2, QObject *parent=0)
      : Base (parent),
        mArgument1 (arg),
        mArgument2 (arg2)
    {}
    virtual ~Bind2() {} // = default;

    virtual void exec()
    {
        Q_ASSERT (!mArgument1.isNull());
        Q_ASSERT (!mArgument2.isNull());
        StrategyType strategy;
        strategy(mArgument1, mArgument2);
    }

private:
    QPointer<T1> mArgument1;
    QPointer<T2> mArgument2;
};



/**
 *
 * TODO: rename
 */
template <typename StrategyType, typename T>
class ModelIndex2MessageId : public ModelIndex
{
public:
    ModelIndex2MessageId(T *t, QObject *parent=0)
      : ModelIndex (parent),
        mArgument1 (t)
    {}
    virtual ~ModelIndex2MessageId() {} // = default;

    virtual void exec(const QModelIndex &index)
    {
        Q_ASSERT (!mArgument1.isNull());
        Q_ASSERT (index.isValid());
        const QVariant &data = index.data(QMailMessageModelBase::MessageIdRole);
        Q_ASSERT (data.canConvert<QMailMessageId>());
        StrategyType strategy;
        strategy(data.value<QMailMessageId>(), mArgument1);
    }

private:
    QPointer<T> mArgument1;
};



template <typename StrategyType, typename T, typename ModelT>
class Int2Id : public IntegralIndex
{
public:
    Int2Id(T *t, ModelT *model, QObject *parent=0)
      : IntegralIndex (parent),
        mArgument2 (t),
        mModel (model)
    {}
    virtual ~Int2Id() {} // = default;

    virtual void exec(int index)
    {
        Q_ASSERT (!mModel.isNull());
        const auto &model_index = mModel->index(index, 0);
        const auto &id = mModel->idFromIndex(model_index);
        Q_ASSERT (!mArgument2.isNull());
        StrategyType strategy;
        strategy(id, mArgument2.data());
    }

private:
    QPointer<T> mArgument2;
    QPointer<ModelT> mModel;
};



template <typename StrategyType, typename T, typename ModelT>
class Index2Id : public ModelIndex
{
public:
    Index2Id(T *t, ModelT *model, QObject *parent=0)
      : ModelIndex (parent),
        mArgument2 (t),
        mModel (model)
    {}
    virtual ~Index2Id() {} // = default;

    virtual void exec(const QModelIndex &model_index)
    {
        Q_ASSERT (!mModel.isNull());
        const auto &id = mModel->idFromIndex(model_index);
        Q_ASSERT (!mArgument2.isNull());
        StrategyType strategy;
        strategy(id, mArgument2.data());
    }

private:
    QPointer<T> mArgument2;
    QPointer<ModelT> mModel;
};



template <typename StrategyType>
class Index2Location : public ModelIndex
{
public:
    Index2Location(QObject *parent=0)
      : ModelIndex (parent)
    {}
    virtual ~Index2Location() {} // = default;

    virtual void exec(const QModelIndex &model_index)
    {
        QVariant data = model_index.data(models::AttachmentList::LocationRole);
        data.canConvert<QMailMessagePart::Location>();
        QMailMessagePart::Location location = data.value<QMailMessagePart::Location>();
        StrategyType strategy;
        strategy(location);
    }
};



/**
 * TEMP: UI manager should return view wich emits adequate value
 *
 * TODO: rename
 */
template <typename StrategyType, typename T>
class Dialog2QMailFolderId : public Base
{
public:
    Dialog2QMailFolderId(widgets::InputDialog *dialog, T *t, QObject *parent=0)
      : Base (parent),
        mArgument2 (t),
        mDialog (dialog)
    {}
    virtual ~Dialog2QMailFolderId() {} // = default;

    virtual void exec()
    {
        Q_ASSERT (!mArgument2.isNull());
        Q_ASSERT (!mDialog.isNull());
        widgets::ComboBox *folders_combo = qobject_cast<widgets::ComboBox *>(mDialog->inputWidget());
        Q_ASSERT (folders_combo);

        const QVariant &data = folders_combo->currentIndex().data(models::FolderListModel::FolderIdRole);
        if (!data.canConvert<QMailFolderId>()) {

            qWarning() << "@ctx::Dialog2QMailFolderId:"
                       << "cannot onvert to QMailFolderId -" << data;
            return;
        }

        StrategyType strategy;
        strategy(data.value<QMailFolderId>(), mArgument2.data());
    }

private:
    QPointer<T> mArgument2;
    QPointer<widgets::InputDialog> mDialog;
};



template <typename StrategyType, typename T>
class ExtractFolderId : public Base
{
public:
    ExtractFolderId(T *list, QObject *parent=0)
      : Base (parent),
        mList (list)
    {}
    virtual ~ExtractFolderId() {} // = default;

    virtual void exec()
    {
        Q_ASSERT (!mList.isNull());
        const auto &model_index = mList->currentIndex();
        const QVariant &data = model_index.data(models::FolderListModel::FolderIdRole);
        if (!data.canConvert<QMailFolderId>()) {

            qWarning() << "@ctx::ExtractFolderId"
                       << "cannot onvert to QMailFolderId:" << data;
            return;
        }

        StrategyType strategy;
        strategy(data.value<QMailFolderId>());
    }

private:
    QPointer<T> mList;
};



/** A special case... Consider to break into separate normal contexts/strategies */
template <typename T1, typename T2>
class ActionTracker : public ActionState
{
public:
    ActionTracker(quint64 filter, T1 *progress_widget, T2 *message_widget, QObject *parent=0)
      : ActionState (parent),
        mFilter (filter),
        mProgressWidget (progress_widget),
        mMessageWidget (message_widget)
    {}
    virtual ~ActionTracker() {} // = default;

    void activityChanged(quint64 serial, QMailServiceAction::Activity activity)
    {
        if (0 != mFilter && serial != mFilter) return;

        switch (activity) {

        case QMailServiceAction::Successful:
            if (!mMessageWidget.isNull()) {
                mMessageWidget->clearMessage();
            }
        // NOTE: fall-through
        case QMailServiceAction::Failed:
            if (!mProgressWidget.isNull()) {
                mProgressWidget->hide();
                mProgressWidget->reset();
            }
            break;

        case QMailServiceAction::InProgress:
            if (!mProgressWidget.isNull()) {
                mProgressWidget->show();
            }
            break;

        default: ;
        }
    }

    void progressChanged(quint64 serial, uint value, uint total)
    {
        if (mProgressWidget.isNull() || (0 != mFilter && serial != mFilter)) return;
        mProgressWidget->setValue(value);
        mProgressWidget->setMaximum(total);
    }

    void statusChanged(quint64 serial, const QMailServiceAction::Status &status)
    {
        if (mMessageWidget.isNull() || (0 != mFilter && serial != mFilter)) return;
        mMessageWidget->showMessage(status.text);
    }

private:
    quint64 mFilter;
    QPointer<T1> mProgressWidget;
    QPointer<T2> mMessageWidget;
};



template <typename StrategyType, typename T>
class ExtractMessage : public Base
{
public:
    ExtractMessage(T *list, QObject *parent=0)
      : Base (parent),
        mModel (list)
    {}
    virtual ~ExtractMessage() {} // = default;

    virtual void exec()
    {
        Q_ASSERT (!mModel.isNull());

        StrategyType strategy;
        strategy(mModel->message());
    }

private:
    QPointer<T> mModel;
};



}  // namespace



#endif // CONTEXT_H
