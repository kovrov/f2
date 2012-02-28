#ifndef DEBUG_H
#define DEBUG_H

#include <qdebug.h>
#include <qmfclient/qmailmessage.h>
#include <qmfclient/qmailserviceaction.h>

class QTextStream;
class QMailMessageMetaData;


QDebug & operator<<(QDebug &debug, const QMailMessageMetaData &data);
QTextStream & operator<<(QTextStream &s, const QMailMessageMetaData &data);


QDebug & operator<<(QDebug &debug, const QMailServiceAction::Status &s);
QTextStream & operator<<(QTextStream &ts, const QMailServiceAction::Status &s);


class DebugOut
{
    QDebug stream;
    const QString indent;
    int times;
    bool is_space;
public:
    DebugOut() : stream(QtDebugMsg), indent("    "), times(0), is_space(true) {}
    DebugOut &operator<<(const QMailMessagePartContainer &partContainer);
    DebugOut &operator<<(const QMailMessagePart &part);
    DebugOut & operator<<(const QMailMessageContentType &contentType);
    DebugOut & operator<<(const QMailAddress &address);
    DebugOut & operator<<(const QMailMessage &message);
    DebugOut & operator<<(const QMailMessageMetaData &metaData);

    template <class T>
    DebugOut & operator<<(const QList<T> &list)
    {
        *this << "(";
        for (Q_TYPENAME QList<T>::size_type i = 0; i < list.count(); ++i) {
            if (i)
                *this << ", ";
            *this << list.at(i);
        }
        *this << ")";
        return *this;
    }

    template <typename T>
    DebugOut &operator<<(T t) { stream << t << " "; return *this; }
};

//inline DebugOut debug() { return DebugOut(); }

//{ DebugOut out; out << "### message" << endl << mMessage << endl; }



#endif // DEBUG_H
