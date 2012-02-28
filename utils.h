#ifndef UTILLS_H
#define UTILLS_H


#include <qmfclient/qmailfolder.h>



namespace find
{
    QMailFolderId standard_folder(const QMailAccountId &account_id, QMailFolder::StandardFolder folder);
    const QMailMessagePartContainer * messageBody(const QMailMessage &message);
}



namespace check
{
    bool isDownloaded(const QMailMessagePartContainer *container);
}



#endif // UTILLS_H
