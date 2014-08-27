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

#include "thumbview.h"
#include "preview.h"

#define BAD_IMG_SZ	64

ThumbView::ThumbView(QWidget *parent) : QListView(parent)
{
	GData::thumbsBackgroundColor = GData::appSettings->value("backgroundThumbColor").value<QColor>();
	GData::thumbsTextColor = GData::appSettings->value("textThumbColor").value<QColor>();
	setThumbColors();
	GData::thumbSpacing = GData::appSettings->value("thumbSpacing").toInt();
	GData::thumbPagesReadahead = GData::appSettings->value("thumbPagesReadahead").toInt();
	GData::thumbsLayout = GData::appSettings->value("thumbLayout").toInt();
	thumbSize = GData::appSettings->value("thumbsZoomVal").toInt();
	currentRow = 0;

	setViewMode(QListView::IconMode);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setResizeMode(QListView::Adjust);
	setWordWrap(true);
	setDragEnabled(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setItemDelegate(new QItemDelegate);
	setUniformItemSizes(false);

	thumbViewModel = new QStandardItemModel(this);
	thumbViewModel->setSortRole(SortRole);
	setModel(thumbViewModel);

	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(loadVisibleThumbs(int)));
	connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(handleSelectionChanged(QItemSelection)));
   	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), 
				parent, SLOT(loadImagefromThumb(const QModelIndex &)));

	thumbsDir = new QDir();
	fileFilters = new QStringList;

	emptyImg.load(":/images/no_image.png");

	QTime time = QTime::currentTime();
	qsrand((uint)time.msec());
	mainWindow = parent;
	infoView = new InfoView(this);
	imagePreview = new ImagePreview(this);
}

void ThumbView::setThumbColors()
{
	QPalette scrollBarOrigPal = verticalScrollBar()->palette();
	QPalette thumbViewOrigPal = palette();
	thumbViewOrigPal.setColor(QPalette::Base, GData::thumbsBackgroundColor);
	thumbViewOrigPal.setColor(QPalette::Text, GData::thumbsTextColor);
	setPalette(thumbViewOrigPal);
	verticalScrollBar()->setPalette(scrollBarOrigPal);
}

void ThumbView::selectCurrentIndex()
{
	if (currentIndex.isValid() && thumbViewModel->rowCount() > 0)
	{
		scrollTo(currentIndex);
		setCurrentIndex(currentIndex);
	}
}

QString ThumbView::getSingleSelectionFilename()
{
	if (selectionModel()->selectedIndexes().size() == 1)
		return thumbViewModel->item(selectionModel()->selectedIndexes().first().row())->data(FileNameRole).toString();

	return("");
}

int ThumbView::getNextRow()
{
	if (currentRow == thumbViewModel->rowCount() - 1)
		return -1;

	return currentRow + 1;
}

int ThumbView::getPrevRow()
{
	if (currentRow == 0)
		return -1;

	return currentRow - 1;
}

int ThumbView::getLastRow()
{
	return thumbViewModel->rowCount() - 1;
}

int ThumbView::getRandomRow()
{
	return qrand() % (thumbViewModel->rowCount());
}

int ThumbView::getCurrentRow()
{
	return currentRow;
}

void ThumbView::setCurrentRow(int row)
{
	if (row >= 0)
		currentRow = row;
	else
		currentRow = 0;
}

void ThumbView::setImageviewWindowTitle()
{
	QString title = thumbViewModel->item(currentRow)->data(FileNameRole).toString()
			+ " - ["
			+ QString::number(currentRow + 1)
			+ "/"
			+ QString::number(thumbViewModel->rowCount())
			+ "] - Phototonic";


	mainWindow->setWindowTitle(title);
}

bool ThumbView::setCurrentIndexByName(QString &FileName)
{
	QModelIndexList indexList = thumbViewModel->match(thumbViewModel->index(0, 0), FileNameRole, FileName);
	if (indexList.size())
	{
	 	currentIndex = indexList[0];
	 	setCurrentRow(currentIndex.row());
	 	return true;
 	}

 	return false;
}

void ThumbView::updateExifInfo(QString imageFullPath)
{
	Exiv2::Image::AutoPtr exifImage;
	QString key;
	QString val;

	try
	{
		exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
	}
	catch (Exiv2::Error &error)
	{
		return;
	}

	exifImage->readMetadata();
	Exiv2::ExifData &exifData = exifImage->exifData();

	if (!exifData.empty())
	{
		Exiv2::ExifData::const_iterator end = exifData.end();
		for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md)
		{
			// qDebug() << Exiv2::toString(md->key()).c_str() << " " << Exiv2::toString(md->value()).c_str();
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
			infoView->addEntry(key, val);
		}
	}

	Exiv2::IptcData &iptcData = exifImage->iptcData();
	if (!iptcData.empty())
	{
		Exiv2::IptcData::iterator end = iptcData.end();
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md)
		{
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
	   		infoView->addEntry(key, val);
		}
	}

	Exiv2::XmpData &xmpData = exifImage->xmpData();
	if (!xmpData.empty())
	{
		Exiv2::XmpData::iterator end = xmpData.end();
		for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md)
		{
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
			infoView->addEntry(key, val);
		}
	}
}

void ThumbView::handleSelectionChanged(const QItemSelection&)
{
	QString info;
	QString state;
	QModelIndexList indexesList = selectionModel()->selectedIndexes();
	int nSelected = indexesList.size();
	QString imageFullPath;

	infoView->clear();
	imagePreview->clear();

	if (!nSelected)
		state = QString::number(thumbViewModel->rowCount()) + tr(" images");
	else if (nSelected >= 1)
		state = QString(tr("Selected ") + QString::number(nSelected) + tr(" of ")
							+ QString::number(thumbViewModel->rowCount()) + tr(" images"));

	if (nSelected == 1)
	{
		QString imageFullPath = thumbViewModel->item(indexesList.first().row())->data(FileNameRole).toString();
		imageInfoReader.setFileName(imageFullPath);
		QString key;
		QString val;

		imagePreview->load(imageFullPath);
		
		if (imageInfoReader.size().isValid())
		{
			QFileInfo imageInfo = QFileInfo(imageFullPath);

			key = tr("File name");
			val = imageInfo.fileName();
			infoView->addEntry(key, val);

			key = tr("Location");
			val = imageInfo.path();
			infoView->addEntry(key, val);

			key = tr("Format");
			val = imageInfoReader.format().toUpper();
			infoView->addEntry(key, val);

			key = tr("Resolution");
			val = QString::number(imageInfoReader.size().width())
					+ " x "
					+ QString::number(imageInfoReader.size().height());
			infoView->addEntry(key, val);

			key = tr("Megapixel");
			val = QString::number((imageInfoReader.size().width() * imageInfoReader.size().height()) / 1000000.0, 'f', 2);
			infoView->addEntry(key, val);

			key = tr("Size");
			val = QString::number(imageInfo.size() / 1024.0, 'f', 2) + "K";
			infoView->addEntry(key, val);

			key = tr("Modified");
			val = imageInfo.lastModified().toString(Qt::SystemLocaleShortDate);
			infoView->addEntry(key, val);
			
			updateExifInfo(imageFullPath);
			recentThumb = imageFullPath;
		}
		else
		{
			imageInfoReader.read();
			key = imageInfoReader.errorString();
			val = "";
			infoView->addEntry(key, val);
		}
	}

	emit setStatus(state);
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

void ThumbView::loadVisibleThumbs(int scrollBarValue)
{
	static int lastScrollBarValue = 0;
	scrolledForward = (scrollBarValue >= lastScrollBarValue);
	lastScrollBarValue = scrollBarValue;

Start:

	int first = getFirstVisibleThumb();
	int last = getLastVisibleThumb();
	if (abortOp || first < 0 || last < 0) 
		return;

	if (scrolledForward)
	{
		last += ((last - first) * (GData::thumbPagesReadahead + 1));
		if (last >= thumbViewModel->rowCount())
			last = thumbViewModel->rowCount() - 1;
	}
	else
	{
		first -= (last - first) * (GData::thumbPagesReadahead + 1);
		if (first < 0)
			first = 0;

		last += 10;
		if (last >= thumbViewModel->rowCount())
			last = thumbViewModel->rowCount() - 1;
	}

	if (thumbsRangeFirst == first && thumbsRangeLast == last)
		return;

	thumbsRangeFirst = first;
	thumbsRangeLast = last;

	loadThumbsRange();
	if (!abortOp)
		goto Start;
}

int ThumbView::getFirstVisibleThumb()
{
	QModelIndex idx;

	for (int currThumb = 0; currThumb < thumbViewModel->rowCount(); ++currThumb)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
		{
			return idx.row();
		}
	}

	return -1;
}

int ThumbView::getLastVisibleThumb()
{
	QModelIndex idx;

	for (int currThumb = thumbViewModel->rowCount() -1; currThumb >= 0 ; --currThumb)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
		{
			return idx.row();
		}
	}

	return -1;
}

bool ThumbView::isThumbVisible(QModelIndex idx)
{
	if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
	{
		return true;
	}

	return false;
}

void ThumbView::updateThumbsCount()
{
	if (thumbViewModel->rowCount() > 0)
	{
		QString state = (QString::number(thumbViewModel->rowCount()) + tr(" images"));
		emit setStatus(state);
	}
	else
		emit setStatus(tr("No images"));
}

void ThumbView::load()
{
	float thumbAspect = 1.33;
	if (GData::thumbsLayout == Compact)
		thumbAspect = 1.77;
	else if (GData::thumbsLayout == Squares)
		thumbAspect = 2;

	thumbHeight = (GData::thumbsLayout == Squares)? thumbSize * thumbAspect : thumbSize;
	thumbWidth = (GData::thumbsLayout == Squares)? thumbSize * thumbAspect : thumbHeight * thumbAspect;
	setIconSize(QSize(thumbWidth, thumbHeight));

	fileFilters->clear();
	QString textFilter("*");
	textFilter+= filterStr;
	*fileFilters	<< textFilter + "*.BMP"
					<< textFilter + "*.GIF"
					<< textFilter + "*.ICO"
					<< textFilter + "*.JPEG"
					<< textFilter + "*.JPG"
					<< textFilter + "*.MNG"
					<< textFilter + "*.PBM"
					<< textFilter + "*.PGM"
					<< textFilter + "*.PNG"
					<< textFilter + "*.PPM"
					<< textFilter + "*.SVG"
					<< textFilter + "*.SVGZ"
					<< textFilter + "*.TGA"					
					<< textFilter + "*.TIF"
					<< textFilter + "*.TIFF"
					<< textFilter + "*.WBMP"
					<< textFilter + "*.XBM"
					<< textFilter + "*.XPM"
					<< textFilter + "*.JPE";
	thumbsDir->setNameFilters(*fileFilters);
	thumbsDir->setFilter(QDir::Files);
	if (GData::showHiddenFiles)
		thumbsDir->setFilter(thumbsDir->filter() | QDir::Hidden);
	
	thumbsDir->setPath(currentViewDir);
	thumbsDir->setSorting(thumbsSortFlags);
	thumbViewModel->clear();

	setSpacing(GData::thumbSpacing);

	if (isNeedScroll)
		scrollToTop();

	abortOp = false;
	newIndex = 0;

	thumbsRangeFirst = -1;
	thumbsRangeLast = -1;

	initThumbs();

	updateThumbsCount();
	loadVisibleThumbs();

	if (GData::includeSubFolders)
	{
		QDirIterator iterator(currentViewDir, QDirIterator::Subdirectories);
		while (iterator.hasNext())
		{
			iterator.next();
			if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..")
			{
				thumbsDir->setPath(iterator.filePath());
				initThumbs();
				updateThumbsCount();
				loadVisibleThumbs();

				if (abortOp)
				{
					goto finish;
				}
			}
			QApplication::processEvents();
		}
	}

finish:
	thumbLoaderActive = false;
	emit unsetBusy();
	return;
}

void ThumbView::initThumbs()
{
	thumbFileInfoList = thumbsDir->entryInfoList();
	static QStandardItem *thumbIitem;
	static int currThumb;
	static QPixmap emptyPixMap;
	static QSize hintSize;

	emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbWidth, thumbHeight);

	if (GData::thumbsLayout == Squares)
		hintSize = QSize(thumbWidth / 2, thumbWidth / 2);
	else if (GData::thumbsLayout == Classic)
		hintSize = QSize(thumbWidth, thumbHeight + 
							(GData::showLabels? QFontMetrics(font()).height() + 5 : 0));

	for (currThumb = 0; currThumb < thumbFileInfoList.size(); ++currThumb)
	{
		thumbFileInfo = thumbFileInfoList.at(currThumb);
		thumbIitem = new QStandardItem();
		thumbIitem->setData(false, LoadedRole);
		thumbIitem->setData(currThumb, SortRole);
		thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
		if (GData::thumbsLayout != Squares && GData::showLabels)
			thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);
		if (GData::thumbsLayout == Compact)
			thumbIitem->setIcon(emptyPixMap);
		thumbIitem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
		if (GData::thumbsLayout != Compact)
			thumbIitem->setSizeHint(hintSize);

		thumbViewModel->appendRow(thumbIitem);
	}
}

void ThumbView::loadThumbsRange()
{
	static bool inProgress = false;
	static QImageReader thumbReader;
	static QSize currThumbSize;
	static int currRowCount;
	static QString imageFileName;
	static QImage thumb;
	int currThumb;

	if (inProgress)
	{	
		abortOp = true;
		QTimer::singleShot(0, this, SLOT(loadThumbsRange()));
		return;
	}

	inProgress = true;
	currRowCount = thumbViewModel->rowCount();

	for (	scrolledForward? currThumb = thumbsRangeFirst : currThumb = thumbsRangeLast;
			(scrolledForward? currThumb <= thumbsRangeLast : currThumb >= thumbsRangeFirst);
			scrolledForward? ++currThumb : --currThumb)
	{
		if (abortOp || thumbViewModel->rowCount() != currRowCount)
			break;

		if (thumbViewModel->item(currThumb)->data(LoadedRole).toBool())
			continue;

		imageFileName = thumbViewModel->item(currThumb)->data(FileNameRole).toString();
		thumbReader.setFileName(imageFileName);
		currThumbSize = thumbReader.size();

		if (currThumbSize.isValid())
		{
			if (!GData::noEnlargeSmallThumb
				|| (currThumbSize.width() > thumbWidth || currThumbSize.height() > thumbHeight))
			{
				currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
			}

			thumbReader.setScaledSize(currThumbSize);
			thumb = thumbReader.read();

			if (GData::exifThumbRotationEnabled)
			{
				ImageView::rotateByExifRotation(thumb, imageFileName);
				currThumbSize = thumb.size();
				currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
			}
				
			thumbViewModel->item(currThumb)->setIcon(QPixmap::fromImage(thumb));
		}
		else
		{
			thumbViewModel->item(currThumb)->setIcon(QIcon::fromTheme("image-missing",
									QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
			currThumbSize.setHeight(BAD_IMG_SZ);
			currThumbSize.setWidth(BAD_IMG_SZ);
		}

		if (GData::thumbsLayout == Compact)
		{
			if (GData::showLabels)
				currThumbSize.setHeight(currThumbSize.height() + QFontMetrics(font()).height() + 5);
			thumbViewModel->item(currThumb)->setSizeHint(currThumbSize);
			if (isThumbVisible(thumbViewModel->item(currThumb)->index()))
				setRowHidden(currThumb, false);
		}

		thumbViewModel->item(currThumb)->setData(true, LoadedRole);

		QApplication::processEvents();
	}

	if (GData::thumbsLayout == Compact && thumbViewModel->rowCount() > 0)
	{
		setRowHidden(0 , false);
	}

	inProgress = false;
	abortOp = false;
}

void ThumbView::addThumb(QString &imageFullPath)
{
	QStandardItem *thumbIitem = new QStandardItem();
	QImageReader thumbReader;
	QSize hintSize;
	QSize currThumbSize;
	static QImage thumb;

	if (GData::thumbsLayout == Squares)
		hintSize = QSize(thumbWidth / 2, thumbWidth / 2);
	else if (GData::thumbsLayout == Classic)
		hintSize = QSize(thumbWidth, thumbHeight + 
							(GData::showLabels? QFontMetrics(font()).height() + 5 : 0));
	
	thumbFileInfo = QFileInfo(imageFullPath);
	thumbIitem->setData(true, LoadedRole);
	thumbIitem->setData(0, SortRole);
	thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
	if (GData::thumbsLayout != Squares && GData::showLabels)
		thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);

	thumbReader.setFileName(imageFullPath);
	currThumbSize = thumbReader.size();
	if (currThumbSize.isValid())
	{
		if (!GData::noEnlargeSmallThumb
			|| (currThumbSize.width() > thumbWidth || currThumbSize.height() > thumbHeight))
		{
			currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
		}
			
		thumbReader.setScaledSize(currThumbSize);
		thumb = thumbReader.read();

		if (GData::exifThumbRotationEnabled)
		{
			ImageView::rotateByExifRotation(thumb, imageFullPath);
			currThumbSize = thumb.size();
			currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
		}
			
		thumbIitem->setIcon(QPixmap::fromImage(thumb));
	}
	else
	{
		thumbIitem->setIcon(QIcon::fromTheme("image-missing",
								QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
		currThumbSize.setHeight(BAD_IMG_SZ);
		currThumbSize.setWidth(BAD_IMG_SZ);
	}

	if (GData::thumbsLayout == Compact)
	{
		if (GData::showLabels)
			currThumbSize.setHeight(currThumbSize.height() + QFontMetrics(font()).height() + 5);
		thumbIitem->setSizeHint(currThumbSize);
	}
	else
		thumbIitem->setSizeHint(hintSize);

	thumbViewModel->appendRow(thumbIitem);
}

void ThumbView::wheelEvent(QWheelEvent *event)
{
	if (event->delta() < 0)
		verticalScrollBar()->setValue(verticalScrollBar()->value() + thumbHeight);
	else
		verticalScrollBar()->setValue(verticalScrollBar()->value() - thumbHeight);
}

void ThumbView::mousePressEvent(QMouseEvent *event)
{
	QListView::mousePressEvent(event);

	if (GData::reverseMouseBehavior && event->button() == Qt::MiddleButton)
	{
		if (selectionModel()->selectedIndexes().size() == 1)
			emit(doubleClicked(selectionModel()->selectedIndexes().first()));
	}
}

void ThumbView::invertSelection()
{
	QModelIndex idx;

	for (int currThumb = 0; currThumb < thumbViewModel->rowCount(); ++currThumb)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		selectionModel()->select(idx, QItemSelectionModel::Toggle);
	}
}

FSTree::FSTree(QWidget *parent) : QTreeView(parent)
{
	setAcceptDrops(true);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::InternalMove);

	connect(this, SIGNAL(expanded(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));
	connect(this, SIGNAL(collapsed(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));
}

QModelIndex FSTree::getCurrentIndex()
{
	return selectedIndexes().first();
}

void FSTree::resizeTreeColumn(const QModelIndex &)
{
	resizeColumnToContents(0);
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
	if (event->source())
	{
		QString fstreeStr="FSTree";
		bool dirOp = (event->source()->metaObject()->className() == fstreeStr);
		emit dropOp(event->keyboardModifiers(), dirOp, event->mimeData()->urls().at(0).toLocalFile());
		setCurrentIndex(dndOrigSelection);
	}
}

