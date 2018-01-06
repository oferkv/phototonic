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

#include "thumbview.h"

ThumbView::ThumbView(QWidget *parent, MetadataCache *mdCache) : QListView(parent) {
    this->mdCache = mdCache;
    Settings::thumbsBackgroundColor = Settings::appSettings->value("backgroundThumbColor").value<QColor>();
    Settings::thumbsTextColor = Settings::appSettings->value("textThumbColor").value<QColor>();
    setThumbColors();
    Settings::thumbSpacing = Settings::appSettings->value("thumbSpacing").toInt();
    Settings::thumbPagesReadahead = Settings::appSettings->value("thumbPagesReadahead").toInt();
    thumbSize = Settings::appSettings->value("thumbsZoomVal").toInt();
    currentRow = 0;

    setViewMode(QListView::IconMode);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformItemSizes(false);

    thumbViewModel = new QStandardItemModel(this);
    thumbViewModel->setSortRole(SortRole);
    setModel(thumbViewModel);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(loadVisibleThumbs(int)));
    connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection)));
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), parent, SLOT(loadImagefromThumb(const QModelIndex &)));

    thumbsDir = new QDir();
    fileFilters = new QStringList;

    emptyImg.load(":/images/no_image.png");

    QTime time = QTime::currentTime();
    qsrand((uint) time.msec());
    mainWindow = parent;
    infoView = new InfoView(this);
}

void ThumbView::setThumbColors() {
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

void ThumbView::selectCurrentIndex() {
    if (currentIndex.isValid() && thumbViewModel->rowCount() > 0) {
        scrollTo(currentIndex);
        setCurrentIndex(currentIndex);
    }
}

QString ThumbView::getSingleSelectionFilename() {
    if (selectionModel()->selectedIndexes().size() == 1)
        return thumbViewModel->item(selectionModel()->selectedIndexes().first().row())->data(FileNameRole).toString();

    return ("");
}

int ThumbView::getNextRow() {
    if (currentRow == thumbViewModel->rowCount() - 1)
        return -1;

    return currentRow + 1;
}

int ThumbView::getPrevRow() {
    if (currentRow == 0)
        return -1;

    return currentRow - 1;
}

int ThumbView::getLastRow() {
    return thumbViewModel->rowCount() - 1;
}

int ThumbView::getRandomRow() {
    return qrand() % (thumbViewModel->rowCount());
}

int ThumbView::getCurrentRow() {
    return currentRow;
}

void ThumbView::setCurrentRow(int row) {
    if (row >= 0)
        currentRow = row;
    else
        currentRow = 0;
}

void ThumbView::setImageviewWindowTitle() {
    QString title = thumbViewModel->item(currentRow)->data(FileNameRole).toString()
                    + " - ["
                    + QString::number(currentRow + 1)
                    + "/"
                    + QString::number(thumbViewModel->rowCount())
                    + "] - Phototonic";


    mainWindow->setWindowTitle(title);
}

bool ThumbView::setCurrentIndexByName(QString &FileName) {
    QModelIndexList indexList = thumbViewModel->match(thumbViewModel->index(0, 0), FileNameRole, FileName);
    if (indexList.size()) {
        currentIndex = indexList[0];
        setCurrentRow(currentIndex.row());
        setRowHidden(currentIndex.row(), false);
        return true;
    }

    return false;
}

bool ThumbView::setCurrentIndexByRow(int row) {
    QModelIndex idx = thumbViewModel->indexFromItem(thumbViewModel->item(row));
    if (idx.isValid()) {
        currentIndex = idx;
        setCurrentRow(idx.row());
        return true;
    }

    return false;
}

void ThumbView::updateExifInfo(QString imageFullPath) {
    Exiv2::Image::AutoPtr exifImage;
    QString key;
    QString val;

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

void ThumbView::handleSelectionChanged(const QItemSelection &) {
    QString info;
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    int nSelected = indexesList.size();
    QString imageFullPath;
    QString statusStr;

    infoView->clear();

    if (nSelected == 1) {
        QString imageFullPath = thumbViewModel->item(indexesList.first().row())->data(FileNameRole).toString();
        imageInfoReader.setFileName(imageFullPath);
        QString key;
        QString val;

        QFileInfo imageInfo = QFileInfo(imageFullPath);
        infoView->addTitleEntry(tr("General"));

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

            updateExifInfo(imageFullPath);
            recentThumb = imageFullPath;
        } else {
            imageInfoReader.read();
            key = tr("Error");
            val = imageInfoReader.errorString();
            infoView->addEntry(key, val);
        }
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
                .arg(tr(" %n image(s)", "", thumbViewModel->rowCount()));
    }

    emit setStatus(statusStr);
}

QStringList ThumbView::getSelectedThumbsList() {
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    QStringList SelectedThumbsPaths;

    for (int tn = indexesList.size() - 1; tn >= 0; --tn) {
        SelectedThumbsPaths <<
                            thumbViewModel->item(indexesList[tn].row())->data(FileNameRole).toString();
    }

    return SelectedThumbsPaths;
}

void ThumbView::startDrag(Qt::DropActions) {
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    if (indexesList.isEmpty()) {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    QList<QUrl> urls;
    for (QModelIndexList::const_iterator it = indexesList.constBegin(),
                 end = indexesList.constEnd(); it != end; ++it) {
        urls << QUrl(thumbViewModel->item(it->row())->data(FileNameRole).toString());
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
            QPixmap pix = thumbViewModel->item(indexesList.at(i).row())->icon().pixmap(72);
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
        pix = thumbViewModel->item(indexesList.at(0).row())->icon().pixmap(128);
        drag->setPixmap(pix);
    }
    drag->setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));
    drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction, Qt::IgnoreAction);
}

void ThumbView::abort() {
    abortOp = true;
}

void ThumbView::loadVisibleThumbs(int scrollBarValue) {
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
        if (lastVisible >= thumbViewModel->rowCount()) {
            lastVisible = thumbViewModel->rowCount() - 1;
        }
    } else {
        firstVisible -= (lastVisible - firstVisible) * (Settings::thumbPagesReadahead + 1);
        if (firstVisible < 0) {
            firstVisible = 0;
        }

        lastVisible += 10;
        if (lastVisible >= thumbViewModel->rowCount()) {
            lastVisible = thumbViewModel->rowCount() - 1;
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

int ThumbView::getFirstVisibleThumb() {
    QModelIndex idx;

    for (int currThumb = 0; currThumb < thumbViewModel->rowCount(); ++currThumb) {
        idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
        if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1))) {
            return idx.row();
        }
    }

    return -1;
}

int ThumbView::getLastVisibleThumb() {
    QModelIndex idx;

    for (int currThumb = thumbViewModel->rowCount() - 1; currThumb >= 0; --currThumb) {
        idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
        if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1))) {
            return idx.row();
        }
    }

    return -1;
}

void ThumbView::updateThumbsCount() {
    QString state;

    if (thumbViewModel->rowCount() > 0) {
        state = tr("%n image(s)", "", thumbViewModel->rowCount());
    } else {
        state = tr("No images");
    }
    thumbsDir->setPath(Settings::currentViewDir);
    emit setStatus(state);
}

void ThumbView::loadPrepare() {

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

    thumbViewModel->clear();
    setSpacing(Settings::thumbSpacing);

    if (isNeedScroll) {
        scrollToTop();
    }

    abortOp = false;
    newIndex = 0;

    thumbsRangeFirst = -1;
    thumbsRangeLast = -1;

    imageTags->resetTagsState();
}

void ThumbView::load() {
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

void ThumbView::loadDuplicates() {
    loadPrepare();

    emit showBusy(true);
    emit setStatus(tr("Searching duplicate images..."));

    dupImageHashes.clear();
    findDupes(true);

    if (Settings::includeSubDirectories) {
        QDirIterator iterator(Settings::currentViewDir, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..") {
                thumbsDir->setPath(iterator.filePath());

                findDupes(false);
                if (abortOp) {
                    goto finish;
                }
            }
            QApplication::processEvents();
        }
    }

    finish:
    busy = false;
    emit showBusy(false);
    return;
}

void ThumbView::initThumbs() {
    thumbFileInfoList = thumbsDir->entryInfoList();
    static QStandardItem *thumbIitem;
    static int fileIndex;
    static QPixmap emptyPixMap;
    static QSize hintSize;
    int processEventsCounter = 0;

    emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbSize, thumbSize);
    hintSize = QSize(thumbSize, thumbSize + QFontMetrics(font()).height());

    for (fileIndex = 0; fileIndex < thumbFileInfoList.size(); ++fileIndex) {
        thumbFileInfo = thumbFileInfoList.at(fileIndex);

        mdCache->loadImageMetadata(thumbFileInfo.filePath());
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

        thumbViewModel->appendRow(thumbIitem);

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

void ThumbView::selectThumbByRow(int row) {
    setCurrentIndexByRow(row);
    selectCurrentIndex();
}

void ThumbView::updateFoundDupesState(int duplicates, int filesScanned, int originalImages) {
    QString state;
    state = tr("Scanned %1, displaying %2 (%3 and %4)")
            .arg(tr("%n image(s)", "", filesScanned))
            .arg(tr("%n image(s)", "", originalImages + duplicates))
            .arg(tr("%n original(s)", "", originalImages))
            .arg(tr("%n duplicate(s)", "", duplicates));
    emit setStatus(state);
}

void ThumbView::findDupes(bool resetCounters) {
    thumbFileInfoList = thumbsDir->entryInfoList();
    int processEventsCounter = 0;
    static int originalImages;
    static int foundDups;
    static int totalFiles;
    if (resetCounters) {
        originalImages = totalFiles = foundDups = 0;
    }

    for (int currThumb = 0; currThumb < thumbFileInfoList.size(); ++currThumb) {
        thumbFileInfo = thumbFileInfoList.at(currThumb);
        QCryptographicHash md5gen(QCryptographicHash::Md5);
        QString currentFilePath = thumbFileInfo.filePath();

        QFile file(currentFilePath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        totalFiles++;

        md5gen.addData(file.readAll());
        file.close();
        QString md5 = md5gen.result().toHex();

        if (dupImageHashes.contains(md5)) {
            if (dupImageHashes[md5].duplicates < 1) {
                addThumb(dupImageHashes[md5].filePath);
                originalImages++;
            }

            foundDups++;
            dupImageHashes[md5].duplicates++;
            addThumb(currentFilePath);
        } else {
            DuplicateImage dupImage;
            dupImage.filePath = currentFilePath;
            dupImage.duplicates = 0;
            dupImageHashes.insert(md5, dupImage);
        }

        ++processEventsCounter;
        if (processEventsCounter > 9) {
            processEventsCounter = 0;
            QApplication::processEvents();
        }

        updateFoundDupesState(foundDups, totalFiles, originalImages);

        if (abortOp) {
            break;
        }
    }

    updateFoundDupesState(foundDups, totalFiles, originalImages);
}

void ThumbView::loadThumbsRange() {
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
    currRowCount = thumbViewModel->rowCount();

    for (scrolledForward ? currThumb = thumbsRangeFirst : currThumb = thumbsRangeLast;
         (scrolledForward ? currThumb <= thumbsRangeLast : currThumb >= thumbsRangeFirst);
         scrolledForward ? ++currThumb : --currThumb) {

        if (abortOp || thumbViewModel->rowCount() != currRowCount || currThumb < 0) {
            break;
        }

        if (thumbViewModel->item(currThumb)->data(LoadedRole).toBool()) {
            continue;
        }

        imageFileName = thumbViewModel->item(currThumb)->data(FileNameRole).toString();
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
                imageView->rotateByExifRotation(thumb, imageFileName);
                currThumbSize = thumb.size();
                currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
            }

            thumbViewModel->item(currThumb)->setIcon(QPixmap::fromImage(thumb));
        } else {
            thumbViewModel->item(currThumb)->setIcon(QIcon::fromTheme("image-missing",
                                                                      QIcon(":/images/error_image.png")).pixmap(
                    BAD_IMG_SZ, BAD_IMG_SZ));
            currThumbSize.setHeight(BAD_IMG_SZ);
            currThumbSize.setWidth(BAD_IMG_SZ);
        }

        thumbViewModel->item(currThumb)->setData(true, LoadedRole);
        QApplication::processEvents();
    }

    inProgress = false;
    abortOp = false;
}

void ThumbView::addThumb(QString &imageFullPath) {
    QStandardItem *thumbIitem = new QStandardItem();
    QImageReader thumbReader;
    QSize hintSize;
    QSize currThumbSize;
    static QImage thumb;

    hintSize = QSize(thumbSize, thumbSize + (QFontMetrics(font()).height() + 5));

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
            imageView->rotateByExifRotation(thumb, imageFullPath);
            currThumbSize = thumb.size();
            currThumbSize.scale(QSize(thumbSize, thumbSize), Qt::KeepAspectRatio);
        }

        thumbIitem->setIcon(QPixmap::fromImage(thumb));
    } else {
        thumbIitem->setIcon(QIcon::fromTheme("image-missing", QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
        currThumbSize.setHeight(BAD_IMG_SZ);
        currThumbSize.setWidth(BAD_IMG_SZ);
    }


    thumbIitem->setSizeHint(hintSize);
    thumbViewModel->appendRow(thumbIitem);
}

void ThumbView::wheelEvent(QWheelEvent *event) {
    if (event->delta() < 0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + thumbSize);
    } else {
        verticalScrollBar()->setValue(verticalScrollBar()->value() - thumbSize);
    }
}

void ThumbView::mousePressEvent(QMouseEvent *event) {
    QListView::mousePressEvent(event);

    if (Settings::reverseMouseBehavior && event->button() == Qt::MiddleButton) {
        if (selectionModel()->selectedIndexes().size() == 1)
                emit(doubleClicked(selectionModel()->selectedIndexes().first()));
    }
}

void ThumbView::invertSelection() {
    QItemSelection toggleSelection;
    QModelIndex firstIndex = thumbViewModel->index(0, 0);
    QModelIndex lastIndex = thumbViewModel->index(thumbViewModel->rowCount() - 1, 0);
    toggleSelection.select(firstIndex, lastIndex);
    selectionModel()->select(toggleSelection, QItemSelectionModel::Toggle);
}

void ThumbView::setNeedScroll(bool needScroll) {
    isNeedScroll = needScroll;
}

void ThumbView::setImageView(ImageView *imageView) {
    this->imageView = imageView;
}

