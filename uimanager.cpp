// #t
#include <QTreeView>
#include <QProgressBar>
#include <QShortcut>

// QMF
#include <qmfclient/qmailaccountlistmodel.h>  // QMailAccountListModel
#include <qmfclient/qmailmessagelistmodel.h>  // QMailMessageListModel

// project
#include "context.h"
#include "uistrategies.h"
#include "view.h"
#include "uimanager.h"
#include "models/folderlistmodel.h"
#include "models/messagelistmodel.h"
#include "models/messagemodel.h"
#include "widgets/combobox.h"
#include "widgets/progressindicator.h"
#include "widgets/messagelistdelegate.h"
#include "widgets/attachmentlistdelegate.h"
#include "ui_mainview.h"
#include "ui_attachmentview.h"



/**
 * Desktop UI factory routines.
 */

using namespace desktopUI;

#define CONNECT(a,b,c,d) if (!QObject::connect(a,b,c,d)) { Q_ASSERT (false); }
#define CONNECT_Q(a,b,c,d) if (!QObject::connect(a,b,c,d,Qt::QueuedConnection)) { Q_ASSERT (false); }


namespace {

View *construct_main_view(QMainWindow *window)
{
    Ui::main_view ui_builder;
    ui_builder.setupUi(window);
    View *view = new View(window);

    widgets::ComboBox *folders_list = ui_builder.folders_list;
    QListView *messages_list = ui_builder.messages_list;
    auto message_viewer = ui_builder.message_viewer;
    QStatusBar *status_bar = ui_builder.status_bar;
    QPushButton *start_download_button = ui_builder.start_download_button;
    QPushButton *stop_download_button = ui_builder.stop_download_button;
    QAction *action_attachments_window = ui_builder.action_attachments_window;

    /// Account/Folder interaction
    {
        models::FoldersTree *folders_model = new models::FoldersTree(view);
        folders_list->setModel(folders_model);

        QTreeView *tree_view = new QTreeView(folders_list);
        tree_view->setItemsExpandable(false);
        tree_view->setHeaderHidden(true);
        folders_list->setView(tree_view);
        tree_view->expandAll();
        tree_view->setRootIsDecorated(false);
        CONNECT (folders_model, SIGNAL(rowsInserted(QModelIndex,int,int)), tree_view, SLOT(expandAll()));

        typedef ctx::Index2Id<strategy::ShowFolder, View, models::FoldersTree> ShowFolderStrategy;
        CONNECT_Q (folders_list, SIGNAL(currentIndexChanged(QModelIndex)),
                 new ShowFolderStrategy(view, folders_model), SLOT(exec(QModelIndex)));

        // refresh action
        {
            QShortcut *sync_action = new QShortcut(QKeySequence(QKeySequence::Refresh), window);
            typedef ctx::ExtractFolderId<backend_strategy::SyncFolder, widgets::ComboBox> SyncFolderStrategy;
            CONNECT (sync_action, SIGNAL(activated()),
                     new SyncFolderStrategy(folders_list), SLOT(exec()));
        }
    }

    auto message_model = new models::MessageModel(message_viewer);
    /// Message viewer interaction
    {
        message_viewer->setModel(message_model);

        typedef ctx::Bind2<strategy::UpdateMessageView, View, models::MessageModel> UpdateMessageViewStrategy;
        CONNECT (message_model, SIGNAL(updated()),
                 new UpdateMessageViewStrategy(view, message_model), SLOT(exec()));

        typedef ctx::ExtractMessage<backend_strategy::DownloadMessageBody, models::MessageModel> DownloadMessageBodyStrategy;
        CONNECT (start_download_button, SIGNAL(clicked()),
                 new DownloadMessageBodyStrategy(message_model), SLOT(exec()));

        typedef ctx::ExtractMessage<backend_strategy::StopDownloadMessageBody, models::MessageModel> StopDownloadMessageBodyStrategy;
        CONNECT (stop_download_button, SIGNAL(clicked()),
                 new StopDownloadMessageBodyStrategy(message_model), SLOT(exec()));
    }

    /// Messages list interaction
    {
        messages_list->setItemDelegate(new widgets::MessageListDelegate(messages_list));

        auto *messagelist_model = new models::MessageListModel(view);
        messages_list->setModel(messagelist_model);

        typedef ctx::ModelIndex2MessageId<strategy::ShowMessage, models::MessageModel> ShowMessageStrategy;
        CONNECT_Q (messages_list->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                 new ShowMessageStrategy(message_model), SLOT(exec(QModelIndex)));

    }

    /// Progress indicator
    {
        QProgressBar *progress_indicator = new QProgressBar(status_bar);
        progress_indicator->setObjectName("progress_indicator");
//        progress_indicator->setMinimumWidth(16);
        status_bar->addPermanentWidget(progress_indicator);
        //ProgressIndicator *progress_indicator = qobject_cast<ProgressIndicator *>(main_view->queryQWidget("progress_indicator"));

        auto action_tracker_strategy = new ctx::ActionTracker<QProgressBar,QStatusBar>(0, progress_indicator, status_bar, window);
        CONNECT (ServiceActionManager::instance(), SIGNAL(progressChanged(quint64,uint,uint)),
                            action_tracker_strategy, SLOT(progressChanged(quint64,uint,uint)));
        CONNECT (ServiceActionManager::instance(), SIGNAL(activityChanged(quint64,QMailServiceAction::Activity)),
                            action_tracker_strategy, SLOT(activityChanged(quint64,QMailServiceAction::Activity)));
        CONNECT (ServiceActionManager::instance(), SIGNAL(statusChanged(quint64,QMailServiceAction::Status)),
                            action_tracker_strategy, SLOT(statusChanged(quint64,QMailServiceAction::Status)));
        CONNECT (ServiceActionManager::instance(), SIGNAL(connectivityChanged(quint64,QMailServiceAction::Connectivity)),
                            action_tracker_strategy, SLOT(connectivityChanged(quint64,QMailServiceAction::Connectivity)));
    }

    /// misc. actions
    {
        typedef ctx::Bind<strategy::DisplayAttachments, models::MessageModel> DisplayAttachmentsStrategy;
        CONNECT (action_attachments_window, SIGNAL(triggered()),
                 new DisplayAttachmentsStrategy(message_model), SLOT(exec()));
    }

    return view;
}


View *construct_attachment_view(QWidget *parent)
{
    Ui::attachment_view ui_builder;
    ui_builder.setupUi(parent);
    View *view = new View(parent);

    QAbstractItemView *attachment_list = ui_builder.attachment_list;

    models::AttachmentList *attachments_model = new models::AttachmentList(attachment_list);
    attachment_list->setModel(attachments_model);
    attachment_list->setItemDelegate(new widgets::AttachmentListDelegate(parent));

    typedef ctx::Index2Location<backend_strategy::DownloadMessagePart> DownloadPartStrategy;
    CONNECT (attachment_list, SIGNAL(doubleClicked(QModelIndex)),
             new DownloadPartStrategy(attachment_list), SLOT(exec(QModelIndex)));

    return view;
}


} // namespace




/**
 * desktopUI::Manager class
 */

Manager * Manager::instance()
{
    static Manager *self = NULL;
    if (NULL == self)
        self = new Manager();
    return self;
}


View* Manager::createView(const QString &name)
{
    if (name == "main_view") {
        QMainWindow *window = new QMainWindow();
        return construct_main_view(window);
    }
    else if (name == "attachment_view") {
        QWidget *window = new QWidget();
        return construct_attachment_view(window);
    }

    return NULL;
}
