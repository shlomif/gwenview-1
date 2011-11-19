// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef BIRDEYEVIEW_H
#define BIRDEYEVIEW_H

// Local

// KDE

// Qt
#include <QGraphicsWidget>

namespace Gwenview
{

class DocumentView;

class BirdEyeViewPrivate;
/**
 * Shows a bird-eye view of the current document. Makes it possible to scroll
 * through the document.
 */
class BirdEyeView : public QGraphicsWidget
{
    Q_OBJECT
public:
    BirdEyeView(DocumentView* docView);
    ~BirdEyeView();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0); // reimp

public Q_SLOTS:
    void adjustGeometry();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private Q_SLOTS:
    void adjustVisibleRect();

private:
    BirdEyeViewPrivate* const d;
};

} // namespace

#endif /* BIRDEYEVIEW_H */