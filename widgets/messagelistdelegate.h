#ifndef MESSAGELISTDELEGATE_H
#define MESSAGELISTDELEGATE_H



#include <QAbstractItemDelegate>

namespace widgets {



class MessageListDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit MessageListDelegate(QObject *parent = 0);

    // painting
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

//    // editing
//    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

//    void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

//    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

//    // editor factory
//    QItemEditorFactory *itemEditorFactory() const;
//    void setItemEditorFactory(QItemEditorFactory *factory);

//    virtual QString displayText(const QVariant &value, const QLocale &locale) const;

protected:
//    virtual void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const;

//    bool eventFilter(QObject *object, QEvent *event);
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

private:
//    Q_DECLARE_PRIVATE(MessageListDelegate)
//    Q_DISABLE_COPY(MessageListDelegate)

//    Q_PRIVATE_SLOT(d_func(), void _q_commitDataAndCloseEditor(QWidget*))
};



}  // namespace widgets

#endif // MESSAGELISTDELEGATE_H
