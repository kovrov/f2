#include "view.h"

#include <QWidget>


using namespace desktopUI;


View::View(QWidget *root)
  : QObject (root),
    mRoot (root)
{
}


QWidget* View::queryQWidget(const QString &name)
{
    if (name.isNull() || name == mRoot->objectName())
        return mRoot;

    return mRoot->findChild<QWidget*>(name);
}
