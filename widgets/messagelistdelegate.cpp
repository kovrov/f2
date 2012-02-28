#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

#include <qdebug.h>

#include <qmfclient/qmailmessagemodelbase.h> // QMailMessageModelBase
#include <qmfclient/qmailmessage.h> // QMailMessageMetaData

// project
#include "models/messagelistmodel.h"
#include "models/progressinfo.h"

#include "messagelistdelegate.h"

using namespace widgets;


namespace widgets {

namespace {
struct MessageListItemOption
{
    MessageListItemOption(const QStyleOptionViewItem &option, const QModelIndex &index)
      : _index (index),
        _option (option)
    {
        widget = _option.widget;
        style = widget ? widget->style() : QApplication::style();
    }


    QStyle::StateFlag checkState()
    {
        const QVariant &data = _index.data(Qt::CheckStateRole);
        switch (static_cast<Qt::CheckState>(data.toInt())) {
//        case Qt::Unchecked:
//            return QStyle::State_Off;
//        case Qt::Checked:
//            return QStyle::State_On;
//        case Qt::PartiallyChecked:
//            return QStyle::State_NoChange;
        default:
            return QStyle::State_None;
        }
    }


    QRect checkRect()
    {
        if (checkState() == QStyle::State_None)
            return QRect();

        const int h = style->pixelMetric(QStyle::PM_IndicatorHeight, &_option, widget);
        const QRect &bounding_rect = _option.rect;

        QRect rect(bounding_rect.x(),
                   bounding_rect.y() + ((bounding_rect.height() - h) / 2),
                   style->pixelMetric(QStyle::PM_IndicatorWidth, &_option, widget),
                   h);

        if (_option.direction == Qt::RightToLeft)
            rect.translate(2 * (bounding_rect.right() - rect.right())
                           + rect.width() - bounding_rect.width(),
                           0);
        return rect;
    }


    QRect decorationRect()
    {
        QSize decorationSize;

        QVariant value = _index.data(Qt::DecorationRole);
        if (value.isValid() && !value.isNull()) {
            switch (value.type()) {
            case QVariant::Icon: {
                QIcon icon = qvariant_cast<QIcon>(value);
                QIcon::Mode mode;
                if (!(_option.state & QStyle::State_Enabled))
                    mode = QIcon::Disabled;
                else if (_option.state & QStyle::State_Selected)
                    mode = QIcon::Selected;
                else
                    mode = QIcon::Normal;
                QIcon::State state = _option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
                decorationSize = icon.actualSize(decorationSize, mode, state);
                break;
            }
            case QVariant::Color: {
                QPixmap pixmap(decorationSize);
                pixmap.fill(qvariant_cast<QColor>(value));
                /// QIcon icon = QIcon(pixmap);
                break;
            }
            case QVariant::Image: {
                QImage image = qvariant_cast<QImage>(value);
                /// QIcon icon = QIcon(QPixmap::fromImage(image));
                decorationSize = image.size();
                break;
            }
            case QVariant::Pixmap: {
                QPixmap pixmap = qvariant_cast<QPixmap>(value);
                /// QIcon icon = QIcon(pixmap);
                decorationSize = pixmap.size();
                break;
            }
            default:
                break;
            }
        }

        return QRect(QPoint(0, _option.rect.y() + (_option.rect.height() - decorationSize.height()) / 2),
                     decorationSize);
    }


    QRect topTextRect()
    {
        const int margin = 4; // TODO: get from style
        int V_offset = margin;
        if (_option.direction == Qt::LeftToRight) {
            const QRect &check_rect = checkRect();
            V_offset += check_rect.x() + check_rect.width();
        }

        int h = _option.fontMetrics.height();

        return QRect(_option.rect.x() + V_offset,
                     _option.rect.y() + (_option.rect.height() /*- margin*/) / 2 - h,
                     _option.rect.width() - V_offset,
                     h);
    }


    QRect bottomTextRect()
    {
        const int margin = 4; // TODO: get from style
        int v_offset = margin;
        if (_option.direction == Qt::LeftToRight) {
            const QRect &check_rect = checkRect();
            v_offset += check_rect.x() + check_rect.width();
        }

        int h = _option.fontMetrics.height();
        const QRect &date_rect = dateRect();

        return QRect(_option.rect.x() + v_offset,
                     _option.rect.y() + (_option.rect.height()/* + margin*/) / 2,
                     date_rect.x() - (_option.rect.x() + v_offset) - margin,
                     h);
    }


    QRect dateRect()
    {
        const int margin = 4; // TODO: get from style

        int h = _option.fontMetrics.height();
        int width = _option.fontMetrics.width(dateText);

        return QRect(_option.rect.x() + _option.rect.width() - width - margin,
                     _option.rect.y() + (_option.rect.height()/* + margin*/) / 2,
                     width,
                     h);
    }


    QRect focusRect()
    {
        Q_UNUSED (widget);

        return _option.rect;
    }

    QRect progressRect()
    {
        const int margin = 4; // TODO: get from style
        return QRect(_option.rect.x() + margin, _option.rect.y() + _option.rect.height() - 6,
                     _option.rect.width() - margin*2, 6);
    }

    const QStyle *style;
    const QWidget *widget;
    QString dateText;

private:
    const QModelIndex &_index;
    QStyleOptionViewItemV4 _option;
};
}

}


MessageListDelegate::MessageListDelegate(QObject *parent)
  : QAbstractItemDelegate (parent)
{
}


void MessageListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT (index.isValid());
    MessageListItemOption opt(option, index);

    const QVariant &data = index.data(QMailMessageModelBase::MessageIdRole);
    Q_ASSERT (data.canConvert<QMailMessageId>());
    const QMailMessageMetaData message(data.value<QMailMessageId>());
    opt.dateText = message.date().toLocalTime().toString(Qt::SystemLocaleShortDate);

    painter->save();
    painter->setClipRect(option.rect);

    // draw the background
    opt.style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, opt.widget);

    // draw check mark
    {
        QStyleOptionButton button_option;
        button_option.rect = opt.checkRect();
        if (!button_option.rect.isNull()) {
            button_option.state = opt.checkState();
            opt.style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &button_option, painter, opt.widget);
        }
    }

    // draw icon
    {
//        const QRect &icon_rect = msg_opt.decorationRect();
//        QIcon::Mode mode = QIcon::Normal;
//        if (!(option.state & QStyle::State_Enabled))
//            mode = QIcon::Disabled;
//        else if (option.state & QStyle::State_Selected)
//            mode = QIcon::Selected;
//        QIcon::State state = option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
//        /// TODO: option.icon.paint(painter, icon_rect, opt.decorationAlignment, mode, state);
    }

    // draw top text
    {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal
                              : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
        else
            painter->setPen(option.palette.color(cg, QPalette::Text));

        const QRect &top_text_rect = opt.topTextRect();
        painter->drawText(top_text_rect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(message.subject(),
                                                        Qt::ElideRight,
                                                        top_text_rect.width()));
    }

    // draw sender label
    {
        const QRect &bottom_text_rect = opt.bottomTextRect();
        painter->drawText(bottom_text_rect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(message.from().name(),
                                                        Qt::ElideRight,
                                                        bottom_text_rect.width()));
    }

    // draw date label
    {
        const QRect &rect = opt.dateRect();
        painter->drawText(rect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(opt.dateText,
                                                        Qt::ElideRight,
                                                        rect.width()));
    }

    // draw progress info
    const QVariant &progress_data = index.data(models::MessageListModel::ProgressInfoRole);
    if (!progress_data.isNull()) {
        Q_ASSERT (progress_data.canConvert<ProgressInfo>());
        const models::ProgressInfo progress(progress_data.value<models::ProgressInfo>());
        QStyleOptionProgressBar progress_option;
        progress_option.maximum = progress.total();
        progress_option.progress = progress.value();
        progress_option.rect = opt.progressRect();
        opt.style->drawControl(QStyle::CE_ProgressBar, &progress_option, painter, opt.widget);
    }

    // draw focus rect
    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect focus_option;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        focus_option.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
        focus_option.rect = opt.focusRect();
        focus_option.state = option.state | QStyle::State_KeyboardFocusChange | QStyle::State_Item;
        opt.style->drawPrimitive(QStyle::PE_FrameFocusRect, &focus_option, painter, opt.widget);
    }

    painter->restore();
}


QSize MessageListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MessageListItemOption opt(option, index);
    return QSize(0, (opt.decorationRect() | opt.topTextRect() | opt.bottomTextRect() | opt.checkRect()).height() + 8);
}


bool MessageListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_ASSERT (event);
    Q_ASSERT (model);

    MessageListItemOption msg_opt(option, index);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled) || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    const QRect &check_rect = msg_opt.checkRect();
    if (check_rect.isNull())
        return false;

    // make sure that we have the right event type
    switch (event->type()) {

    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        if (static_cast<QMouseEvent*>(event)->button() != Qt::LeftButton
         || !check_rect.contains(static_cast<QMouseEvent*>(event)->pos()))
            return false;
        // eat the double click events inside the check rect
        if (event->type() == QEvent::MouseButtonDblClick)
            return true;
        break;

    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
         && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
        break;

    default:
        return false;
    }

    Qt::CheckState state = msg_opt.checkState() == QStyle::State_Off ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}
