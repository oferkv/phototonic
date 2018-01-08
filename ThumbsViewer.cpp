/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
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

#include "ThumbsViewer.h"

ThumbsViewer::ThumbsViewer(QWidget *parent, MetadataCache *metadataCache) : QListView(parent) {
    this->metadataCache = metadataCache;
    Settings::thumbsBackgroundColor = Settings::appSettings->value("backgroundThumbColor").value<QColor>();
    Settings::thumbsTextColor = Settings::appSettings->value("textThumbColor").value<QColor>();
    setThumbColors();
    Settings::thumbPagesReadahead = Settings::appSettings->value("thumbPagesReadahead").toInt();
    thumbSize = Settings::appSettings->value(Settings::optionThumbsZoomLevel).toInt();
    currentRow = 0;

    setViewMode(QListView::IconMode);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformItemSizes(false);

    thumbsViewerModel = new QStandardItemModel(this);
    thumbsViewerModel->setSortRole(SortRole);
    setModel(thumbsViewerModel);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(loadVisibleThumbs(int)));
    connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection)));
    connect(this, SIGNAL(doubleClicked(
                                 const QModelIndex &)), parent, SLOT(loadImagefromThumb(
                                                                             const QModelIndex &)));

    thumbsDir = new QDir();
    fileFilters = new QStringList;

    emptyImg.load(":/images/no_image.png");

    QTime time = QTime::currentTime();
    qsrand((uint) time.msec());
    mainWindow = parent;
    infoView = new InfoView(this);
    connect(infoView, SIGNAL(updateInfo(QItemSelection)), this, SLOT(handleSelectionChanged(QItemSelection)));
}

void ThumbsViewer::setThumbColors() {
    QString bgColor = "background: rgb(%1, %2, %3); ";
    bgColor = bgColor.arg(Settings::thumbsBackgroundColor.red())
            .arg(Settings::thumbsBackgroundColor.green())
            .arg(Settings::thumbsBackgroundColor.blue());

    QString ss = "QListView { " + bgColor + "background-image: url("
                 + Settings::thumbsBackImage
                 + "); background-attachment: fixed; }";
    setStyleSheet(ss);

    QPalette scrollBarOrigPal = verticalScrollBar()->palette();
    QPalette thumbViewOrigPal = palette();
    thumbViewOrigPal.setColor(QPalette::Text, Settings::thumbsTextColor);
    setPalette(thumbViewOrigPal);
    verticalScrollBar()->setPalette(scrollBarOrigPal);
}

void ThumbsViewer::selectCurrentIndex() {
    if (currentIndex.isValid() && thumbsViewerModel->rowCount() > 0) {
        scrollTo(currentIndex);
        setCurrentIndex(currentIndex);
    }
}

QString ThumbsViewer::getSingleSelectionFilename() {
    if (selectionModel()->selectedIndexes().size() == 1)
        return thumbsViewerModel->item(selectionModel()->selectedIndexes().first().row())->data(
                FileNameRole).toString();

    return ("");
}

int ThumbsViewer::getNextRow() {
    if (currentRow == thumbsViewerModel->rowCount() - 1)
        return -1;

    return currentRow + 1;
}

int ThumbsViewer::getPrevRow() {
    if (currentRow == 0)
        return -1;

    return currentRow - 1;
}

int ThumbsViewer::getLastRow() {
    return thumbsViewerModel->rowCount() - 1;
}

int ThumbsViewer::getRandomRow() {
    return qrand() % (thumbsViewerModel->rowCount());
}

int ThumbsViewer::getCurrentRow() {
    return currentRow;
}

void ThumbsViewer::setCurrentRow(int row) {
    if (row >= 0)
        currentRow = row;
    else
        currentRow = 0;
}

void ThumbsViewer::setImageViewerWindowTitle() {
    QString title = thumbsViewerModel->item(currentRow)->data(FileNameRole).toString()
                    + " - ["
                    + QString::number(currentRow + 1)
                    + "/"
                    + QString::number(thumbsViewerModel->rowCount())
                    + "] - Phototonic";


    mainWindow->setWindowTitle(title);
}

bool ThumbsViewer::setCurrentIndexByName(QString &fileName) {
    QModelIndexList indexList = thumbsViewerModel->match(thumbsViewerModel->index(0, 0), FileNameRole, fileName);
    if (indexList.size()) {
        currentIndex = indexList[0];
        setCurrentRow(currentIndex.row());
        setRowHidden(currentIndex.row(), false);
        return true;
    }

    return false;
}

bool ThumbsViewer::setCurrentIndexByRow(int row) {
    QModelIndex idx = thumbsViewerModel->indexFromItem(thumbsViewerModel->item(row));
    if (idx.isValid()) {
        currentIndex = idx;
        setCurrentRow(idx.row());
        return true;
    }

    return false;
}

void ThumbsViewer::updateImageInfoViewer(QString imageFullPath) {

    imageInfoReader.setFileName(imageFullPath);
    QString key;
    QString val;

    QFileInfo imageInfo = QFileInfo(imageFullPath);
    infoView->addTitleEntry(tr("Image"));

    key = tr("File name");
    val = imageInfo.fileName();
    infoView->addEntry(key, val);

    key = tr("Location");
    val = imageInfo.path();
    infoView->addEntry(key, val);

    key = tr("Size");
    val = QString::number(imageInfo.size() / 1024.0, 'f', 2) + "K";
    infoView->addEntry(key, val);

    key = tr("Modified");
    val = imageInfo.lastModified().toString(Qt::SystemLocaleShortDate);
    infoView->addEntry(key, val);

    if (imageInfoReader.size().isValid()) {
        key = tr("Format");
        val = imageInfoReader.format().toUpper();
        infoView->addEntry(key, val);

        key = tr("Resolution");
        val = QString::number(imageInfoReader.size().width())
              + "x"
              + QString::number(imageInfoReader.size().height());
        infoView->addEntry(key, val);

        key = tr("Megapixel");
        val = QString::number((imageInfoReader.size().width() * imageInfoReader.size().height()) / 1000000.0, 'f',
                              2);
        infoView->addEntry(key, val);
    } else {
        imageInfoReader.read();
        key = tr("Error");
        val = imageInfoReader.errorString();
        infoView->addEntry(key, val);
    }

    Exiv2::Image::AutoPtr exifImage;
    try {
        exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
        exifImage->readMetadata();
    }
    catch (Exiv2::Error &error) {
        return;
    }

    Exiv2::ExifData &exifData = exifImage->exifData();
    if (!exifData.empty()) {
        Exiv2::ExifData::const_iterator end = exifData.end();
        infoView->addTitleEntry("Exif");
        for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
            key = QString::fromUtf8(md->tagName().c_str());
            val = QString::fromUtf8(md->print().c_str());
            infoView->addEntry(key, val);
        }
    }

    Exiv2::IptcData &iptcData = exifImage->iptcData();
    if (!iptcData.empty()) {
        Exiv2::IptcData::iterator end = iptcData.end();
        infoView->addTitleEntry("IPTC");
        for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {
            key = QString::fromUtf8(md->tagName().c_str());
            val = QString::fromUtf8(md->print().c_str());
            infoView->addEntry(key, val);
        }
    }

    Exiv2::XmpData &xmpData = exifImage->xmpData();
    if (!xmpData.empty()) {
        Exiv2::XmpData::iterator end = xmpData.end();
        infoView->addTitleEntry("XMP");
        for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
            key = QString::fromUtf8(md->tagName().c_str());
            val = QString::fromUtf8(md->print().c_str());
            infoView->addEntry(key, val);
        }
    }
}

void ThumbsViewer::handleSelectionChanged(const QItemSelection &) {
    QString info;
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    int nSelected = indexesList.size();
    QString imageFullPath;
    QString statusStr;

    infoView->clear();

    if (nSelected == 1) {
        QString thumbFullPath = thumbsViewerModel->item(indexesList.first().row())->data(FileNameRole).toString();
        updateImageInfoViewer(thumbFullPath);
    }

    if (imageTags->currentDisplayMode == SelectionTagsDisplay) {
        imageTags->showSelectedImagesTags();
    }

    /* update status bar */
    if (!nSelected) {
        updateThumbsCount();
        return;
    } else if (nSelected >= 1) {
        statusStr = tr("Selected %1 of%2")
                .arg(QString::number(nSelected))
                .arg(tr(" %n image(s)", "", thumbsViewerModel->rowCount()));
    }

    emit setStatus(statusStr);
}

QStringList ThumbsViewer::getSelectedThumbsList() {
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    QStringList SelectedThumbsPaths;

    for (int tn = indexesList.size() - 1; tn >= 0; --tn) {
        SelectedThumbsPaths <<
                            thumbsViewerModel->item(indexesList[tn].row())->data(FileNameRole).toString();
    }

    return SelectedThumbsPaths;
}

void ThumbsViewer::startDrag(Qt::DropActions) {
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    if (indexesList.isEmpty()) {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    QList<QUrl> urls;
    for (QModelIndexList::const_iterator it = indexesList.constBegin(),
                 end = indexesList.constEnd(); it != end; ++it) {
        urls << QUrl(thumbsViewerModel->item(it->row())->data(FileNameRole).toString());
    }
    mimeData->setUrls(urls);
    drag->setMimeData(mimeData);
    QPixmap pix;
    if (indexesList.count() > 1) {
        pix = QPixmap(128, 112);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, 2));
        int x = 0, y = 0, xMax = 0, yMax = 0;
        for (int i = 0; i < qMin(5, indexesList.count()); ++i) {
            QPixmap pix = thumbsViewerModel->item(indexesList.at(i).row())->icon().pixmap(72);
            if (i == 4) {
                x = (xMax - pix.width()) / 2;
                y = (yMax - pix.height()) / 2;
            }
            painter.drawPixmap(x, y, pix);
            xMax = qMax(xMax, qMin(128, x + pix.width()));
            yMax = qMax(yMax, qMin(112, y + pix.height()));
            painter.drawRect(x + 1, y + 1, qMin(126, pix.width() - 2), qMin(110, pix.height() - 2));
            x = !(x == y) * 56;
            y = !y * 40;
        }
        painter.end();
        pix = pix.copy(0, 0, xMax, yMax);
        drag->setPixmap(pix);
    } else {
        pix = thumbsViewerModel->item(indexesList.at(0).row())->icon().pixmap(128);
        drag->setPixmap(pix);
    }
    drag->setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));
    drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction, Qt::IgnoreAction);
}

void ThumbsViewer::abort() {
    abortOp = true;
}

void ThumbsViewer::loadVisibleThumbs(int scrollBarValue) {
    static int lastScrollBarValue = 0;

    scrolledForward = (scrollBarValue >= lastScrollBarValue);

    lastScrollBarValue = scrollBarValue;

    Start:
    int firstVisible = getFirstVisibleThumb();
    int lastVisible = getLastVisibleThumb();
    if (abortOp || firstVisible < 0 || lastVisible < 0) {
        return;
    }

    if (scrolledForward) {
        lastVisible += ((lastVisible - firstVisible) * (Settings::thumbPagesReadahead + 1));
        if (lastVisible >= thumbsViewerModel->rowCount()) {
            lastVisible = thumbsViewerModel->rowCount() - 1;
        }
    } else {
        firstVisible -= (lastVisible - firstVisible) * (Settings::thumbPagesReadahead + 1);
        if (firstVisible < 0) {
            firstVisible = 0;
        }

        lastVisible += 10;
        if (lastVisible >= thumbsViewerModel->rowCount()) {
            lastVisible = thumbsViewerModel->rowCount() - 1;
        }
    }

    if (thumbsRangeFirst == firstVisible && thumbsRangeLast == lastVisible) {
        return;
    }

    thumbsRangeFirst = firstVisible;
    thumbsRangeLast = lastVisible;

    loadThumbsRange();
    if (!abortOp) {
        goto Start;
    }
}

int ThumbsViewer::getFirstVisibleThumb() {
    QModelIndex idx;

    for (int currThumb = 0; currThumb < thumbsViewerModel->rowCount(); ++currThumb) {
        idx = thumbsViewerModel->indexFromItem(thumbsViewerModel->item(currThumb));
        if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1))) {
            return idx.row();
        }
    }

    return -1;
}

int ThumbsViewer::getLastVisibleThumb() {
    QModelIndex idx;

    for (int currThumb = thumbsViewerModel->rowCount() - 1; currThumb >= 0; --currThumb) {
        idx = thumbsViewerModel->indexFromItem(thumbsViewerModel->item(currThumb));
        if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1))) {
            return idx.row();
        }
    }

    return -1;
}

void ThumbsViewer::updateThumbsCount() {
    QString state;

    if (thumbsViewerModel->rowCount() > 0) {
        state = tr("%n image(s)", "", thumbsViewerModel->rowCount());
    } else {
        state = tr("No images");
    }
    thumbsDir->setPath(Settings::currentViewDir);
    emit setStatus(state);
}

void ThumbsViewer::loadPrepare() {

    setIconSize(QSize(thumbSize, thumbSize));

    fileFilters->clear();
    QString textFilter("*");
    textFilter += filterStr;
    *fileFilters << textFilter + "*.bmp"
                 << textFilter + "*.cur"
                 << textFilter + "*.dds"
                 << textFilter + "*.gif"
                 << textFilter + "*.icns"
                 << textFilter + "*.ico"
                 << textFilter + "*.jpeg"
                 << textFilter + "*.jpg"
                 << textFilter + "*.jp2"
                 << textFilter + "*.jpe"
                 << textFilter + "*.mng"
                 << textFilter + "*.pbm"
                 << textFilter + "*.pgm"
                 << textFilter + "*.png"
                 << textFilter + "*.ppm"
                 << textFilter + "*.svg"
                 << textFilter + "*.svgz"
                 << textFilter + "*.tga"
                 << textFilter + "*.tif"
                 << textFilter + "*.tiff"
                 << textFilter + "*.wbmp"
                 << textFilter + "*.webp"
                 << textFilter + "*.xbm"
                 << textFilter + "*.xpm";
    thumbsDir->setNameFilters(*fileFilters);
    thumbsDir->setFilter(QDir::Files);
    if (Settings::showHiddenFiles) {
        thumbsDir->setFilter(thumbsDir->filter() | QDir::Hidden);
    }

    thumbsDir->setPath(Settings::currentViewDir);

    QDir::SortFlags tempThumbsSortFlags = thumbsSortFlags;
    if (tempThumbsSortFlags & QDir::Size || tempThumbsSortFlags & QDir::Time) {
        tempThumbsSortFlags ^= QDir::Reversed;
    }
    thumbsDir->setSorting(tempThumbsSortFlags);

    thumbsViewerModel->clear();
    setSpacing(10);

    if (isNeedScroll) {
        scrollToTop();
    }

    abortOp = false;

    thumbsRangeFirst = -1;
    thumbsRangeLast = -1;

    imageTags->resetTagsState();
}

void ThumbsViewer::load() {
    emit showBusy(true);
    loadPrepare();
    initThumbs();
    updateThumbsCount();
    loadVisibleThumbs();

    if (Settings::includeSubDirectories) {
        QDirIterator iterator(Settings::currentViewDir, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..") {
                thumbsDir->setPath(iterator.filePath());

                initThumbs();
                updateThumbsCount();
                loadVisibleThumbs();

                if (abortOp) {
                    goto finish;
                }
            }
            QApplication::processEvents();
        }

        QItemSelection dummy;
        handleSelectionChanged(dummy);
    }

    finish:
    emit showBusy(false);
    busy = false;
    return;
}

void ThumbsViewer::initThumbs() {
    thumbFileInfoList = thumbsDir->entryInfoList();
    static QStandardItem *thumbIitem;
    static int fileIndex;
    static QPixmap emptyPixMap;
    static QSize hintSize;
    int processEventsCounter = 0;

    emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbSize, thumbSize);
    hintSize = QSize(thumbSize, thumbSize + ((int) (QFontMetrics(font()).height() * 1.5)));

    for (fileIndex = 0; fileIndex < thumbFileInfoList.size(); ++fileIndex) {
        thumbFileInfo = thumbFileInfoList.at(fileIndex);

        metadataCache->loadImageMetadata(thumbFileInfo.filePath());
        if (imageTags->folderFilteringActive && imageTags->isImageFilteredOut(thumbFileInfo.filePath())) {
            continue;
        }

        thumbIitem = new QStandardItem();
        thumbIitem->setData(false, LoadedRole);
        thumbIitem->setData(fileIndex, SortRole);
        thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
        thumbIitem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
        thumbIitem->setSizeHint(hintSize);
        thumbIitem->setText(thumbFileInfo.fileName());

        thumbsViewerModel->appendRow(thumbIitem);

        ++processEventsCounter;
        if (processEventsCounter > 100) {
            processEventsCounter = 0;
            QApplication::processEvents();
        }
    }

    imageTags->populateTagsTree();
    if (imageTags->currentDisplayMode == SelectionTagsDisplay) {
        imageTags->showSelectedImagesTags();
    } else {
        imageTags->showTagsFilter();
    }

    if (thumbFileInfoList.size() && selectionModel()->selectedIndexes().size() == 0) {
        selectThumbByRow(0);
    }
}

void ThumbsViewer::selectThumbByRow(int row) {
    setCurrentIndexByRow(row);
    selectCurrentIndex();
}

void ThumbsViewer::loadThumbsRange() {
    static bool inProgress = false;
    static QImageReader thumbReader;
    static QSize currThumbSize;
    static int currRowCount;
    static QString imageFileName;
    QImage thumb;
    int currThumb;
    bool imageReadOk;

    if (inProgress) {
        abortOp = true;
        QTimer::singleShot(0, this, SLOT(loadThumbsRange()));
        return;
    }

    inProgress = true;
    currRowCount = thumbsViewerModel->rowCount();

    for (scrolledForward ? currThumb = thumbsRangeFirst : currThumb = thumbsRangeLast;
         (scrolledForward ? currThumb <= thumbsRangeLast : currThumb >= thumbsRangeFirst);
         scrolledForward ? ++currThumb : --currThumb) {

        if (abortOp || thumbsViewerModel->rowCount() != currRowCount || currThumb < 0) {
            break;
        }

        if (thumbsViewerModel->item(currThumb)->data(LoadedRole).toBool()) {
            continue;
        }

        imageFileName = thumbsViewerModel->item(currThumb)->data(FileNameRole).toString();
        thumbReader.setFileName(imageFileName);
        currThumbSize = thumbReader.size();
        imageReadOk = false;

        if (currThumbSize.isValid()) {
            if (currThumbSize.width() > thumbSize || currThumbSize.height() > thumbSize) {
                currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
            }

            thumbReader.setScaledSize(currThumbSize);
            imageReadOk = thumbReader.read(&thumb);
        }

        if (imageReadOk) {
            if (Settings::exifThumbRotationEnabled) {
                imageViewer->rotateByExifRotation(thumb, imageFileName);
                currThumbSize = thumb.size();
                currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
            }

            thumbsViewerModel->item(currThumb)->setIcon(QPixmap::fromImage(thumb));
        } else {
            thumbsViewerModel->item(currThumb)->setIcon(QIcon::fromTheme("image-missing",
                                                                         QIcon(":/images/error_image.png")).pixmap(
                    BAD_IMG_SZ, BAD_IMG_SZ));
            currThumbSize.setHeight(BAD_IMG_SZ);
            currThumbSize.setWidth(BAD_IMG_SZ);
        }

        thumbsViewerModel->item(currThumb)->setData(true, LoadedRole);
        QApplication::processEvents();
    }

    inProgress = false;
    abortOp = false;
}

void ThumbsViewer::addThumb(QString &imageFullPath) {
    QStandardItem *thumbIitem = new QStandardItem();
    QImageReader thumbReader;
    QSize hintSize;
    QSize currThumbSize;
    static QImage thumb;

    hintSize = QSize(thumbSize, thumbSize + ((int) (QFontMetrics(font()).height() * 1.5)));

    thumbFileInfo = QFileInfo(imageFullPath);
    thumbIitem->setData(true, LoadedRole);
    thumbIitem->setData(0, SortRole);
    thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
    thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);

    thumbReader.setFileName(imageFullPath);
    currThumbSize = thumbReader.size();
    if (currThumbSize.isValid()) {
        if (currThumbSize.width() > thumbSize || currThumbSize.height() > thumbSize) {
            currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
        }

        thumbReader.setScaledSize(currThumbSize);
        thumb = thumbReader.read();

        if (Settings::exifThumbRotationEnabled) {
            imageViewer->rotateByExifRotation(thumb, imageFullPath);
            currThumbSize = thumb.size();
            currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
        }

        thumbIitem->setIcon(QPixmap::fromImage(thumb));
    } else {
        thumbIitem->setIcon(
                QIcon::fromTheme("image-missing", QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
        currThumbSize.setHeight(BAD_IMG_SZ);
        currThumbSize.setWidth(BAD_IMG_SZ);
    }


    thumbIitem->setSizeHint(hintSize);
    thumbsViewerModel->appendRow(thumbIitem);
}

void ThumbsViewer::wheelEvent(QWheelEvent *event) {
    if (event->delta() < 0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + thumbSize);
    } else {
        verticalScrollBar()->setValue(verticalScrollBar()->value() - thumbSize);
    }
}

void ThumbsViewer::mousePressEvent(QMouseEvent *event) {
    QListView::mousePressEvent(event);

    if (Settings::reverseMouseBehavior && event->button() == Qt::MiddleButton) {
        if (selectionModel()->selectedIndexes().size() == 1)
                emit(doubleClicked(selectionModel()->selectedIndexes().first()));
    }
}

void ThumbsViewer::invertSelection() {
    QItemSelection toggleSelection;
    QModelIndex firstIndex = thumbsViewerModel->index(0, 0);
    QModelIndex lastIndex = thumbsViewerModel->index(thumbsViewerModel->rowCount() - 1, 0);
    toggleSelection.select(firstIndex, lastIndex);
    selectionModel()->select(toggleSelection, QItemSelectionModel::Toggle);
}

void ThumbsViewer::setNeedScroll(bool needScroll) {
    isNeedScroll = needScroll;
}

void ThumbsViewer::setImageView(ImageViewer *imageViewer) {
    this->imageViewer = imageViewer;
}

