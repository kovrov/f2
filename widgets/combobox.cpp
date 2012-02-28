#include <QStyle>
#include <QAbstractItemView>
#include <QTimerEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QStack>
#include <QTreeView>
#include <QHeaderView>
#include <QLayout>
#include <QInputContext>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QListView>
#include <QStylePainter>
#include <QGraphicsProxyWidget>
#include <qmath.h>

#include <qdebug.h>



#include "combobox.h"


using namespace widgets;


void internal::ComboBoxContainer::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == adjustSizeTimer.timerId()) {
        adjustSizeTimer.stop();
        if (combo->sizeAdjustPolicy() == ComboBox::AdjustToContents) {
            combo->updateGeometry();
            combo->adjustSize();
            combo->update();
        }
    }
}

void internal::ComboBoxContainer::resizeEvent(QResizeEvent *e)
{
    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
        QStyleOption myOpt;
        myOpt.initFrom(this);
        QStyleHintReturnMask mask;
        if (combo->style()->styleHint(QStyle::SH_Menu_Mask, &myOpt, this, &mask)) {
            setMask(mask.region);
        }
    } else {
        clearMask();
    }
    QFrame::resizeEvent(e);
}

internal::ComboBoxContainer::ComboBoxContainer(QAbstractItemView *itemView, ComboBox *parent)
    : QFrame(parent, Qt::Popup), combo(parent), view(0), top(0), bottom(0)
{
    // we need the combobox and itemview
    Q_ASSERT(parent);
    Q_ASSERT(itemView);

    setAttribute(Qt::WA_WindowPropagation);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo);

    // setup container
    blockMouseReleaseTimer.setSingleShot(true);

    // we need a vertical layout
    QBoxLayout *layout =  new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setSpacing(0);
    layout->setMargin(0);

    // set item view
    setItemView(itemView);

    // add scroller arrows if style needs them
    QStyleOptionComboBox opt = comboStyleOption();
    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
    if (usePopup) {
        top = new internal::ComboBoxScroller(QAbstractSlider::SliderSingleStepSub, this);
        bottom = new internal::ComboBoxScroller(QAbstractSlider::SliderSingleStepAdd, this);
        top->hide();
        bottom->hide();
    } else {
        setLineWidth(1);
    }

    setFrameStyle(combo->style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, &opt, combo));

    if (top) {
        layout->insertWidget(0, top);
        connect(top, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }
    if (bottom) {
        layout->addWidget(bottom);
        connect(bottom, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }

    // Some styles (Mac) have a margin at the top and bottom of the popup.
    layout->insertSpacing(0, 0);
    layout->addSpacing(0);
    updateTopBottomMargin();
}

void internal::ComboBoxContainer::scrollItemView(int action)
{
    if (view->verticalScrollBar())
        view->verticalScrollBar()->triggerAction(static_cast<QAbstractSlider::SliderAction>(action));
}

/*
    Hides or shows the scrollers when we emulate a popupmenu
*/
void internal::ComboBoxContainer::updateScrollers()
{
    if (!top || !bottom)
        return;

    if (isVisible() == false)
        return;

    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo) &&
        view->verticalScrollBar()->minimum() < view->verticalScrollBar()->maximum()) {

        bool needTop = view->verticalScrollBar()->value()
                       > (view->verticalScrollBar()->minimum() + spacing());
        bool needBottom = view->verticalScrollBar()->value()
                          < (view->verticalScrollBar()->maximum() - spacing()*2);
        if (needTop)
            top->show();
        else
            top->hide();
        if (needBottom)
            bottom->show();
        else
            bottom->hide();
    } else {
        top->hide();
        bottom->hide();
    }
}

/*
    Cleans up when the view is destroyed.
*/
void internal::ComboBoxContainer::viewDestroyed()
{
    view = 0;
    setItemView(new QListView());
}

/*
    Returns the item view used for the combobox popup.
*/
QAbstractItemView *internal::ComboBoxContainer::itemView() const
{
    return view;
}

/*!
    Sets the item view to be used for the combobox popup.
*/
void internal::ComboBoxContainer::setItemView(QAbstractItemView *itemView)
{
    Q_ASSERT(itemView);

    // clean up old one
    if (view) {
        view->removeEventFilter(this);
        view->viewport()->removeEventFilter(this);
        disconnect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                   this, SLOT(updateScrollers()));
        disconnect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
                   this, SLOT(updateScrollers()));
        disconnect(view, SIGNAL(destroyed()),
                   this, SLOT(viewDestroyed()));

        delete view;
        view = 0;
    }

    // setup the item view
    view = itemView;
    view->setParent(this);
    view->setAttribute(Qt::WA_MacShowFocusRect, false);
    qobject_cast<QBoxLayout*>(layout())->insertWidget(top ? 2 : 0, view);
    view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QStyleOptionComboBox opt = comboStyleOption();

    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
    if (usePopup)
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (combo->style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
        usePopup) {
        view->setMouseTracking(true);
    }
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setFrameStyle(QFrame::NoFrame);
    view->setLineWidth(0);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(updateScrollers()));
    connect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
            this, SLOT(updateScrollers()));
    connect(view, SIGNAL(destroyed()),
            this, SLOT(viewDestroyed()));
}

/*!
    Returns the spacing between the items in the view.
*/
int internal::ComboBoxContainer::spacing() const
{
    QListView *lview = qobject_cast<QListView*>(view);
    if (lview)
        return lview->spacing();
    return 0;
}

void internal::ComboBoxContainer::updateTopBottomMargin()
{
    if (!layout() || layout()->count() < 1)
        return;

    QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(layout());
    if (!boxLayout)
        return;

    const QStyleOptionComboBox opt = comboStyleOption();
    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
    const int margin = usePopup ? combo->style()->pixelMetric(QStyle::PM_MenuVMargin, &opt, combo) : 0;

    QSpacerItem *topSpacer = boxLayout->itemAt(0)->spacerItem();
    if (topSpacer)
        topSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QSpacerItem *bottomSpacer = boxLayout->itemAt(boxLayout->count() - 1)->spacerItem();
    if (bottomSpacer && bottomSpacer != topSpacer)
        bottomSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);

    boxLayout->invalidate();
}

void internal::ComboBoxContainer::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        QStyleOptionComboBox opt = comboStyleOption();
        view->setMouseTracking(combo->style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
                               combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo));
        setFrameStyle(combo->style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, &opt, combo));
    }

    QWidget::changeEvent(e);
}


bool internal::ComboBoxContainer::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (view->currentIndex().isValid() && (view->currentIndex().flags() & Qt::ItemIsEnabled) ) {
                combo->hidePopup();
                emit itemSelected(view->currentIndex());
            }
            return true;
        case Qt::Key_Down:
            if (!(static_cast<QKeyEvent*>(e)->modifiers() & Qt::AltModifier))
                break;
            // fall through
        case Qt::Key_F4:
        case Qt::Key_Escape:
            combo->hidePopup();
            return true;
        default:
            break;
        }
    break;
    case QEvent::MouseMove:
        if (isVisible()) {
            QMouseEvent *m = static_cast<QMouseEvent *>(e);
            QWidget *widget = static_cast<QWidget *>(o);
            QPoint vector = widget->mapToGlobal(m->pos()) - initialClickPosition;
            if (vector.manhattanLength() > 9 && blockMouseReleaseTimer.isActive())
                blockMouseReleaseTimer.stop();
            QModelIndex indexUnderMouse = view->indexAt(m->pos());
            if (indexUnderMouse.isValid() && indexUnderMouse != view->currentIndex()
                    && !(indexUnderMouse.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))) {
                view->setCurrentIndex(indexUnderMouse);
            }
        }
        break;
    case QEvent::MouseButtonRelease: {
        QMouseEvent *m = static_cast<QMouseEvent *>(e);
        if (isVisible() && view->rect().contains(m->pos()) && view->currentIndex().isValid()
            && !blockMouseReleaseTimer.isActive()
            && (view->currentIndex().flags() & Qt::ItemIsEnabled)
            && (view->currentIndex().flags() & Qt::ItemIsSelectable)) {
            combo->hidePopup();
            emit itemSelected(view->currentIndex());
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QFrame::eventFilter(o, e);
}

void internal::ComboBoxContainer::showEvent(QShowEvent *)
{
    combo->update();
}

void internal::ComboBoxContainer::hideEvent(QHideEvent *)
{
    emit resetButton();
    combo->update();
    // QGraphicsScenePrivate::removePopup closes the combo box popup, it hides it non-explicitly.
    // Hiding/showing the QComboBox after this will unexpectedly show the popup as well.
    // Re-hiding the popup container makes sure it is explicitly hidden.
    if (QGraphicsProxyWidget *proxy = graphicsProxyWidget())
        proxy->hide();
}

void internal::ComboBoxContainer::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED (e);
    QStyleOptionComboBox opt = comboStyleOption();
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_ComboBoxArrow;
    combo->hidePopup();
}

void internal::ComboBoxContainer::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if (!blockMouseReleaseTimer.isActive()){
        combo->hidePopup();
        emit resetButton();
    }
}

QStyleOptionComboBox internal::ComboBoxContainer::comboStyleOption() const
{
    // This should use QComboBox's initStyleOption(), but it's protected
    // perhaps, we could cheat by having the QCombo private instead?
    QStyleOptionComboBox opt;
    opt.initFrom(combo);
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_None;
    opt.editable = false; //combo->isEditable();
    return opt;
}


//------------------------------------------------------------------------------


internal::ComboBoxScroller::ComboBoxScroller(QAbstractSlider::SliderAction action, QWidget *parent)
    : QWidget(parent), sliderAction(action)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAttribute(Qt::WA_NoMousePropagation);
}

QSize internal::ComboBoxScroller::sizeHint() const
{
    return QSize(20, style()->pixelMetric(QStyle::PM_MenuScrollerHeight));
}

void internal::ComboBoxScroller::enterEvent(QEvent *)
{
    startTimer();
}

void internal::ComboBoxScroller::leaveEvent(QEvent *)
{
    stopTimer();
}

void internal::ComboBoxScroller::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timer.timerId()) {
        emit doScroll(sliderAction);
        if (fast) {
            emit doScroll(sliderAction);
            emit doScroll(sliderAction);
        }
    }
}

void internal::ComboBoxScroller::hideEvent(QHideEvent *)
{
    stopTimer();
}

void internal::ComboBoxScroller::mouseMoveEvent(QMouseEvent *e)
{
    // Enable fast scrolling if the cursor is directly above or below the popup.
    const int mouseX = e->pos().x();
    const int mouseY = e->pos().y();
    const bool horizontallyInside = pos().x() < mouseX && mouseX < rect().right() + 1;
    const bool verticallyOutside = (sliderAction == QAbstractSlider::SliderSingleStepAdd)
            ? rect().bottom() + 1 < mouseY
            : mouseY < pos().y();

    fast = horizontallyInside && verticallyOutside;
}

void internal::ComboBoxScroller::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionMenuItem menuOpt;
    menuOpt.init(this);
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.menuRect = rect();
    menuOpt.maxIconWidth = 0;
    menuOpt.tabWidth = 0;
    menuOpt.menuItemType = QStyleOptionMenuItem::Scroller;
    if (sliderAction == QAbstractSlider::SliderSingleStepAdd)
        menuOpt.state |= QStyle::State_DownArrow;
    p.eraseRect(rect());
    style()->drawControl(QStyle::CE_MenuScroller, &menuOpt, &p);
}



//------------------------------------------------------------------------------


ComboBox::ComboBox(QWidget *parent) :
    QWidget(parent),
    mModel(0),
    mContainer(0),
    mSizeAdjustPolicy(ComboBox::AdjustToContentsOnFirstShow),
    mMinimumContentsLength(0),
    mShownOnce(false),
    mMaxVisibleItems(10),
    mModelColumn(0),
    mArrowState(QStyle::State_None),
    mHoverControl(QStyle::SC_None)
{
    setFocusPolicy(Qt::WheelFocus);
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed, QSizePolicy::ComboBox));
    /// setLayoutItemMargins(QStyle::SE_ComboBoxLayoutItem);
    setModel(new QStandardItemModel(0, 1, this));
    setAttribute(Qt::WA_InputMethodEnabled, false);

    // used to be done in QStyle::polish, but it unaware of custom widgets.
    setAttribute(Qt::WA_Hover, true);
}


ComboBox::~ComboBox()
{
    disconnect(mModel, SIGNAL(destroyed()), this, SLOT(_q_modelDestroyed()));
}


ComboBox::SizeAdjustPolicy ComboBox::sizeAdjustPolicy() const
{
    return mSizeAdjustPolicy;
}

void ComboBox::setSizeAdjustPolicy(ComboBox::SizeAdjustPolicy policy)
{
    if (policy == mSizeAdjustPolicy)
        return;

    mSizeAdjustPolicy = policy;
    mSizeHint = QSize();
    adjustComboBoxSize();
    updateGeometry();
}


int ComboBox::minimumContentsLength() const
{
    return mMinimumContentsLength;
}

void ComboBox::setMinimumContentsLength(int characters)
{
    if (characters == mMinimumContentsLength || characters < 0)
        return;

    mMinimumContentsLength = characters;

    if (mSizeAdjustPolicy == AdjustToContents
            || mSizeAdjustPolicy == AdjustToMinimumContentsLength
            || mSizeAdjustPolicy == AdjustToMinimumContentsLengthWithIcon) {
        mSizeHint = QSize();
        adjustComboBoxSize();
        updateGeometry();
    }
}


QSize ComboBox::iconSize() const
{
    if (mIconSize.isValid())
        return mIconSize;

    int iconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    return QSize(iconWidth, iconWidth);
}


void ComboBox::setIconSize(const QSize &size)
{
    if (size == mIconSize)
        return;

    view()->setIconSize(size);
    mIconSize = size;
    mSizeHint = QSize();
    updateGeometry();
}


QAbstractItemModel *ComboBox::model() const
{
    if (NULL == mModel) {
        ComboBox *that = const_cast<ComboBox*>(this);
        that->setModel(new QStandardItemModel(0, 1, that));
    }
    return mModel;
}


void ComboBox::setModel(QAbstractItemModel *model)
{
    //qWarning() << "[setModel]" << model;
    if (!model) {
        qWarning("QComboBox::setModel: cannot set a 0 model");
        return;
    }

    if (mModel) {
        disconnect(mModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
        disconnect(mModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
        disconnect(mModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(mModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
        disconnect(mModel, SIGNAL(destroyed()), this, SLOT(_q_modelDestroyed()));
        disconnect(mModel, SIGNAL(modelAboutToBeReset()), this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(mModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));
        if (mModel->QObject::parent() == this)
            delete mModel;
    }

    mModel = model;

    connect(mModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    connect(mModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(_q_updateIndexBeforeChange()));
    connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    connect(mModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(_q_updateIndexBeforeChange()));
    connect(mModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
    connect(mModel, SIGNAL(destroyed()), this, SLOT(_q_modelDestroyed()));
    connect(mModel, SIGNAL(modelAboutToBeReset()), this, SLOT(_q_updateIndexBeforeChange()));
    connect(mModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));

    if (mContainer)
        mContainer->itemView()->setModel(mModel);

    if (mSizeAdjustPolicy == ComboBox::AdjustToContents) {
        mSizeHint = QSize();
        adjustComboBoxSize();
        updateGeometry();
    }
}


QModelIndex ComboBox::rootModelIndex() const
{
    return QModelIndex(mRoot);
}


void ComboBox::setRootModelIndex(const QModelIndex &index)
{
    mRoot = QPersistentModelIndex(index);
    view()->setRootIndex(index);
    update();
}


int ComboBox::modelColumn() const
{
    return mModelColumn;
}


void ComboBox::setModelColumn(int visibleColumn)
{
    mModelColumn = visibleColumn;
    QListView *lv = qobject_cast<QListView *>(viewContainer()->itemView());
    if (lv)
        lv->setModelColumn(visibleColumn);
    /// WTF?
    setCurrentIndex(mCurrentIndex); //update the text to the text of the new column;
}


QModelIndex ComboBox::currentIndex() const
{
    return mCurrentIndex;
}


QAbstractItemView *ComboBox::view() const
{
    return const_cast<ComboBox*>(this)->viewContainer()->itemView();
}


void ComboBox::setView(QAbstractItemView *itemView)
{
    if (!itemView) {
        qWarning("QComboBox::setView: cannot set a 0 view");
        return;
    }

    if (itemView->model() != mModel)
        itemView->setModel(mModel);
    viewContainer()->setItemView(itemView);
}

/*! \reimp */
QSize ComboBox::sizeHint() const
{
    return recomputeSizeHint(mSizeHint);
}

/*! \reimp */
QSize ComboBox::minimumSizeHint() const
{
    return recomputeSizeHint(mMinimumSizeHint);
}


void ComboBox::showPopup()
{
    if (mModel->rowCount(mRoot) <= 0)
        return;

    QStyle * const style = this->style();

    // set current item and select it
    view()->selectionModel()->setCurrentIndex(mCurrentIndex,
                                              QItemSelectionModel::ClearAndSelect);
    internal::ComboBoxContainer* container = viewContainer();
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QRect listRect(style->subControlRect(QStyle::CC_ComboBox, &opt,
                                         QStyle::SC_ComboBoxListBoxPopup, this));

    QRect screen = QApplication::desktop()->availableGeometry(QApplication::desktop()->screenNumber(this));
    QPoint below = mapToGlobal(listRect.bottomLeft());
    int belowHeight = screen.bottom() - below.y();
    QPoint above = mapToGlobal(listRect.topLeft());
    int aboveHeight = above.y() - screen.y();
    bool boundToScreen = !window()->testAttribute(Qt::WA_DontShowOnScreen);

    const bool usePopup = style->styleHint(QStyle::SH_ComboBox_Popup, &opt, this);
    {
        int listHeight = 0;
        int count = 0;
        QStack<QModelIndex> toCheck;
        toCheck.push(view()->rootIndex());
        QTreeView *treeView = qobject_cast<QTreeView*>(view());
        if (treeView && treeView->header() && !treeView->header()->isHidden())
            listHeight += treeView->header()->height();
        while (!toCheck.isEmpty()) {
            QModelIndex parent = toCheck.pop();
            for (int i = 0; i < mModel->rowCount(parent); ++i) {
                QModelIndex idx = mModel->index(i, mModelColumn, parent);
                if (!idx.isValid())
                    continue;
                listHeight += view()->visualRect(idx).height() + container->spacing();
                if (mModel->hasChildren(idx) && treeView && treeView->isExpanded(idx))
                    toCheck.push(idx);
                ++count;
                if (!usePopup && count >= mMaxVisibleItems) {
                    toCheck.clear();
                    break;
                }
            }
        }
        listRect.setHeight(listHeight);
    }

    {
        // add the spacing for the grid on the top and the bottom;
        int heightMargin = 2*container->spacing();

        // add the frame of the container
        int marginTop, marginBottom;
        container->getContentsMargins(0, &marginTop, 0, &marginBottom);
        heightMargin += marginTop + marginBottom;

        //add the frame of the view
        view()->getContentsMargins(0, &marginTop, 0, &marginBottom);
        /// marginTop += static_cast<QAbstractScrollAreaPrivate *>(QObjectPrivate::get(view()))->top;
        /// marginBottom += static_cast<QAbstractScrollAreaPrivate *>(QObjectPrivate::get(view()))->bottom;
        heightMargin += marginTop + marginBottom;

        listRect.setHeight(listRect.height() + heightMargin);
    }

    // Add space for margin at top and bottom if the style wants it.
    if (usePopup)
        listRect.setHeight(listRect.height() + style->pixelMetric(QStyle::PM_MenuVMargin, &opt, this) * 2);

    // Make sure the popup is wide enough to display its contents.
    if (usePopup) {
        const int diff = computeWidthHint() - width();
        if (diff > 0)
            listRect.setWidth(listRect.width() + diff);
    }

    //we need to activate the layout to make sure the min/maximum size are set when the widget was not yet show
    container->layout()->activate();
    //takes account of the minimum/maximum size of the container
    listRect.setSize( listRect.size().expandedTo(container->minimumSize())
                      .boundedTo(container->maximumSize()));

    // make sure the widget fits on screen
    if (boundToScreen) {
        if (listRect.width() > screen.width() )
            listRect.setWidth(screen.width());
        if (mapToGlobal(listRect.bottomRight()).x() > screen.right()) {
            below.setX(screen.x() + screen.width() - listRect.width());
            above.setX(screen.x() + screen.width() - listRect.width());
        }
        if (mapToGlobal(listRect.topLeft()).x() < screen.x() ) {
            below.setX(screen.x());
            above.setX(screen.x());
        }
    }

    if (usePopup) {
        // Position horizontally.
        listRect.moveLeft(above.x());

        // Position vertically so the curently selected item lines up
        // with the combo box.
        const QRect currentItemRect = view()->visualRect(view()->currentIndex());
        const int offset = listRect.top() - currentItemRect.top();
        listRect.moveTop(above.y() + offset - listRect.top());

        // Clamp the listRect height and vertical position so we don't expand outside the
        // available screen geometry.This may override the vertical position, but it is more
        // important to show as much as possible of the popup.
        const int height = !boundToScreen ? listRect.height() : qMin(listRect.height(), screen.height());
        listRect.setHeight(height);

        if (boundToScreen) {
            if (listRect.top() < screen.top())
                listRect.moveTop(screen.top());
            if (listRect.bottom() > screen.bottom())
                listRect.moveBottom(screen.bottom());
        }
    } else if (!boundToScreen || listRect.height() <= belowHeight) {
        listRect.moveTopLeft(below);
    } else if (listRect.height() <= aboveHeight) {
        listRect.moveBottomLeft(above);
    } else if (belowHeight >= aboveHeight) {
        listRect.setHeight(belowHeight);
        listRect.moveTopLeft(below);
    } else {
        listRect.setHeight(aboveHeight);
        listRect.moveBottomLeft(above);
    }

    QScrollBar *sb = view()->horizontalScrollBar();
    Qt::ScrollBarPolicy policy = view()->horizontalScrollBarPolicy();
    bool needHorizontalScrollBar = (policy == Qt::ScrollBarAsNeeded || policy == Qt::ScrollBarAlwaysOn)
                                   && sb->minimum() < sb->maximum();
    if (needHorizontalScrollBar) {
        listRect.adjust(0, 0, 0, sb->height());
    }
    container->setGeometry(listRect);
    const bool updatesEnabled = container->updatesEnabled();

// Don't disable updates on Mac OS X. Windows are displayed immediately on this platform,
// which means that the window will be visible before the call to container->show() returns.
// If updates are disabled at this point we'll miss our chance at painting the popup
// menu before it's shown, causing flicker since the window then displays the standard gray
// background.
    container->setUpdatesEnabled(false);

    container->raise();
    container->show();
    container->updateScrollers();
    view()->setFocus();

    view()->scrollTo(view()->currentIndex(),
                     style->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)
                             ? QAbstractItemView::PositionAtCenter
                             : QAbstractItemView::EnsureVisible);

    container->setUpdatesEnabled(updatesEnabled);
    container->update();
}


void ComboBox::hidePopup()
{
    if (mContainer && mContainer->isVisible()) {
        mContainer->hide();
    }
    _q_resetButton();
}


/// FIXME recomputeSizeHint needs refactoring
QSize ComboBox::recomputeSizeHint(QSize &sh) const
{
    if (sh.isValid())
        return sh.expandedTo(QApplication::globalStrut());

    bool has_icon = mSizeAdjustPolicy == ComboBox::AdjustToMinimumContentsLengthWithIcon ? true : false;
    const int count = mModel->rowCount(mRoot);
    const QSize icon_size = iconSize();
    const QFontMetrics &fm = fontMetrics();

    // text width
    if (&sh != &mSizeHint && mMinimumContentsLength != 0) {
        for (int i = 0; i < count && !has_icon; ++i) {
            const QModelIndex &index = mModel->index(i, mModelColumn, mRoot);
            has_icon = !itemIcon(index).isNull();
        }
    }
    else {
        switch (mSizeAdjustPolicy) {

        case ComboBox::AdjustToContents:
        case ComboBox::AdjustToContentsOnFirstShow:
            if (count == 0) {
                sh.rwidth() = 7 * fm.width(QLatin1Char('x'));
            }
            else {
                for (int i = 0; i < count; ++i) {
                    const QModelIndex &index = mModel->index(i, mModelColumn, mRoot);
                    if (itemIcon(index).isNull()) {
                        sh.setWidth(qMax(sh.width(), fm.boundingRect(itemText(index)).width()));
                    }
                    else {
                        has_icon = true;
                        sh.setWidth(qMax(sh.width(), fm.boundingRect(itemText(index)).width() + icon_size.width() + 4));
                    }
                }
            }
            break;

        case ComboBox::AdjustToMinimumContentsLength:
            for (int i = 0; i < count && !has_icon; ++i) {
                const QModelIndex &index = mModel->index(i, mModelColumn, mRoot);
                has_icon = !itemIcon(index).isNull();
            }

        default: ;
        }
    }

    if (mMinimumContentsLength > 0)
        sh.setWidth(qMax(sh.width(), mMinimumContentsLength * fm.width(QLatin1Char('X')) + (has_icon ? icon_size.width() + 4 : 0)));

    // height
    sh.setHeight(qMax(qCeil(QFontMetricsF(fm).height()), 14) + 2);
    if (has_icon) {
        sh.setHeight(qMax(sh.height(), icon_size.height() + 2));
    }

    // add style and strut values
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    sh = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, sh, this);

    return sh.expandedTo(QApplication::globalStrut());
}


QIcon ComboBox::itemIcon(const QModelIndex &index) const
{
    const QVariant &decoration = mModel->data(index, Qt::DecorationRole);

    if (decoration.type() == QVariant::Icon) {
        /// FIXME: proper icon size
        /// return decoration.value<QIcon>();
        return QIcon(decoration.value<QIcon>().pixmap(iconSize()));
    }

    if (decoration.type() == QVariant::Pixmap)
        return QIcon(decoration.value<QPixmap>());

    return QIcon();
}


QString ComboBox::itemText(const QModelIndex &index) const
{
    return index.isValid() ? mModel->data(index, Qt::DisplayRole).toString() : QString();
}


void ComboBox::adjustComboBoxSize()
{
    viewContainer()->adjustSizeTimer.start(20, mContainer);
}


void ComboBox::setCurrentIndex(const QModelIndex &mi)
{
    //qWarning() << "[setCurrentIndex]" << mi;
    QModelIndex normalized;
    if (mi.column() != mModelColumn)
        normalized = mModel->index(mi.row(), mModelColumn, mi.parent());
    if (!normalized.isValid())
        normalized = mi;    // Fallback to passed index.

    bool indexChanged = (normalized != mCurrentIndex);
    if (indexChanged) {
        mCurrentIndex = QPersistentModelIndex(normalized);
        update();
        emit currentIndexChanged(mCurrentIndex); // _q_emitCurrentIndexChanged
    }
}


void ComboBox::_q_itemSelected(const QModelIndex &index)
{
    //qWarning() << "[_q_itemSelected]" << index;
    if (index != mCurrentIndex)
        setCurrentIndex(index);

    emitActivated(mCurrentIndex);
}


void ComboBox::_q_emitHighlighted(const QModelIndex &index)
{
    //qWarning() << "[_q_emitHighlighted]" << index;
    if (!index.isValid())
        return;

    emit highlighted(index);
}


void ComboBox::_q_emitCurrentIndexChanged(const QModelIndex &index)
{
    //qWarning() << "[_q_emitCurrentIndexChanged]" << index;
    emit currentIndexChanged(index);
}


void ComboBox::_q_resetButton()
{
    //qWarning() << "[_q_resetButton]";
    updateArrow(QStyle::State_None);
}


void ComboBox::_q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    //qWarning() << "[_q_dataChanged]" << topLeft << bottomRight;
    if (/*inserting ||*/ topLeft.parent() != mRoot)
        return;

    if (mSizeAdjustPolicy == ComboBox::AdjustToContents) {
        mSizeHint = QSize();
        adjustComboBoxSize();
        updateGeometry();
    }

    if (mCurrentIndex.row() >= topLeft.row() && mCurrentIndex.row() <= bottomRight.row()) {
        update();
    }
}


void ComboBox::_q_updateIndexBeforeChange()
{
    //qWarning() << "[_q_updateIndexBeforeChange]";
    mIndexBeforeChange = mCurrentIndex;
}


void ComboBox::_q_rowsInserted(const QModelIndex &parent, int start, int end)
{
    //qWarning() << "[_q_rowsInserted]" << parent << start << end;
    if (/*inserting ||*/ parent != mRoot)
        return;

    if (mSizeAdjustPolicy == ComboBox::AdjustToContents) {
        mSizeHint = QSize();
        adjustComboBoxSize();
        updateGeometry();
    }

    // set current index if combo was previously empty
    int count = mModel->rowCount(mRoot);
    if (start == 0 && (end - start + 1) == count && !mCurrentIndex.isValid()) {
        QModelIndex index = mModel->index(0, mModelColumn, mRoot);
        setCurrentIndex(index);
        // need to emit changed if model updated index "silently"
    }
    else if (mCurrentIndex != mIndexBeforeChange) {
        update();
        _q_emitCurrentIndexChanged(mCurrentIndex);
    }
}


void ComboBox::_q_rowsRemoved(const QModelIndex &parent, int start, int end)
{
    //qWarning() << "[_q_rowsRemoved]" << parent << start << end;
    Q_UNUSED (start);
    Q_UNUSED (end);

    if (parent != mRoot)
        return;

    if (mSizeAdjustPolicy == ComboBox::AdjustToContents) {
        mSizeHint = QSize();
        adjustComboBoxSize();
        updateGeometry();
    }

    // model has changed the currentIndex
    if (mCurrentIndex != mIndexBeforeChange) {

        const int count = mModel->rowCount(mRoot) /*count()*/;
        if (!mCurrentIndex.isValid() && count) {
            QModelIndex index = mModel->index(qMin(count - 1, qMax(mIndexBeforeChange.row(), 0)), mModelColumn, mRoot);
            setCurrentIndex(index);
            return;
        }

        update();
        _q_emitCurrentIndexChanged(mCurrentIndex);
    }
}


void ComboBox::_q_modelDestroyed()
{
    //qWarning() << "[_q_modelDestroyed]";
    mModel = NULL; //QAbstractItemModelPrivate::staticEmptyModel();
}


void ComboBox::_q_modelReset()
{
    //qWarning() << "[_q_modelReset]";
    if (mCurrentIndex != mIndexBeforeChange)
        _q_emitCurrentIndexChanged(mCurrentIndex);
    update();
}

/*! \reimp */
bool ComboBox::event(QEvent *event)
{
    switch (event->type()) {

    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
        updateLayoutDirection();
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            updateHoverControl(he->pos());
        break;

    default:
        break;
    }

    return QWidget::event(event);
}

/*! \reimp */
void ComboBox::focusInEvent(QFocusEvent *e)
{
    Q_UNUSED (e);
    update();
}

/*! \reimp */
void ComboBox::focusOutEvent(QFocusEvent *e)
{
    Q_UNUSED (e);
    update();
}

/*! \reimp */
void ComboBox::changeEvent(QEvent *e)
{
    switch (e->type()) {

    case QEvent::StyleChange:
        mSizeHint = QSize(); // invalidate size hint
        mMinimumSizeHint = QSize();
        updateLayoutDirection();
        /// setLayoutItemMargins(QStyle::SE_ComboBoxLayoutItem);
        // need to update scrollers etc. as well here
        break;

    case QEvent::EnabledChange:
        if (!isEnabled())
            hidePopup();
        break;

    case QEvent::PaletteChange: {
        updateViewContainerPaletteAndOpacity();
        break;
    }
    case QEvent::FontChange:
        mSizeHint = QSize(); // invalidate size hint
        viewContainer()->setFont(font());
        break;

    default:
        break;
    }

    QWidget::changeEvent(e);
}

/*! \reimp */
void ComboBox::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // draw the combobox frame, focusrect and selected etc.
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

/*! \reimp */
void ComboBox::showEvent(QShowEvent *e)
{
    if (!mShownOnce && mSizeAdjustPolicy == ComboBox::AdjustToContentsOnFirstShow) {
        mSizeHint = QSize();
        updateGeometry();
    }
    mShownOnce = true;
    QWidget::showEvent(e);
}

/*! \reimp */
void ComboBox::hideEvent(QHideEvent *)
{
    hidePopup();
}

/*! \reimp */
void ComboBox::mousePressEvent(QMouseEvent *e)
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStyle::SubControl sc = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, e->pos(), this);
    if (e->button() == Qt::LeftButton && (sc == QStyle::SC_ComboBoxArrow || true/*!isEditable()*/) && !viewContainer()->isVisible()) {
        if (sc == QStyle::SC_ComboBoxArrow)
            updateArrow(QStyle::State_Sunken);
            // We've restricted the next couple of lines, because by not calling
            // viewContainer(), we avoid creating the QComboBoxPrivateContainer.
            viewContainer()->blockMouseReleaseTimer.start(QApplication::doubleClickInterval());
            viewContainer()->initialClickPosition = mapToGlobal(e->pos());
        showPopup();
    }
    else {
        QWidget::mousePressEvent(e);
    }
}

/*! \reimp */
void ComboBox::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    updateArrow(QStyle::State_None);
}

/*! \reimp */
void ComboBox::keyPressEvent(QKeyEvent *e)
{
    enum Move { NoMove=0 , MoveUp , MoveDown , MoveFirst , MoveLast};

    Move move = NoMove;
    switch (e->key()) {
    case Qt::Key_Up:
        if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
    case Qt::Key_PageUp:
        move = MoveUp;
        break;
    case Qt::Key_Down:
        if (e->modifiers() & Qt::AltModifier) {
            showPopup();
            return;
        } else if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
        // fall through
    case Qt::Key_PageDown:
        move = MoveDown;
        break;
    case Qt::Key_Home:
        move = MoveFirst;
        break;
    case Qt::Key_End:
        move = MoveLast;
        break;
    case Qt::Key_F4:
        if (!e->modifiers()) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Space:
        showPopup();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        e->ignore();
        break;
    default:
        if (!e->text().isEmpty())
            keyboardSearchString(e->text());
        else
            e->ignore();
    }

    /// FIXME !!!
    int new_index = mCurrentIndex.row();

    if (move != NoMove) {
        e->accept();
        switch (move) {
        case MoveFirst:
            new_index = QModelIndex().row();
        case MoveDown: {
            new_index++;
            int count = mModel->rowCount(mRoot);
            while ((new_index < count) && !(mModel->flags(mModel->index(new_index,mModelColumn,mRoot)) & Qt::ItemIsEnabled))
                new_index++;
            break;
        }
        case MoveLast:
            new_index = mModel->rowCount(mRoot);
        case MoveUp:
            new_index--;
            while ((new_index >= 0) && !(mModel->flags(mModel->index(new_index,mModelColumn,mRoot)) & Qt::ItemIsEnabled))
                new_index--;
            break;
        default:
            e->ignore();
            break;
        }

        int count = mModel->rowCount(mRoot);
        if (new_index >= 0 && new_index < count && new_index != mCurrentIndex.row()) {
            QModelIndex index = mModel->index(new_index, mModelColumn, mRoot);
            setCurrentIndex(index);
            emitActivated(mCurrentIndex);
        }
    }
}

/*! \reimp */
void ComboBox::wheelEvent(QWheelEvent *e)
{
    if (!viewContainer()->isVisible()) {
        /// FIXME !!!
        int new_index = mCurrentIndex.row();

        if (e->delta() > 0) {
            new_index--;
            while ((new_index >= 0) && !(mModel->flags(mModel->index(new_index,mModelColumn,mRoot)) & Qt::ItemIsEnabled))
                new_index--;
        } else {
            new_index++;
            int count = mModel->rowCount(mRoot);
            while ((new_index < count) && !(mModel->flags(mModel->index(new_index,mModelColumn,mRoot)) & Qt::ItemIsEnabled))
                new_index++;
        }

        int count = mModel->rowCount(mRoot);
        if (new_index >= 0 && new_index < count && new_index != mCurrentIndex.row()) {
            QModelIndex index = mModel->index(new_index, mModelColumn, mRoot);
            setCurrentIndex(index);
            emitActivated(mCurrentIndex);
        }
        e->accept();
    }
}


internal::ComboBoxContainer* ComboBox::viewContainer()
{
    if (mContainer)
        return mContainer;

    mContainer = new internal::ComboBoxContainer(new QListView(this), this);
    mContainer->itemView()->setModel(mModel);
    mContainer->itemView()->setTextElideMode(Qt::ElideMiddle);
    updateLayoutDirection();
    updateViewContainerPaletteAndOpacity();
    QObject::connect(mContainer, SIGNAL(itemSelected(QModelIndex)), this, SLOT(_q_itemSelected(QModelIndex)));
    QObject::connect(mContainer->itemView()->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(_q_emitHighlighted(QModelIndex)));
    QObject::connect(mContainer, SIGNAL(resetButton()), this, SLOT(_q_resetButton()));
    return mContainer;
}


void ComboBox::updateViewContainerPaletteAndOpacity()
{
    if (!mContainer)
        return;

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    mContainer->setPalette(palette());
    mContainer->setWindowOpacity(1.0);
}


void ComboBox::initStyleOption(QStyleOptionComboBox *option) const
{
    if (!option)
        return;

    option->initFrom(this);
    option->editable = false; //isEditable();
    option->frame = true; //frame;
    if (hasFocus() && !option->editable)
        option->state |= QStyle::State_Selected;
    option->subControls = QStyle::SC_All;
    if (mArrowState == QStyle::State_Sunken) {
        option->activeSubControls = QStyle::SC_ComboBoxArrow;
        option->state |= mArrowState;
    }
    else {
        option->activeSubControls = mHoverControl;
    }

    if (mCurrentIndex.isValid()) {

        QStringList text;
        QModelIndex index = mCurrentIndex;
        while (index.isValid()) {
            text.prepend(itemText(index));
            index = index.parent();
        }
        option->currentText = text.join(" / ");
        /// FIXME: make configurable
        option->currentIcon = itemIcon(mCurrentIndex);
        option->iconSize = iconSize();
    }

    if (mContainer && mContainer->isVisible())
        option->state |= QStyle::State_On;
}

// Computes a size hint based on the maximum width for the items in the combobox
int ComboBox::computeWidthHint() const
{
    int width = 0;
    const int count = mModel->rowCount(mRoot) /*count()*/;
    const int iconWidth = iconSize().width() + 4;
    const QFontMetrics &font_metrics = fontMetrics();

    for (int i = 0; i < count; ++i) {
        const QModelIndex &index = mModel->index(i, mModelColumn, mRoot);
        const int textWidth = font_metrics.width(itemText(index));
        if (itemIcon(index).isNull())
            width = (qMax(width, textWidth));
        else
            width = (qMax(width, textWidth + iconWidth));
    }

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QSize tmp(width, 0);
    tmp = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp, this);
    return tmp.width();
}


void ComboBox::updateArrow(QStyle::StateFlag state)
{
    if (mArrowState == state)
        return;
    mArrowState = state;
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    update(style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this));
}


void ComboBox::updateLayoutDirection()
{
    if (NULL == mContainer)
        return;
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    Qt::LayoutDirection dir = Qt::LayoutDirection(style()->styleHint(
                QStyle::SH_ComboBox_LayoutDirection, &opt, this));
    mContainer->setLayoutDirection(dir);
}


bool ComboBox::updateHoverControl(const QPoint &pos)
{
    const bool does_hover = testAttribute(Qt::WA_Hover);
    if (!does_hover)
        return false;

    const QRect last_hover_rect = mHoverRect;
    const QStyle::SubControl last_hover_control = mHoverControl;

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    mHoverControl = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, pos, this);
    mHoverRect = (mHoverControl != QStyle::SC_None)
                   ? style()->subControlRect(QStyle::CC_ComboBox, &opt, mHoverControl, this)
                   : QRect();

    if (last_hover_control == mHoverControl)
        return false;

    update(last_hover_rect);
    update(mHoverRect);
    return true;
}


void ComboBox::keyboardSearchString(const QString &text)
{
    // use keyboardSearch from the listView so we do not duplicate code
    QAbstractItemView *view = viewContainer()->itemView();
    view->setCurrentIndex(mCurrentIndex);
    int currentRow = view->currentIndex().row();
    view->keyboardSearch(text);
    if (currentRow != view->currentIndex().row()) {
        setCurrentIndex(view->currentIndex());
        emitActivated(mCurrentIndex);
    }
}


void ComboBox::emitActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    emit activated(index);
}
