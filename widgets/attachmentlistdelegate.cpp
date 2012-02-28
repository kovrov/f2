#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QFileIconProvider>

#include <qdebug.h>

// project
#include "models/attachmentlistmodel.h"
#include "models/progressinfo.h"

#include "attachmentlistdelegate.h"

using namespace widgets;


namespace widgets {

namespace {
struct AttachmentListItemOption
{
    AttachmentListItemOption(const QStyleOptionViewItem &option, const QModelIndex &index)
      : _index (index),
        _option (option)
    {
        widget = _option.widget;
        style = widget ? widget->style() : QApplication::style();
    }


    QIcon mimeIcon()
    {
        // TODO: AttachmentListModel::MimeTypeRole
        const QString &filename = _index.data(Qt::DisplayRole).toString();
        QFileIconProvider icon_provider;
        return icon_provider.icon(filename);
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
        QSize decoration_size;

        const QVariant &value = _index.data(Qt::DecorationRole);
        if (!value.isValid() || value.isNull()) {
            icon = mimeIcon();
            iconMode = (!(_option.state & QStyle::State_Enabled)) ? QIcon::Disabled
                       : (_option.state & QStyle::State_Selected) ? QIcon::Selected
                                                                  : QIcon::Normal;
            iconState = _option.state & QStyle::State_Open ? QIcon::On
                                                           : QIcon::Off;
            decoration_size = icon.actualSize(QSize(32,32), iconMode, iconState);
        }
        else {
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
                decoration_size = icon.actualSize(decoration_size, mode, state);
                break;
            }
            case QVariant::Color: {
                QPixmap pixmap(decoration_size);
                pixmap.fill(qvariant_cast<QColor>(value));
                /// QIcon icon = QIcon(pixmap);
                break;
            }
            case QVariant::Image: {
                QImage image = qvariant_cast<QImage>(value);
                /// QIcon icon = QIcon(QPixmap::fromImage(image));
                decoration_size = image.size();
                break;
            }
            case QVariant::Pixmap: {
                QPixmap pixmap = qvariant_cast<QPixmap>(value);
                /// QIcon icon = QIcon(pixmap);
                decoration_size = pixmap.size();
                break;
            }
            default:
                break;
            }
        }

        const int margin = 4; // TODO: get from style
        return QRect(QPoint(_option.rect.x() + (_option.rect.width() - decoration_size.width() - margin),
                            _option.rect.y() + (_option.rect.height() - decoration_size.height()) / 2),
                     decoration_size);
    }


    QRect topTextRect()
    {
        const int margin = 4; // TODO: get from style
        int v_offset = margin;
        if (_option.direction == Qt::LeftToRight) {
            const QRect &check_rect = checkRect();
            v_offset += check_rect.x() + check_rect.width();
        }

        int h = _option.fontMetrics.height();
        const QRect &decoration_rect = decorationRect();

        return QRect(_option.rect.x() + v_offset,
                     _option.rect.y() + (_option.rect.height() /*- margin*/) / 2 - h,
                     decoration_rect.x() - v_offset - _option.rect.x() - v_offset,
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
        const QRect &decoration_rect = decorationRect();

        return QRect(_option.rect.x() + v_offset,
                     _option.rect.y() + (_option.rect.height()/* + margin*/) / 2,
                     decoration_rect.x() - v_offset - _option.rect.x() - v_offset,
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
        int v_offset = margin;
        const QRect &decoration_rect = decorationRect();
        return QRect(_option.rect.x() + margin,
                     _option.rect.y() + _option.rect.height() - _option.rect.height() / 4 - 3,
                     decoration_rect.x() - v_offset - _option.rect.x() - margin,
                     6);
    }


    const QStyle *style;
    const QWidget *widget;
    QIcon icon;
    QIcon::Mode iconMode;
    QIcon::State iconState;

private:
    const QModelIndex &_index;
    QStyleOptionViewItemV4 _option;
};
}

}



AttachmentListDelegate::AttachmentListDelegate(QObject *parent)
  : QAbstractItemDelegate (parent)
{
}


void AttachmentListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT (index.isValid());
    AttachmentListItemOption opt(option, index);

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
        opt.icon.paint(painter, opt.decorationRect(),
                       option.decorationAlignment, opt.iconMode, opt.iconState);
        //painter->drawPixmap(...);
    }

    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                          ? QPalette::Normal
                          : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    painter->setPen(option.palette.color(cg, (option.state & QStyle::State_Selected)
                                         ? QPalette::HighlightedText
                                         : QPalette::Text));
    // draw top text
    {

        const QRect &top_text_rect = opt.topTextRect();
        painter->drawText(top_text_rect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                        Qt::ElideRight,
                                                        top_text_rect.width()));
    }

    // draw size label / draw progress info
    const QVariant &progress_data = index.data(models::AttachmentList::ProgressInfoRole);
    if (progress_data.isNull()) {
        const QRect &bottom_text_rect = opt.bottomTextRect();
        painter->drawText(bottom_text_rect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(QString("%1 bytes").arg(index.data(models::AttachmentList::SizeRole).toInt()),
                                                        Qt::ElideRight,
                                                        bottom_text_rect.width()));
    }
    else {
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


QSize AttachmentListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    AttachmentListItemOption opt(option, index);
    return QSize(0, (opt.decorationRect() | opt.topTextRect() | opt.bottomTextRect() | opt.checkRect()).height() + 8);
}


bool AttachmentListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_ASSERT (event);
    Q_ASSERT (model);

    AttachmentListItemOption msg_opt(option, index);

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
