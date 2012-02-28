#include <QPainter>
#include <QCommonStyle>
#include <QPaintEvent>

#include "progressindicator.h"

using namespace widgets;



ProgressIndicator::ProgressIndicator(QWidget *parent)
  : QWidget (parent),
    mMaximum (100),
    mMinimum (0),
    mValue (0),
    mBusyValue (-1),
    mTimer (0)
{
//    QPainterPath path;
//    path.moveTo(center);
//    path.arcTo(boundingRect, startAngle, sweepLength);
}


void ProgressIndicator::setMaximum(int maximum)
{
    mMaximum = maximum;
    update();
}


void ProgressIndicator::setMinimum(int minimum)
{
    mMinimum = minimum;
    update();
}


void ProgressIndicator::setValue(int value)
{
    mValue = value;
    update();
}


void ProgressIndicator::reset()
{
    mValue = 0;
    update();
}


void ProgressIndicator::start()
{
    mTimer = startTimer(1000/15);
    mBusyValue = 0;
}


void ProgressIndicator::stop()
{
    if (mTimer)
        killTimer(mTimer);
    mTimer = 0;
    mBusyValue = -1;
}


void ProgressIndicator::timerEvent(QTimerEvent *event)
{
    if (mTimer == event->timerId()) {
        mBusyValue = (mBusyValue + 16/2) % 5760;
        update();
    }
}


void ProgressIndicator::paintEvent(QPaintEvent *)
{
    #define FULL_CIRCLE 5760.0
    #define PEN_WIDTH 16
    #define PROGRESS_PEN 24
    #define PROGRESS_OFFSET 8

    QPainter painter(this);
    painter.translate(width() / 2.0, height() / 2.0);
    int side = qMin(width(), height());
    painter.scale(side / 100.0, side / 100.0);
    painter.setRenderHint(QPainter::Antialiasing);

    static const QColor color = QCommonStyle().standardPalette().color(QPalette::Dark);
    static const QPen pen = QPen(QBrush(color), PEN_WIDTH, Qt::SolidLine, Qt::RoundCap);
    if (mBusyValue != -1) {
        painter.setPen(pen);
        painter.drawArc(-50.0+PEN_WIDTH/2.0,-50.0+PEN_WIDTH/2.0,100.0-PEN_WIDTH,100.0-PEN_WIDTH,
                        mBusyValue*16, 120*16);
    }

    static const QColor progress_color = QColor(color.red(),color.green(),color.blue(), color.alpha()/2);
    static const QPen progress_pen = QPen(QBrush(progress_color), PROGRESS_PEN, Qt::SolidLine, Qt::FlatCap);
    if (mMinimum < mValue && mValue <= mMaximum) {
        int progress = qreal(mValue - mMinimum) * FULL_CIRCLE / qreal(mMaximum - mMinimum);
        painter.setPen(progress_pen);
        painter.drawArc(-50.0 + PROGRESS_OFFSET + PROGRESS_PEN/2.0, -50.0 + PROGRESS_OFFSET + PROGRESS_PEN/2.0,
                        100.0 - PROGRESS_OFFSET*2.0 - PROGRESS_PEN, 100.0 - PROGRESS_OFFSET*2.0 - PROGRESS_PEN,
                        90*16, -progress);
    }
}
