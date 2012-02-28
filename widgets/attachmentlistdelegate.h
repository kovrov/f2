#ifndef ATTACHMENTLISTDELEGATE_H
#define ATTACHMENTLISTDELEGATE_H



#include <QAbstractItemDelegate>

namespace widgets {



class AttachmentListDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit AttachmentListDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
};



}  // widgets

#endif // ATTACHMENTLISTDELEGATE_H
