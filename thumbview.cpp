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

#include <QApplication>
#include <QImageReader>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QUrl>
#include "thumbview.h"

ThumbView::ThumbView(QWidget *parent, int thumbSize) : QListView(parent)
{
	thumbHeight = thumbSize;
	thumbWidth = GData::thumbsCompactLayout? thumbHeight * GData::thumbAspect : thumbHeight;

	GData::thumbsBackgroundColor = GData::appSettings->value("backgroundThumbColor").value<QColor>();
	GData::thumbsTextColor = GData::appSettings->value("textThumbColor").value<QColor>();
	setThumbColors();
	GData::thumbsCompactLayout = GData::appSettings->value("showThumbNames").toBool();
	GData::thumbSpacing = GData::appSettings->value("thumbSpacing").toInt();

	setIconSize(QSize(thumbWidth, thumbHeight));
	setViewMode(QListView::IconMode);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setResizeMode(QListView::Adjust);
	setFrameShape(QFrame::NoFrame);
	setMovement(QListView::Movement(QListView::Static));
	setWordWrap(true);
	setDragEnabled(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);

	thumbViewModel = new QStandardItemModel(this);
	thumbViewModel->setSortRole(SortRole);
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

	emptyImg.load(":/images/no_image.png");
}

ThumbView::~ThumbView()
{

}

void ThumbView::setThumbColors()
{
	QPalette sbOrig = verticalScrollBar()->palette();
	QPalette tvOrig = palette();
	tvOrig.setColor(QPalette::Base, GData::thumbsBackgroundColor);
	tvOrig.setColor(QPalette::Text, GData::thumbsTextColor);
	setPalette(tvOrig);
	verticalScrollBar()->setPalette(sbOrig);
}

void ThumbView::setCurrentIndexByName(QString &FileName)
{
	QModelIndexList indexList = thumbViewModel->match(thumbViewModel->index(0, 0), FileNameRole, FileName);
	setCurrentIndex(indexList[0]);
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
    if (indexesList.isEmpty())
	{
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
	thumbWidth = GData::thumbsCompactLayout? thumbHeight * GData::thumbAspect : thumbHeight;
	setIconSize(QSize(thumbWidth, thumbHeight));
	thumbsDir->setPath(currentViewDir);
	thumbsDir->setSorting(thumbsSortFlags);
	
	abortOp = false;
	newIndex = 0;

	initThumbs();
	loadThumbs();

	if ((thumbViewModel->rowCount() - 1) == 0)
		emit updateState("No images");
	else 
	{
		QString state = (QString::number(thumbViewModel->rowCount() - 1) + " images");
		emit updateState(state);
	}
}

void ThumbView::initThumbs()
{
	thumbFileInfoList = thumbsDir->entryInfoList();
	QStandardItem *thumbIitem;
	int currThumb;

	setSpacing(GData::thumbSpacing);
	thumbViewModel->clear();
	thumbIsLoaded->clear();

	if (m_needScroll)
		scrollToTop();

	if (!GData::thumbsCompactLayout)
		setUniformItemSizes(true);
	else
		setUniformItemSizes(false);

	QPixmap emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbWidth, thumbHeight);

	for (currThumb = 0; currThumb < thumbFileInfoList.size(); currThumb++)
	{
		thumbFileInfo = thumbFileInfoList.at(currThumb);
		thumbIitem = new QStandardItem();
		thumbIitem->setData(currThumb, SortRole);
		thumbIitem->setData(thumbFileInfo.fileName(), FileNameRole);
		if (!GData::thumbsCompactLayout)
			thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);
		thumbIitem->setIcon(emptyPixMap);
//		thumbIitem->setTextAlignment(Qt::AlignTop);

		thumbViewModel->appendRow(thumbIitem);
		thumbIsLoaded->append(false);
	}

	// Dummy image
	thumbIitem = new QStandardItem("");
	thumbIitem->setIcon(emptyPixMap);
	thumbViewModel->appendRow(thumbIitem);
	thumbIsLoaded->append(true);
	setRowHidden(thumbViewModel->rowCount() - 1, true);
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
	for (int currThumb = 0; currThumb < thumbViewModel->rowCount() - 1; currThumb++)
	{
		if (thumbIsLoaded->at(currThumb))
			continue;
		
		thumbFileInfo = thumbFileInfoList.at(currThumb);

		thumbReader.setFileName(thumbFileInfo.filePath());
		thumbSize = thumbReader.size();

		if (!thumbSize.isValid())
			thumbReader.setFileName(":/images/error_image.png");

		
		
//			if (thumbSize.height() > thumbHeight || thumbSize.width() > thumbWidth)
	//		{
				thumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
	//		}
			thumbReader.setScaledSize(thumbSize);
			thumbViewModel->item(currThumb)->setIcon(QPixmap::fromImage(thumbReader.read()));

			int zz = thumbWidth /2 ;//- (thumbSize.width() / 2);
			QSize hintSize(zz, zz);
			thumbViewModel->item(currThumb)->setSizeHint(hintSize);
		 

		if (GData::thumbsCompactLayout)
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

void ThumbView::addNewThumb(QString &imageFileName)
{
	QStandardItem *thumbIitem;
	QImageReader thumbReader;
	QSize thumbSize;
	QPixmap emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbWidth, thumbHeight);
	QImage errorImg;
	errorImg.load(":/images/error_image.png");
	QPixmap errorPixMap = QPixmap::fromImage(errorImg);
	QFileInfo fInfo = QFileInfo(imageFileName);

	thumbIitem = new QStandardItem();
	thumbIitem->setIcon(emptyPixMap);
	thumbIitem->setData(666, SortRole);
	thumbIitem->setData(fInfo.fileName(), FileNameRole);
	if (!GData::thumbsCompactLayout)
		thumbIitem->setData(fInfo.fileName(), Qt::DisplayRole);

	thumbReader.setFileName(fInfo.filePath());
	thumbSize = thumbReader.size();

	if (thumbSize.isValid())
	{
		if (thumbSize.height() > thumbHeight || thumbSize.width() > thumbWidth)
		{
			thumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
		}
		thumbReader.setScaledSize(thumbSize);
		thumbIitem->setIcon(QPixmap::fromImage(thumbReader.read()));
	} 
	else 
		thumbIitem->setIcon(errorPixMap);

	thumbViewModel->insertRow(0, thumbIitem);

	if (GData::thumbsCompactLayout)
		setRowHidden(0, false);
}

void ThumbView::wheelEvent(QWheelEvent *event)
{
	if (event->delta() < 0)
		verticalScrollBar()->setValue(verticalScrollBar()->value() + thumbHeight);
	else
		verticalScrollBar()->setValue(verticalScrollBar()->value() - thumbHeight);
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

