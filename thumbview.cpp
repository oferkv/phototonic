/*
 *  Copyright (C) 2013 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QStandardItemModel>
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QImageReader>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QTreeView>
#include <QUrl>
#include "thumbview.h"

ThumbView::ThumbView(QWidget *parent, int thumbSize) : QListView(parent)
{
	thumbHeight = thumbSize;
	if (!thumbHeight)
		thumbHeight = 200;
	thumbWidth = thumbHeight * GData::thumbAspect;

	setIconSize(QSize(thumbWidth, thumbHeight));
	setViewMode(QListView::IconMode);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setResizeMode(QListView::Adjust);
	setFrameShape(QFrame::NoFrame);
	setMovement(QListView::Movement(QListView::Static));
	setWordWrap(true);
	setDragEnabled(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);

/*	Alternate layout:
	setUniformItemSizes(true);
	setSpacing(0);
	setWrapping(true);
	setFlow(QListView::TopToBottom);
	setViewMode(QListView::ListMode);	*/


	thumbViewModel = new QStandardItemModel(this);
	thumbIsLoaded = new QList<bool>();
	setModel(thumbViewModel);

	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateIndex()));

	connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
      this, SLOT(handleSelectionChanged(QItemSelection)));
   	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), 
		parent, SLOT(loadImagefromThumb(const QModelIndex &)));


	thumbsDir = new QDir();
	QStringList *fileFilters = new QStringList;
	*fileFilters << "*.BMP" << "*.GIF" << "*.JPG" << "*.JPEG" << "*.JPE" << "*.PNG"
				<< "*.PBM" << "*.PGM" << "*.PPM" << "*.XBM" << "*.XPM";
	thumbsDir->setFilter(QDir::Files);
	thumbsDir->setNameFilters(*fileFilters);

}

ThumbView::~ThumbView()
{

}

void ThumbView::handleSelectionChanged(const QItemSelection&)
{
	QString state;
	int nSelected = selectionModel()->selectedIndexes().size();
	
	if (nSelected)
		state = QString("Selected " + QString::number(nSelected) + " images");
	else
		state = "None selected";
	
	emit updateState(state);
}

void ThumbView::startDrag(Qt::DropActions)
{
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    if (indexesList.isEmpty()) {
        return;
    }

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	drag->setMimeData(mimeData);
	drag->start(Qt::CopyAction | Qt::MoveAction);
}

void ThumbView::abort()
{
	abortOp = true;
}

void ThumbView::updateIndex()
{
	if (thumbLoaderActive)
		newIndex = getFirstVisibleItem();
}

int ThumbView::getFirstVisibleItem()
{
	QModelIndex idx;

	for (int currThumb = 0; currThumb < thumbViewModel->rowCount() - 1; currThumb++)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
		{
			return idx.row();
		}
	}

	return -1;
}

bool ThumbView::isItemVisible(QModelIndex idx)
{
	if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
	{
		return true;
	}

	return false;
}

void ThumbView::load()
{
	setIconSize(QSize(thumbWidth, thumbHeight));
	thumbsDir->setPath(currentViewDir);
	thumbsDir->setSorting(thumbsSortFlags);
	
	abortOp = false;
	newIndex = 0;

	initThumbs();
	loadThumbs();
}

void ThumbView::initThumbs()
{
	thumbFileInfoList = thumbsDir->entryInfoList();
	QStandardItem *thumbIitem;

	thumbViewModel->clear();
	thumbIsLoaded->clear();
	if (m_needScroll)
	{
		scrollToTop();
	}

	QImage emptyImg;
	emptyImg.load(":/images/no_image.png");
	QPixmap emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbWidth, thumbHeight);

	int currThumb;
	for (currThumb = 0; currThumb < thumbFileInfoList.size(); currThumb++)
	{
		thumbFileInfo = thumbFileInfoList.at(currThumb);
		if (GData::showThumbnailNames)
			thumbIitem = new QStandardItem(thumbFileInfo.fileName());
		else
			thumbIitem = new QStandardItem();
		thumbIitem->setIcon(emptyPixMap);
		thumbViewModel->appendRow(thumbIitem);
		thumbIsLoaded->append(false);
	}

	if ((thumbViewModel->rowCount()) == 0)
		emit updateState("No images");
	else 
	{
		QString state = (QString::number(thumbViewModel->rowCount()) + " images");
		emit updateState(state);
	}
}

void ThumbView::loadThumbs()
{
	QImageReader thumbReader;
	QSize thumbSize;
	bool needRefresh = false;
	thumbLoaderActive = true;

	QImage errorImg;
	errorImg.load(":/images/error_image.png");
	QPixmap errorPixMap = QPixmap::fromImage(errorImg);

refreshThumbs:
	for (int currThumb = 0; currThumb < thumbViewModel->rowCount(); currThumb++)
	{
		if (thumbIsLoaded->at(currThumb))
			continue;
		
		thumbFileInfo = thumbFileInfoList.at(currThumb);

		thumbReader.setFileName(thumbFileInfo.filePath());
		thumbSize = thumbReader.size();

		if (thumbSize.isValid())
		{
			if (thumbSize.height() > thumbHeight || thumbSize.width() > thumbWidth)
			{
				thumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
			}
			thumbReader.setScaledSize(thumbSize);
			thumbViewModel->item(currThumb)->setIcon(QPixmap::fromImage(thumbReader.read()));
		} 
		else 
			thumbViewModel->item(currThumb)->setIcon(errorPixMap);

		setRowHidden(currThumb , false);
		(*thumbIsLoaded)[currThumb] = true;

		QApplication::processEvents();

		if (newIndex)
		{
			currThumb = newIndex - 1;
			newIndex = 0;
			needRefresh = true;
		}
		
		if (abortOp)
			break;
	}

	if (needRefresh)
	{
		needRefresh = false;
		goto refreshThumbs;
	}
	
	thumbLoaderActive = false;
	emit unsetBusy();
}

FSTree::FSTree(QWidget *parent) : QTreeView(parent)
{
	setAcceptDrops(true);
}

FSTree::~FSTree()
{

}

void FSTree::dragEnterEvent(QDragEnterEvent *event)
{
	QModelIndexList selectedDirs = selectionModel()->selectedRows();
	dndOrigSelection = selectedDirs[0];
	event->acceptProposedAction();
}

void FSTree::dragMoveEvent(QDragMoveEvent *event)
{
	setCurrentIndex(indexAt(event->pos()));
}

void FSTree::dropEvent(QDropEvent *event)
{
	QString fstreeStr="FSTree";
	bool dirOp = (event->source()->metaObject()->className() == fstreeStr);
	emit dropOp(event->keyboardModifiers(), dirOp, event->mimeData()->urls().at(0).toLocalFile());
	setCurrentIndex(dndOrigSelection);
}

