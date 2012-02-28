#include "messagewidget.h"

using namespace widgets;



MessageWidget::MessageWidget(QWidget *parent)
  : QTextEdit (parent)
{
    setReadOnly(true);
}
