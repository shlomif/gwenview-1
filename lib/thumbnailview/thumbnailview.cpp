/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "thumbnailview.moc"

// Std
#include <math.h>

// Qt
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QTimer>

// KDE
#include <kdebug.h>
#include <kdirmodel.h>
#include <kglobalsettings.h>

// Local
#include "archiveutils.h"
#include "abstractthumbnailviewhelper.h"

namespace Gwenview {

/** How many pixels between items */
const int SPACING = 11;


static KFileItem fileItemForIndex(const QModelIndex& index) {
	Q_ASSERT(index.isValid());
	QVariant data = index.data(KDirModel::FileItemRole);
	return qvariant_cast<KFileItem>(data);
}


static KUrl urlForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	return item.url();
}


struct ThumbnailViewPrivate {
	int mThumbnailSize;
	AbstractThumbnailViewHelper* mThumbnailViewHelper;
	QMap<QUrl, QPixmap> mThumbnailForUrl;
	QMap<QUrl, QPersistentModelIndex> mPersistentIndexForUrl;
	QTimer mScheduledThumbnailGenerationTimer;
	QPixmap mWaitingThumbnail;

	void scheduleThumbnailGenerationForVisibleItems() {
		if (!mThumbnailViewHelper) {
			// Not initialized yet
			return;
		}
		mThumbnailViewHelper->abortThumbnailGeneration();
		mScheduledThumbnailGenerationTimer.start();
	}
};


ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate) {
	d->mThumbnailViewHelper = 0;

	setFrameShape(QFrame::NoFrame);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setSpacing(SPACING);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);

	viewport()->setMouseTracking(true);
	// Set this attribute, otherwise the item delegate won't get the
	// State_MouseOver state
	viewport()->setAttribute(Qt::WA_Hover);

	setVerticalScrollMode(ScrollPerPixel);
	setHorizontalScrollMode(ScrollPerPixel);

	// Make sure mThumbnailSize is initialized before calling setThumbnailSize,
	// since it will compare the new size with the old one
	d->mThumbnailSize = 0;
	setThumbnailSize(128);

	d->mScheduledThumbnailGenerationTimer.setSingleShot(true);
	d->mScheduledThumbnailGenerationTimer.setInterval(500);
	connect(&d->mScheduledThumbnailGenerationTimer, SIGNAL(timeout()),
		SLOT(generateThumbnailsForVisibleItems()) );

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );

	if (KGlobalSettings::singleClick()) {
		connect(this, SIGNAL(clicked(const QModelIndex&)),
			SLOT(emitIndexActivatedIfNoModifiers(const QModelIndex&)) );
	} else {
		connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
			SLOT(emitIndexActivatedIfNoModifiers(const QModelIndex&)) );
	}
}


ThumbnailView::~ThumbnailView() {
	delete d;
}


void ThumbnailView::setThumbnailSize(int value) {
	if (d->mThumbnailSize == value) {
		return;
	}
	d->mThumbnailSize = value;

	// mWaitingThumbnail
	int waitingThumbnailSize;
	if (value > 64) {
		waitingThumbnailSize = 48;
	} else {
		waitingThumbnailSize = 32;
	}
	if (d->mWaitingThumbnail.width() != waitingThumbnailSize) {
		QPixmap icon = DesktopIcon("chronometer", waitingThumbnailSize);
		QPixmap pix(icon.size());
		pix.fill(Qt::transparent);
		QPainter painter(&pix);
		painter.setOpacity(0.5);
		painter.drawPixmap(0, 0, icon);
		painter.end();
		d->mWaitingThumbnail = pix;
	}

	thumbnailSizeChanged(value);
	setSpacing(SPACING);
	d->scheduleThumbnailGenerationForVisibleItems();
}


int ThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}


void ThumbnailView::setThumbnailViewHelper(AbstractThumbnailViewHelper* helper) {
	d->mThumbnailViewHelper = helper;
	connect(helper, SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&)),
		SLOT(setThumbnail(const KFileItem&, const QPixmap&)) );
}

AbstractThumbnailViewHelper* ThumbnailView::thumbnailViewHelper() const {
	return d->mThumbnailViewHelper;
}


void ThumbnailView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	QListView::rowsAboutToBeRemoved(parent, start, end);

	KFileItemList itemList;
	for (int pos=start; pos<=end; ++pos) {
		QModelIndex index = model()->index(pos, 0, parent);
		if (!index.isValid()) {
			kDebug() << "Skipping invalid index" << pos;
			continue;
		}
		KFileItem item = fileItemForIndex(index);
		if (item.isNull()) {
			kDebug() << "Skipping invalid item!" << index.data().toString();
			continue;
		}

		QUrl url = item.url();
		d->mThumbnailForUrl.remove(url);
		d->mPersistentIndexForUrl.remove(url);

		itemList.append(item);
	}

	Q_ASSERT(d->mThumbnailViewHelper);
	d->mThumbnailViewHelper->abortThumbnailGenerationForItems(itemList);
}


void ThumbnailView::rowsInserted(const QModelIndex& parent, int start, int end) {
	QListView::rowsInserted(parent, start, end);
	d->mScheduledThumbnailGenerationTimer.start();
}


void ThumbnailView::showContextMenu() {
	d->mThumbnailViewHelper->showContextMenu(this);
}


void ThumbnailView::emitIndexActivatedIfNoModifiers(const QModelIndex& index) {
	if (QApplication::keyboardModifiers() == Qt::NoModifier) {
		emit indexActivated(index);
	}
}


void ThumbnailView::setThumbnail(const KFileItem& item, const QPixmap& pixmap) {
	QUrl url = item.url();
	QPersistentModelIndex persistentIndex = d->mPersistentIndexForUrl[url];
	if (!persistentIndex.isValid()) {
		return;
	}

	// Alpha check

	d->mThumbnailForUrl[url] = pixmap;

	QRect rect = visualRect(persistentIndex);
	update(rect);
	viewport()->update(rect);
}


QPixmap ThumbnailView::thumbnailForIndex(const QModelIndex& index) {
	QVariant data = index.data(KDirModel::FileItemRole);
	KFileItem item = qvariant_cast<KFileItem>(data);
	QUrl url = item.url();

	QPixmap pix;

	QMap<QUrl, QPixmap>::ConstIterator it = d->mThumbnailForUrl.find(url);
	if (it != d->mThumbnailForUrl.constEnd()) {
		pix = it.value();
	} else if (ArchiveUtils::fileItemIsDirOrArchive(item)) {
		pix = item.pixmap(128);
		d->mThumbnailForUrl[url] = pix;
	} else {
		pix = d->mWaitingThumbnail;
	}

	int maxSize = qMax(pix.width(), pix.height());
	if (maxSize > d->mThumbnailSize) {
		return pix.scaled(d->mThumbnailSize, d->mThumbnailSize, Qt::KeepAspectRatio);
	} else {
		return pix;
	}
}


bool ThumbnailView::isModified(const QModelIndex& index) const {
	KUrl url = urlForIndex(index);
	return d->mThumbnailViewHelper->isDocumentModified(url);
}


void ThumbnailView::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}


void ThumbnailView::dragMoveEvent(QDragMoveEvent* event) {
	// Necessary, otherwise we don't reach dropEvent()
	event->acceptProposedAction();
}


void ThumbnailView::dropEvent(QDropEvent* event) {
	const KUrl::List urlList = KUrl::List::fromMimeData(event->mimeData());
	if (urlList.isEmpty()) {
		return;
	}

	QModelIndex destIndex = indexAt(event->pos());
	if (destIndex.isValid()) {
		KFileItem item = fileItemForIndex(destIndex);
		if (item.isDir()) {
			KUrl destUrl = item.url();
			d->mThumbnailViewHelper->showMenuForUrlDroppedOnDir(this, urlList, destUrl);
			return;
		}
	}

	d->mThumbnailViewHelper->showMenuForUrlDroppedOnViewport(this, urlList);

	event->acceptProposedAction();
}


void ThumbnailView::keyPressEvent(QKeyEvent* event) {
	QListView::keyPressEvent(event);
	if (event->key() == Qt::Key_Return) {
		const QModelIndex index = selectionModel()->currentIndex();
		if (index.isValid() && selectionModel()->selectedIndexes().count() == 1) {
			emit indexActivated(index);
		}
	}
}


void ThumbnailView::resizeEvent(QResizeEvent* event) {
	QListView::resizeEvent(event);
	d->scheduleThumbnailGenerationForVisibleItems();
}


void ThumbnailView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
	QListView::selectionChanged(selected, deselected);
	emit selectionChangedSignal(selected, deselected);
}


void ThumbnailView::scrollContentsBy(int dx, int dy) {
	QListView::scrollContentsBy(dx, dy);
	d->scheduleThumbnailGenerationForVisibleItems();
}


void ThumbnailView::generateThumbnailsForVisibleItems() {
	KFileItemList list;
	QRect viewportRect = viewport()->rect();
	for (int row=0; row < model()->rowCount(); ++row) {
		// Filter out invisible items
		QModelIndex index = model()->index(row, 0);
		QRect rect = visualRect(index);
		if (!viewportRect.intersects(rect)) {
			continue;
		}

		// Filter out items which already have a thumbnail
		KFileItem item = fileItemForIndex(index);
		QUrl url = item.url();
		if (d->mThumbnailForUrl.contains(url)) {
			continue;
		}

		// Add the item to our list and to mPersistentIndexForUrl, so that
		// setThumbnail() can find the item to update
		list << item;
		d->mPersistentIndexForUrl[url] = QPersistentModelIndex(index);
	}

	generateThumbnailsForItems(list);
}


void ThumbnailView::generateThumbnailsForItems(const KFileItemList& list) {
	d->mThumbnailViewHelper->generateThumbnailsForItems(list, ThumbnailSize::Large);
}


} // namespace
