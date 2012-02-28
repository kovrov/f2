#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H



#include <QtGui/QWidget>

namespace widgets {



class ProgressIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY (int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY (int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY (int value READ value WRITE setValue)

public:
    ProgressIndicator(QWidget *parent = 0);

    int maximum() const { return mMaximum; }
    int minimum() const { return mMinimum; }
    int value() const { return mValue; }

public slots:
    void setMaximum(int maximum);
    void setMinimum(int minimum);
    void setValue(int value);
    void start();
    void stop();
    void reset();

protected:
    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);

private:
    int mMaximum;
    int mMinimum;
    int mValue;
    int mBusyValue;
    int mTimer;
};



}  // namespace widgets

#endif // PROGRESSINDICATOR_H
