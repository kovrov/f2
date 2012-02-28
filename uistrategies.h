#ifndef UISTRATEGIES_H
#define UISTRATEGIES_H


// Qt
#include <QWidget>
#include <QAbstractItemView>
#include <QModelIndex>
#include <QTreeView>
#include <QSettings>
#include <qdebug.h>

// QMF
#include <qmfclient/qmailid.h> // QMailMessageId
#include <qmfclient/qmailmessagemodelbase.h> // QMailMessageModelBase
#include <qmfclient/qmailmessage.h> // QMailMessage
#include <qmfclient/qmailaccount.h> // QMailAccount
#include <qmfclient/qmailstore.h> // QMailStore
#include <qmfclient/qmailfolder.h> // QMailFolder

// project
#include "debug.h"
#include "view.h"
#include "uimanager.h"
#include "context.h"
#include "backendstrategies.h"
#include "models/folderstreemodel.h"
#include "models/messagemodel.h"
#include "models/folderlistmodel.h"
#include "models/attachmentlistmodel.h"
#include "widgets/combobox.h"
#include "widgets/inputdialog.h"
#include "widgets/progressindicator.h"
#include "widgets/messagewidget.h"


#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }


namespace strategy {

/**
 *
 * All of functor classes in this namespace could be combined with a context
 * holding all the necessary arguments. This will form a closure, which could
 * be connected to arbitrary signals.
 *
 *  Naming patterns:
 *  - ShowXxx
 *    To 'show' something means setting existing widget or it's model to a new
 *    state. It does not meant to create any new UI elements or views.
 *  - DisplayXXX
 *  - SelectXXX
 */



/**
 * The 'Show Message' routine suppose to reset message viwers's model to a new
 * state. Additionally, it might (depending on preferences) ensure availability
 * of the message.
 *
 * Consider to change arguments for something more generic (a View*).
 */
class ShowMessage
{
public:
    void operator()(const QMailMessageId &id, models::MessageModel *model)
    {
        Q_ASSERT (model);
        model->setMessageId(id);

        static const QSettings settings;
        if (settings.value("download_message_body_ondemand", true).toBool()) {
            backend_strategy::DownloadMessageBody download;
            download(model->message());
        }
    }
};



/**
 * To 'Update Message View' means to set message widget's contents (and
 * possibly other UI elements) from a qmailmessage. This includes updating
 * window title, showing a download/cancel button if needed, and a download
 * progress if there is any.
 *
 * This could be a method of some specialized widget.. TBD.
 */
class UpdateMessageView
{
public:
    void operator()(desktopUI::View *view, const models::MessageModel *model)
    {
        Q_ASSERT (model);
        auto message_viewer = qobject_cast<widgets::MessageWidget *>(view->queryQWidget("message_viewer"));
        Q_ASSERT (message_viewer);
        auto download_prompt = view->queryQWidget("download_prompt");
        Q_ASSERT (download_prompt);

        message_viewer->window()->setWindowTitle(model->message().subject());

        if (const auto body_container = find::messageBody(model->message())) {

            message_viewer->setEnabled(true);
            if (body_container->contentType().subType().toLower() == "plain")
                message_viewer->setPlainText(body_container->body().data());
            else
                message_viewer->setHtml(body_container->body().data());

            if (check::isDownloaded(body_container))
                download_prompt->hide();
            else {
                download_prompt->show();

                const auto part = dynamic_cast<const QMailMessagePart*>(body_container);
                const auto &operations = part
                        ? ServiceActionManager::instance()->operations(part->location())
                        : ServiceActionManager::instance()->operations(model->message().id());

                auto start_download_button = view->queryQWidget("start_download_button");
                Q_ASSERT (start_download_button);
                start_download_button->setEnabled(operations.empty());

                auto stop_download_button = view->queryQWidget("stop_download_button");
                Q_ASSERT (stop_download_button);
                stop_download_button->setEnabled(!operations.empty());
            }
        }
        else {
            qWarning() << "@strategy::UpdateMessageView:"
                       << "Message body part not found.";
            message_viewer->setEnabled(false);
            message_viewer->clear();
            download_prompt->hide();
        }

//        DebugOut() << "### message" << endl << model->message() << endl;
    }
};



class ShowFolder
{
public:
    void operator()(const QMailFolderId &id, desktopUI::View *view)
    {
        auto messages_list = qobject_cast<QAbstractItemView*>(view->queryQWidget("messages_list"));
        Q_ASSERT (messages_list);

        QMailMessageModelBase *model = qobject_cast<QMailMessageModelBase*>(messages_list->model());
        Q_ASSERT (model);

        if (!id.isValid()) {
            model->setKey(QMailMessageKey::nonMatchingKey());
            return;
        }

        Q_ASSERT (QMailFolder(id).id().isValid());
        model->setKey(QMailMessageKey::parentFolderId(id));

        if (model->isEmpty()) {
             backend_strategy::InitFolder init_folder;
             init_folder(id);
        }
    }
};



class SelectFolder
{
public:
    void operator()(const QMailFolderId &id, desktopUI::View *view)
    {
        widgets::ComboBox *folders_list = qobject_cast<widgets::ComboBox*>(view->queryQWidget("folders_list"));
        Q_ASSERT (folders_list);
        models::FoldersTree *folders_model = qobject_cast<models::FoldersTree*>(folders_list->model());
        Q_ASSERT (folders_model);

        QMailFolder folder(id);
        if (!folder.id().isValid()) {
            qWarning() << "@strategy::SelectFolder:"
                       << "Selecting an invalid folder..";
        }

        /// folders_model->setAccountId(folder.parentAccountId());
        folders_list->setCurrentIndex(folders_model->indexFromId(folder.id()));
    }
};



/**
 * Functor implementing 'ShowAccount' strategy
 *
 * Meant to be called with a 'view' to show account in, and Id of the account
 * to show.
 *
 * The view should contain a 'messages_list' widget. The widget have to derive
 * from QAbstractItemView (implementing `QAbstractItemModel *model()` method).
 */
class ShowAccount
{
public:
    void operator()(const QMailAccountId &account_id, desktopUI::View *view)
    {
        QMailAccount account(account_id);
        if (!account.id().isValid()) {
            return;
        }

        auto inbox_id = account.standardFolder(QMailFolder::InboxFolder);
        if (inbox_id.isValid()) {

            SelectFolder strategy;
            strategy(inbox_id, view);
            return;
        }

        qDebug() << "@strategy::ShowAccount:"
                 << "Standard folder 'inbox' not found, asking user...";

        /// FIXME: move creation of a dialog to UI manager!!!
        {
            backend_strategy::InitAccount init_account;
            init_account(account_id);

            auto dialog = new widgets::InputDialog(view->queryQWidget()->window());
            dialog->setModal(true);

            dialog->setWindowTitle(account.name());
            dialog->setLabelText("choose inbox folder");

            widgets::ComboBox *folders_combo = new widgets::ComboBox(dialog);

            QTreeView *tree_view = new QTreeView(folders_combo);
            tree_view->setItemsExpandable(false);
            tree_view->setHeaderHidden(true);
            folders_combo->setView(tree_view);

            models::FolderListModel *folders_model = new models::FolderListModel(folders_combo);
            folders_combo->setModel(folders_model);
            folders_model->setAccountId(account.id());

            CONNECT (folders_model, SIGNAL(layoutChanged()), tree_view, SLOT(expandAll()));

            dialog->setInputWidget(folders_combo);

            typedef ctx::Dialog2QMailFolderId<strategy::SelectFolder, desktopUI::View> SelectFolderStrategy;
            QObject::connect(dialog, SIGNAL(accepted()),
                     new SelectFolderStrategy(dialog, view, dialog), SLOT(exec()));

            dialog->open();
        }
    }
};



class DisplayAttachments
{
public:
    void operator()(models::MessageModel *message_model)
    {
        desktopUI::Manager *ui_mgr = desktopUI::Manager::instance();

        desktopUI::View *attachment_view = ui_mgr->queryView("attachment_view");
        if (NULL == attachment_view)
            attachment_view = ui_mgr->createView("attachment_view");
        Q_ASSERT (attachment_view);

        QAbstractItemView *attachment_list = qobject_cast<QAbstractItemView *>(attachment_view->queryQWidget("attachment_list"));
        Q_ASSERT (attachment_list);
        models::AttachmentList *attachment_list_model = qobject_cast<models::AttachmentList *>(attachment_list->model());
        Q_ASSERT (attachment_list_model);
        attachment_list_model->setSourceModel(message_model);

        QWidget *attachment_view_widget = attachment_view->queryQWidget();
        Q_ASSERT (attachment_view_widget);

        attachment_view_widget->show();
        attachment_view_widget->raise();
    }
};



class DisplayApp
{
public:
    void operator()()
    {
        desktopUI::Manager *ui_mgr = desktopUI::Manager::instance();

        desktopUI::View *main_view = ui_mgr->queryView("main_view");
        if (NULL == main_view)
            main_view = ui_mgr->createView("main_view");
        Q_ASSERT (main_view);

        QWidget *main_view_widget = main_view->queryQWidget();
        Q_ASSERT (main_view_widget);

        main_view_widget->show();
        main_view_widget->raise();

        // show an account/folder
        /// TODO: exclude disabled accounts
        const QMailAccountIdList &account_ids = QMailStore::instance()->queryAccounts();
        if (account_ids.isEmpty())
            return;

        // as there's no concept of default account, selecting first one
        ShowAccount strategy;
        strategy(account_ids.first(), main_view);


//        //-tmp------------------------------------------------------------------
//        auto message_viewer = qobject_cast<MessageWidget *>(main_view->queryQWidget("message_viewer"));
//        Q_ASSERT (message_viewer);
//        auto message_model = message_viewer->model();
//        Q_ASSERT (message_model);
//        class DisplayAttachments display_attachments;
//        display_attachments(message_model);
    }
};



}  // namespace strategy



#endif  // UISTRATEGIES_H
