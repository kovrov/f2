#include "debug.h"


#include <qdebug.h>
#include <qmfclient/qmailmessage.h> // QMailMessageMetaData
#include <qmfclient/qmailid.h>



QDebug & operator<<(QDebug &debug, const QMailMessageMetaData &data)
{
    debug << data.id();
    return debug;
}



QTextStream & operator<<(QTextStream &s, const QMailMessageMetaData &data)
{
    s << data.id();
    return s;
}



static const char *code_str[] = { "NoError", "NotImplemented", "FrameworkFault",
                                  "SystemError", "UnknownResponse", "LoginFailed",
                                  "Cancel", "FileSystemFull", "NonexistentMessage",
                                  "EnqueueFailed", "NoConnection", "ConnectionInUse",
                                  "ConnectionNotReady", "Configuration", "InvalidAddress",
                                  "InvalidData", "Timeout", "InternalStateReset" };

QDebug & operator<<(QDebug &debug, const QMailServiceAction::Status &s)
{
    debug << "QMailServiceAction::Status {"
          << (code_str[s.errorCode > 0 ? s.errorCode - QMailServiceAction::Status::ErrorCodeMinimum + 1 : 0])
          << s.text
          << "}";
    return debug;
}



QTextStream & operator<<(QTextStream &ts, const QMailServiceAction::Status &s)
{
    ts << "QMailServiceAction::Status {"
          << (code_str[s.errorCode > 0 ? s.errorCode - QMailServiceAction::Status::ErrorCodeMinimum + 1 : 0])
          << s.text
          << "}";
    return ts;
}




DebugOut & DebugOut::operator<<(const QMailMessagePartContainer &partContainer)
{
    const QString &ind = indent.repeated(times);
    *this << ind << "[QMailMessagePartContainer]" << endl;
    *this << ind << "body: {" // QMailMessageBody
            //<< "data:" << partContainer.body().data() // FIXME: binary data!
            << "isEmpty:" << (partContainer.body().isEmpty()?"true":"false")
            << "length:" << partContainer.body().length()
            << "contentType: {" << partContainer.body().contentType() << "}" << endl;
    *this << ind << "boundary:" << partContainer.boundary() << endl; // QByteArray
    *this << ind << "contentAvailable:" << (partContainer.contentAvailable()?"true":"false") << endl; // bool
    const QMailMessageContentType &contentType = partContainer.contentType();
    *this << ind << "contentType: {"
             << "boundary:" << QString(contentType.boundary())
             << "charset:" << QString(contentType.charset())
             << "name:" << QString(contentType.name())
             << "subType:" << QString(contentType.subType())
             << "type:" << QString(contentType.type()) << "}" << endl;
    *this << ind << "hasBody:" << (partContainer.hasBody()?"true":"false") << endl; // bool
    //*this << ind << "headerFields:" << partContainer.headerFields() << endl; // QList<QMailMessageHeaderField>
    *this << ind << "partialContentAvailable:" << (partContainer.partialContentAvailable()?"true":"false") << endl; // bool
    *this << ind << "transferEncoding:" << partContainer.transferEncoding() << endl; // QMailMessageBody::TransferEncoding
    static const char* multipart_types[] = { "MultipartNone",
                                            "MultipartSigned",
                                            "MultipartEncrypted",
                                            "MultipartMixed",
                                            "MultipartAlternative",
                                            "MultipartDigest",
                                            "MultipartParallel",
                                            "MultipartRelated",
                                            "MultipartFormData",
                                            "MultipartReport" };
    *this << ind << "multipartType:" << QString("QMailMessagePart::") + multipart_types[partContainer.multipartType()] << endl;
    *this << ind << "partCount:" << partContainer.partCount() << endl; // uint
    for (uint i=0; i < partContainer.partCount(); i++) {
        times++;
        *this << "### part" << endl;
        *this << partContainer.partAt(i);
        times--;
    }
    return *this;
}

DebugOut &DebugOut::operator<<(const QMailMessagePart &part)
{
    static const char* Disposition_types[] = { "None", "Inline", "Attachment" };

    const QString &ind = indent.repeated(times);
    *this << ind << "[QMailMessagePart]" << endl;
    *this << ind << "contentDescription:" << part.contentDescription() << endl;
    const QMailMessageContentDisposition &contentDisposition = part.contentDisposition();
    *this << ind << "contentDisposition: {"
             << "creationDate:" << contentDisposition.creationDate().toString()
             << "filename:" << QString(contentDisposition.filename())
             << "modificationDate:" << contentDisposition.modificationDate().toString()
             << "readDate:" << contentDisposition.readDate().toString()
             << "size:" << contentDisposition.size()
             << "type:" << Disposition_types[contentDisposition.type()] << "}" << endl; // DispositionType
    *this << ind << "contentID:" << part.contentID() << endl;
    *this << ind << "contentLanguage:" << part.contentLanguage() << endl;
    *this << ind << "contentLocation:" << part.contentLocation() << endl;
    *this << ind << "displayName:" << part.displayName() << endl;
    *this << ind << "identifier:" << part.identifier() << endl;
    *this << ind << "indicativeSize:" << part.indicativeSize() << endl;
    *this << ind << "location:" << part.location().toString(true) << endl; // QMailMessagePart::Location
    *this << ind << "messageReference:" << part.messageReference() << endl; // QMailMessageId
    *this << ind << "partNumber:" << part.partNumber() << endl;
    *this << ind << "partReference:" << part.partReference().toString(true) << endl; // QMailMessagePart::Location
    *this << ind << "referenceResolution:" << part.referenceResolution() << endl;
    static const char* reference_types[] = { "None", "MessageReference", "PartReference"};
    uint refType = static_cast<uint>(part.referenceType());
    if (refType < sizeof(reference_types) / sizeof(reference_types[0])) {
        *this << ind << "referenceType:" << QString("QMailMessagePart::") + reference_types[refType] << endl;
    } else {
        *this << ind << "Cannot detect referenceType:" << refType << endl;
    }
    *this << *(static_cast<const QMailMessagePartContainer*>(&part));
    return *this;
}

DebugOut & DebugOut::operator<<(const QMailMessageContentType &contentType)
{
    const QString &ind = indent.repeated(times);
    //*this << ind << "[QMailMessageContentType]" << endl;
    *this << ind << "contentType: {"
        << "boundary:" << contentType.boundary() // QByteArray
        << "charset:" << contentType.charset() // QByteArray
        << "content:" << contentType.content() // QByteArray
        << "decodedContent:" << contentType.decodedContent()
        << "id:" << contentType.id() // QByteArray
        << "isNull:" << (contentType.isNull()?"true":"false")
        << "name:" << contentType.name() // QByteArray
        //<< "parameters:" << contentType.parameters() // QList<ParameterType>
        << "subType:" << contentType.subType() // QByteArray
        << "type:" << contentType.type() // QByteArray
        << "}";
    return *this;
}

DebugOut & DebugOut::operator<<(const QMailAddress &address)
{
    *this << address.toString();
    return *this;
}

DebugOut & DebugOut::operator<<(const QMailMessage &message)
{
    const QString &ind = indent.repeated(times);
    *this << ind << "[QMailMessage]" << endl;
    *this << ind << "to:" << message.to() << endl;
    *this << ind << "cc:" << message.cc() << endl;
    *this << ind << "bcc:" << message.bcc() << endl;
    *this << ind << "contentSize:" << message.contentSize() << endl;
    *this << ind << "externalLocationReference:" << message.externalLocationReference() << endl;
    *this << ind << "hasRecipients:" << (message.hasRecipients()?"true":"false") << endl;
    *this << ind << "inReplyTo:" << message.inReplyTo() << endl;
    *this << ind << "recipients:" << message.recipients() << endl;
    *this << ind << "replyTo:" << message.replyTo().toString() << endl;
    *this << *(static_cast<const QMailMessageMetaData*>(&message));
    *this << *(static_cast<const QMailMessagePartContainer*>(&message));
    return *this;
}

DebugOut & DebugOut::operator<<(const QMailMessageMetaData &metaData)
{
    const QString &ind = indent.repeated(times);
    *this << ind << "[QMailMessageMetaData]" << endl;
    *this << ind << "id:" << metaData.id() << endl;
    static const char* content_types[] = { "UnknownContent",
                                           "NoContent",
                                           "PlainTextContent",
                                           "RichTextContent",
                                           "HtmlContent",
                                           "ImageContent",
                                           "AudioContent",
                                           "VideoContent",
                                           "MultipartContent",
                                           "SmilContent",
                                           "VoicemailContent",
                                           "VideomailContent",
                                           "VCardContent",
                                           "VCalendarContent",
                                           "ICalendarContent",
                                           "DeliveryReportContent",
                                           "UserContent" };
    *this << ind << "content:" << QString("QMailMessage::") + content_types[metaData.content()] << endl;
    *this << ind << "contentAvailable:" << (metaData.contentAvailable()?"true":"false") << endl;
    *this << ind << "contentIdentifier:" << metaData.contentIdentifier() << endl;
    *this << ind << "contentScheme:" << metaData.contentScheme() << endl;
    //metaData.customFields()

    *this << ind << "date:" << metaData.date().toString() << endl;
    *this << ind << "from:" << metaData.from().toString() << endl;
    //*this << ind << "to:" << metaData.to() << endl;
    *this << ind << "inResponseTo:" << metaData.inResponseTo() << endl;
    *this << ind << "indicativeSize:" << metaData.indicativeSize() << endl;
    static const char* message_types[] = { "Mms","Sms","Email","Instant","System","None","AnyType"};
    *this << ind << "messageType:" << QString("QMailMessage::") + message_types[metaData.messageType()] << endl;
    *this << ind << "parentAccountId:" << metaData.parentAccountId() << endl;
    *this << ind << "parentFolderId:" << metaData.parentFolderId() << endl;
    *this << ind << "previousParentFolderId:" << metaData.previousParentFolderId() << endl;
    *this << ind << "receivedDate:" << metaData.receivedDate().toString() << endl;
    static const char* response_types[] = { "NoResponse", "Reply", "ReplyToAll", "Forward", "ForwardPart", "Redirect" };
    *this << ind << "responseType:" << QString("QMailMessage::") + response_types[metaData.responseType()] << endl;
    *this << ind << "serverUid:" << metaData.serverUid() << endl;
    *this << ind << "size:" << metaData.size() << endl;
    *this << ind << "status:" << metaData.status(); // FIXME: quint64
    *this << ind << "subject:" << metaData.subject() << endl;
    return *this;
}
