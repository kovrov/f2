/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


/// QComboBox modified so currentIndex() returns QModelIndex instead of int.



#ifndef COMBOBOX_H
#define COMBOBOX_H



#include <QWidget>
#include <QPersistentModelIndex>
#include <QAbstractSlider>
#include <QBasicTimer>
#include <QFrame>
#include <QTimer>
#include <QListView>


#include <QStyle>



class QAbstractItemView;
class QStyleOptionComboBox;


namespace widgets {

namespace internal { class ComboBoxContainer; }


class ComboBox : public QWidget
{
    Q_OBJECT
public:
    explicit ComboBox(QWidget *parent = 0);
    virtual ~ComboBox();

    enum SizeAdjustPolicy {
        AdjustToContents,
        AdjustToContentsOnFirstShow,
        AdjustToMinimumContentsLength,
        AdjustToMinimumContentsLengthWithIcon
    };

    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);

    int minimumContentsLength() const;
    void setMinimumContentsLength(int characters);

    QSize iconSize() const;
    void setIconSize(const QSize &size);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    QModelIndex currentIndex() const;

    QModelIndex rootModelIndex() const;
    void setRootModelIndex(const QModelIndex &index);

    int modelColumn() const;
    void setModelColumn(int visibleColumn);

    QAbstractItemView *view() const;
    void setView(QAbstractItemView *itemView);

    virtual void showPopup();
    virtual void hidePopup();

    QIcon itemIcon(const QModelIndex &index) const;
    QString itemText(const QModelIndex &index) const;

    /// reimp
    bool event(QEvent *event);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void activated(const QModelIndex &item);
    void highlighted(const QModelIndex &item);
    void currentIndexChanged(const QModelIndex &item);

public slots:
    void setCurrentIndex(const QModelIndex &item);

private slots:
    void _q_emitHighlighted(const QModelIndex &);
    void _q_emitCurrentIndexChanged(const QModelIndex &index);
    void _q_resetButton();
    void _q_itemSelected(const QModelIndex &item);
    void _q_updateIndexBeforeChange();
    void _q_dataChanged(const QModelIndex &, const QModelIndex &);
    void _q_rowsInserted(const QModelIndex & parent, int start, int end);
    void _q_rowsRemoved(const QModelIndex & parent, int start, int end);
    void _q_modelDestroyed();
    void _q_modelReset();

protected:
    void initStyleOption(QStyleOptionComboBox *option) const;

    /// reimp
    void changeEvent(QEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void hideEvent(QHideEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);
    void showEvent(QShowEvent *e);
    void wheelEvent(QWheelEvent *e);

private:  // private
    internal::ComboBoxContainer* viewContainer();
    int computeWidthHint() const;
    void updateLayoutDirection();
    bool updateHoverControl(const QPoint &pos);
    QSize recomputeSizeHint(QSize &sh) const;
    void updateViewContainerPaletteAndOpacity();
    void updateArrow(QStyle::StateFlag state);
    void keyboardSearchString(const QString &text);
    void emitActivated(const QModelIndex &index);
    void adjustComboBoxSize();

    QAbstractItemModel *mModel;
    internal::ComboBoxContainer *mContainer;
    SizeAdjustPolicy mSizeAdjustPolicy;
    int mMinimumContentsLength;
    QSize mIconSize;
    bool mShownOnce;
    int mMaxVisibleItems;
    int mModelColumn;
    mutable QSize mMinimumSizeHint;
    mutable QSize mSizeHint;
    QStyle::StateFlag mArrowState;
    QStyle::SubControl mHoverControl;
    QRect mHoverRect;
    QPersistentModelIndex mCurrentIndex;
    QPersistentModelIndex mRoot;
    QPersistentModelIndex mIndexBeforeChange;
};


namespace internal {

class ComboBoxScroller : public QWidget
{
    Q_OBJECT
public:
    ComboBoxScroller(QAbstractSlider::SliderAction action, QWidget *parent);
    QSize sizeHint() const;

protected:
    inline void stopTimer() { timer.stop(); }
    inline void startTimer() { timer.start(100, this); fast = false; }

    void enterEvent(QEvent *);

    void leaveEvent(QEvent *);
    void timerEvent(QTimerEvent *e);
    void hideEvent(QHideEvent *);

    void mouseMoveEvent(QMouseEvent *e);

    void paintEvent(QPaintEvent *);

signals:
    void doScroll(int action);

private:
    QAbstractSlider::SliderAction sliderAction;
    QBasicTimer timer;
    bool fast;
};



class ComboBoxContainer : public QFrame
{
    Q_OBJECT
public:
    ComboBoxContainer(QAbstractItemView *itemView, ComboBox *parent);
    QAbstractItemView *itemView() const;
    void setItemView(QAbstractItemView *itemView);
    int spacing() const;
    void updateTopBottomMargin();

    QTimer blockMouseReleaseTimer;
    QBasicTimer adjustSizeTimer;
    QPoint initialClickPosition;

public slots:
    void scrollItemView(int action);
    void updateScrollers();
    void viewDestroyed();

protected:
    void changeEvent(QEvent *e);
    bool eventFilter(QObject *o, QEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
    void timerEvent(QTimerEvent *timerEvent);
    void resizeEvent(QResizeEvent *e);
    QStyleOptionComboBox comboStyleOption() const;

signals:
    void itemSelected(const QModelIndex &);
    void resetButton();

private:
    ComboBox *combo;
    QAbstractItemView *view;
    internal::ComboBoxScroller *top;
    internal::ComboBoxScroller *bottom;
};

}



}  // namespace widgets

#endif // COMBOBOX_H
