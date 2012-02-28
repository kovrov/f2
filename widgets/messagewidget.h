#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H



#include <QTextEdit>


namespace models { class MessageModel; }

namespace widgets {



class MessageWidget : public QTextEdit
{
    Q_OBJECT
public:
    explicit MessageWidget(QWidget *parent = 0);

    models::MessageModel * model() { return mModel; }
    void setModel(models::MessageModel *model) { mModel = model; }

signals:

public slots:

private:
    models::MessageModel *mModel;
};



}  // namespace widgets

#endif // MESSAGEVIEW_H
