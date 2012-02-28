#ifndef FAPPLICATION_H
#define FAPPLICATION_H

#include <QtGui/QApplication>


class QUrl;
class QMailAccountId;
class QMailFolderId;
class QMailMessageId;
class QTextStream;



class Application : public QApplication
{
    Q_OBJECT
    typedef QApplication super;
public:
     Application(int& argc, char** argv);
     virtual ~Application();
     int exec();

signals:
     void displayAppRequest();
//    void mailtoRequest(const QUrl&);
//    void displayAccountRequest(const QMailAccountId&);
//    void displayFolderRequest(const QMailFolderId&);
//    void displayMessageRequest(const QMailMessageId&);
//    void displayMessageRequest(const QUrl&);
//    void displayAttachmentRequest(const QMailMessagePart::Location&);

private:
    bool launchRequest(const QStringList& parameters);
};



#endif // FAPPLICATION_H
