#ifndef UIMANAGER_H
#define UIMANAGER_H


#include <QObject>


namespace desktopUI {

class View;


class Manager : public QObject
{
    Q_OBJECT

    explicit Manager(QObject *parent = 0) : QObject (parent) {}
public:
    static Manager *instance();
    View* queryView(const QString &name) { Q_UNUSED (name); return NULL; }
    View* createView(const QString &name);

signals:

public slots:

};



} // namespace desktopUI


#endif // UIMANAGER_H
