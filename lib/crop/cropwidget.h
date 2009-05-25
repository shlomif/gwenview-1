// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef CROPWIDGET_H
#define CROPWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KDE

// Local
#include <lib/document/document.h>

namespace Gwenview {

class AbstractImageOperation;
class CropTool;
class ImageView;

class CropWidgetPrivate;
class GWENVIEWLIB_EXPORT CropWidget : public QWidget {
	Q_OBJECT
public:
	CropWidget(QWidget* parent, ImageView*, CropTool*);
	~CropWidget();

Q_SIGNALS:
	void cropRequested();

private Q_SLOTS:
	void slotPositionChanged();
	void slotWidthChanged();
	void slotHeightChanged();
	void setCropRect(const QRect& rect);

	void slotRatioComboBoxEditTextChanged();
	void slotRatioComboBoxActivated();
	void applyRatioConstraint();

private:
	CropWidgetPrivate* const d;
};


} // namespace

#endif /* CROPWIDGET_H */