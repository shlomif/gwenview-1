// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include <tagitemdelegate.h>

// Qt
#include <QAbstractItemView>
#include <QPainter>
#include <QToolButton>

// KDE
#include <KDebug>
#include <KDialog>
#include <KIconLoader>
#include <KLocale>

// Local
#include <lib/semanticinfo/tagmodel.h>

namespace Gwenview
{

TagItemDelegate::TagItemDelegate(QAbstractItemView* view)
: KWidgetItemDelegate(view, view)
{
#define pm(x) view->style()->pixelMetric(QStyle::x)
    mMargin     = pm(PM_ToolBarItemMargin);
    mSpacing    = pm(PM_ToolBarItemSpacing);
#undef pm
    const int iconSize = KIconLoader::global()->currentSize(KIconLoader::Toolbar);
    const QSize sz = view->style()->sizeFromContents(QStyle::CT_ToolButton, 0, QSize(iconSize, iconSize));
    mButtonSize = qMax(sz.width(), sz.height());
}

QList<QWidget*> TagItemDelegate::createItemWidgets() const
{

#define initButton(x) \
    (x)->setAutoRaise(true); \
    setBlockedEventTypes((x), QList<QEvent::Type>() \
                         << QEvent::MouseButtonPress \
                         << QEvent::MouseButtonRelease \
                         << QEvent::MouseButtonDblClick);

    QToolButton* assignToAllButton = new QToolButton;
    initButton(assignToAllButton);
    assignToAllButton->setIcon(QIcon::fromTheme("fill-color")); /* FIXME: Probably not the appropriate icon */
    assignToAllButton->setToolTip(i18nc("@info:tooltip", "Assign this tag to all selected images"));
    connect(assignToAllButton, SIGNAL(clicked()), SLOT(slotAssignToAllButtonClicked()));

    QToolButton* removeButton = new QToolButton;
    initButton(removeButton);
    removeButton->setIcon(QIcon::fromTheme("list-remove"));
    connect(removeButton, SIGNAL(clicked()), SLOT(slotRemoveButtonClicked()));

#undef initButton

    return QList<QWidget*>() << removeButton << assignToAllButton;
}

void TagItemDelegate::updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem& option, const QPersistentModelIndex& index) const
{
    const bool fullyAssigned = index.data(TagModel::AssignmentStatusRole).toInt() == int(TagModel::FullyAssigned);

    QToolButton* removeButton = static_cast<QToolButton*>(widgets[0]);
    QToolButton* assignToAllButton = static_cast<QToolButton*>(widgets[1]);

    QSize buttonSize(mButtonSize, option.rect.height() - 2 * mMargin);

    removeButton->resize(buttonSize);
    assignToAllButton->resize(buttonSize);

    removeButton->move(option.rect.width() - mButtonSize - mMargin, mMargin);

    if (fullyAssigned) {
        assignToAllButton->hide();
    } else {
        assignToAllButton->move(removeButton->x() - mButtonSize - mSpacing, mMargin);
    }
}

void TagItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }
    const bool selected = option.state & QStyle::State_Selected;
    const bool fullyAssigned = index.data(TagModel::AssignmentStatusRole).toInt() == int(TagModel::FullyAssigned);

    itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    QRect textRect = option.rect;
    textRect.setLeft(textRect.left() + mMargin);
    textRect.setWidth(textRect.width() - mButtonSize - mMargin - mSpacing);
    if (!fullyAssigned) {
        textRect.setWidth(textRect.width() - mButtonSize - mSpacing);
    }

    painter->setPen(option.palette.color(QPalette::Normal,
                                         selected
                                         ? QPalette::HighlightedText
                                         : QPalette::Text));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());
}

QSize TagItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int width  = option.fontMetrics.width(index.data().toString());
    const int height = qMax(mButtonSize, option.fontMetrics.height());
    return QSize(width + 2 * mMargin, height + 2 * mMargin);
}

void TagItemDelegate::slotRemoveButtonClicked()
{
    const QModelIndex index = focusedIndex();
    if (!index.isValid()) {
        kWarning() << "!index.isValid()";
        return;
    }
    emit removeTagRequested(index.data(TagModel::TagRole).toString());
}

void TagItemDelegate::slotAssignToAllButtonClicked()
{
    const QModelIndex index = focusedIndex();
    if (!index.isValid()) {
        kWarning() << "!index.isValid()";
        return;
    }
    emit assignTagToAllRequested(index.data(TagModel::TagRole).toString());
}

} // namespace
