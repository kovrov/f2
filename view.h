#ifndef FVIEW_H
#define FVIEW_H


#include <QObject>


namespace desktopUI {



class View : public QObject
{
    Q_OBJECT

public:
    explicit View(QWidget *root);
    QWidget* queryQWidget(const QString &name=QString());

signals:

public slots:

private:
    QWidget *mRoot;
};



}  // desktopUI


#endif // FVIEW_H
