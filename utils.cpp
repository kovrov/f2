#include <qmfclient/qmailfolderkey.h> // QMailFolderKey
#include <qmfclient/qmailstore.h> // QMailStore

#include "utils.h"



QMailFolderId find::standard_folder(const QMailAccountId &account_id, QMailFolder::StandardFolder folder)
{
    switch (folder) {

    case QMailFolder::InboxFolder: {

        auto account_key = QMailFolderKey::parentAccountId(account_id);
        auto folder_key = QMailFolderKey::path("inbox", QMailDataComparator::Equal);
        return QMailStore::instance()->queryFolders(account_key & folder_key).first();
    }

    default:
        Q_ASSERT_X (false, Q_FUNC_INFO, "unknown folder");
        return QMailFolderId();
    }
}


const QMailMessagePartContainer * find::messageBody(const QMailMessage &message)
{
    if (const auto container = message.findHtmlContainer())
        return container;
    if (const auto container = message.findPlainTextContainer())
        return container;
    return NULL;
}


bool check::isDownloaded(const QMailMessagePartContainer *container)
{
    if (container->multipartType() == QMailMessagePart::MultipartNone) {

        if (auto part = dynamic_cast<const QMailMessagePart*>(container))
            return part->contentDisposition().size() == 0 || part->contentAvailable();

        return container->contentAvailable();
    }

    for (uint i=0; i < container->partCount(); ++i) {
        if (!isDownloaded(&container->partAt(i)))
            return false;
    }

    return true;
}
