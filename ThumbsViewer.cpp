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

#include <QMimeDatabase>
#include <QProgressDialog>
#include <QRandomGenerator>

#include "ThumbsViewer.h"
#include "Phototonic.h"

#define BATCH_SIZE 10

ThumbsViewer::ThumbsViewer(QWidget *parent, MetadataCache *metadataCache) : QListView(parent) {
    this->metadataCache = metadataCache;
    Settings::thumbsBackgroundColor = Settings::appSettings->value(
            Settings::optionThumbsBackgroundColor).value<QColor>();
    Settings::thumbsTextColor = Settings::appSettings->value(Settings::optionThumbsTextColor).value<QColor>();
    setThumbColors();
    Settings::thumbsPagesReadCount = Settings::appSettings->value(Settings::optionThumbsPagesReadCount).toUInt();
    thumbSize = Settings::appSettings->value(Settings::optionThumbsZoomLevel).toInt();
    currentRow = 0;

    setViewMode(QListView::IconMode);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setWrapping(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformItemSizes(false);

    // This is the default but set for clarity. Could make it configurable to use
    // QAbstractItemView::ScrollPerPixel instead.
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    thumbsViewerModel = new QStandardItemModel(this);
    thumbsViewerModel->setSortRole(SortRole);
    setModel(thumbsViewerModel);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(loadVisibleThumbs(int)));
    m_selectionChangedTimer.setInterval(10);
    m_selectionChangedTimer.setSingleShot(true);
    connect(&m_selectionChangedTimer, &QTimer::timeout, this, &ThumbsViewer::onSelectionChanged);
    connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() {
        if (!m_selectionChangedTimer.isActive()) {
            m_selectionChangedTimer.start();
        }
    });

    m_loadThumbTimer.setInterval(10);
    m_loadThumbTimer.setSingleShot(true);
    connect(&m_loadThumbTimer, &QTimer::timeout, this, &ThumbsViewer::loadThumbsRange);
    connect(this, SIGNAL(doubleClicked(
                                 const QModelIndex &)), parent, SLOT(loadSelectedThumbImage(
                                                                             const QModelIndex &)));

    thumbsDir = new QDir();
    fileFilters = new QStringList;
    emptyImg.load(":/images/no_image.png");

    phototonic = (Phototonic *) parent;
    infoView = new InfoView(this);

    imagePreview = new ImagePreview(this);
}

void ThumbsViewer::setThumbColors() {
    QString backgroundColor = "background: rgb(%1, %2, %3); ";
    backgroundColor = backgroundColor.arg(Settings::thumbsBackgroundColor.red())
            .arg(Settings::thumbsBackgroundColor.green())
            .arg(Settings::thumbsBackgroundColor.blue());

    QString styleSheet = "QListView { " + backgroundColor + "background-image: url("
                         + Settings::thumbsBackgroundImage
                         + "); background-attachment: fixed; }";
    setStyleSheet(styleSheet);

    QPalette scrollBarOriginalPalette = verticalScrollBar()->palette();
    QPalette thumbViewerOriginalPalette = palette();
    thumbViewerOriginalPalette.setColor(QPalette::Text, Settings::thumbsTextColor);
    setPalette(thumbViewerOriginalPalette);
    verticalScrollBar()->setPalette(scrollBarOriginalPalette);
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
    if (currentRow == thumbsViewerModel->rowCount() - 1) {
        return -1;
    }

    return currentRow + 1;
}

int ThumbsViewer::getPrevRow() {
    if (currentRow == 0) {
        return -1;
    }

    return currentRow - 1;
}

int ThumbsViewer::getLastRow() {
    return thumbsViewerModel->rowCount() - 1;
}

int ThumbsViewer::getRandomRow() {
    return QRandomGenerator::global()->bounded(thumbsViewerModel->rowCount());
}

int ThumbsViewer::getCurrentRow() {
    return currentRow;
}

void ThumbsViewer::setCurrentRow(int row) {
    if (row >= 0) {
        currentRow = row;
    } else {
        currentRow = 0;
    }
}

void ThumbsViewer::setImageViewerWindowTitle() {
    QString title = thumbsViewerModel->item(currentRow)->data(Qt::DisplayRole).toString()
                    + " - ["
                    + QString::number(currentRow + 1)
                    + "/"
                    + QString::number(thumbsViewerModel->rowCount())
                    + "] - Phototonic";

    phototonic->setWindowTitle(title);
}

bool ThumbsViewer::setCurrentIndexByName(QString &fileName) {
    QModelIndexList indexList = thumbsViewerModel->match(thumbsViewerModel->index(0, 0), FileNameRole, fileName);
    if (indexList.size()) {
        currentIndex = indexList[0];
        setCurrentRow(currentIndex.row());
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

void ThumbsViewer::updateImageInfoViewer(int row) {
    QString imageFullPath = thumbsViewerModel->item(row)->data(FileNameRole).toString();
    QImageReader imageInfoReader(imageFullPath);
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

        key = tr("Average brightness");
        val = QString::number(thumbsViewerModel->item(row)->data(BrightnessRole).toReal(), 'f', 2);
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

void ThumbsViewer::onSelectionChanged() {
    infoView->clear();
    imagePreview->clear();
    if (Settings::setWindowIcon && Settings::layoutMode == Phototonic::ThumbViewWidget) {
        phototonic->setWindowIcon(phototonic->getDefaultWindowIcon());
    }

    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    int selectedThumbs = indexesList.size();
    if (selectedThumbs == 1) {
        int currentRow = indexesList.first().row();
        QString thumbFullPath = thumbsViewerModel->item(currentRow)->data(FileNameRole).toString();
        setCurrentRow(currentRow);

        if (infoView->isVisible()) {
            updateImageInfoViewer(currentRow);
        }

        QPixmap imagePreviewPixmap = imagePreview->loadImage(thumbFullPath);
        if (Settings::setWindowIcon && Settings::layoutMode == Phototonic::ThumbViewWidget) {
            phototonic->setWindowIcon(imagePreviewPixmap.scaled(WINDOW_ICON_SIZE, WINDOW_ICON_SIZE,
                                                                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    if (imageTags->isVisible() && imageTags->currentDisplayMode == SelectionTagsDisplay) {
        imageTags->showSelectedImagesTags();
    }

    if (selectedThumbs >= 1) {
        QString statusStr;
        statusStr = tr("Selected %1 of %2").arg(QString::number(selectedThumbs))
                .arg(tr(" %n image(s)", "", thumbsViewerModel->rowCount()));
        phototonic->setStatus(statusStr);
    } else if (!selectedThumbs) {
        updateThumbsCount();
    }
}

QStringList ThumbsViewer::getSelectedThumbsList() {
    QModelIndexList indexesList = selectionModel()->selectedIndexes();
    QStringList SelectedThumbsPaths;

    for (int tn = indexesList.size() - 1; tn >= 0; --tn) {
        SelectedThumbsPaths << thumbsViewerModel->item(indexesList[tn].row())->data(FileNameRole).toString();
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
    isAbortThumbsLoading = true;
}

void ThumbsViewer::loadVisibleThumbs(int scrollBarValue) {
    static int lastScrollBarValue = 0;

    if (scrollBarValue >= 0) {
        scrolledForward = (scrollBarValue >= lastScrollBarValue);
        lastScrollBarValue = scrollBarValue;
    } else {
        loadThumbsRange();
        return;
    }

    for (;;) {
        int firstVisible = getFirstVisibleThumb();
        int lastVisible = getLastVisibleThumb();
        if (isAbortThumbsLoading || firstVisible < 0 || lastVisible < 0) {
            return;
        }

        if (scrolledForward) {
            lastVisible += ((lastVisible - firstVisible) * (Settings::thumbsPagesReadCount + 1));
            if (lastVisible >= thumbsViewerModel->rowCount()) {
                lastVisible = thumbsViewerModel->rowCount() - 1;
            }
        } else {
            firstVisible -= (lastVisible - firstVisible) * (Settings::thumbsPagesReadCount + 1);
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
        if (isAbortThumbsLoading) {
            break;
        }
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

void ThumbsViewer::loadFileList() {
    for (int i = 0; i < Settings::filesList.size(); i++) {
        addThumb(Settings::filesList[i]);
    }
    updateThumbsCount();

    imageTags->populateTagsTree();

    if (thumbFileInfoList.size() && selectionModel()->selectedIndexes().size() == 0) {
        selectThumbByRow(0);
    }

    phototonic->showBusyAnimation(false);
    isBusy = false;
}

void ThumbsViewer::reLoad() {

    isBusy = true;
    phototonic->showBusyAnimation(true);
    loadPrepare();

    if (Settings::isFileListLoaded) {
        loadFileList();
        return;
    }

    applyFilter();
    initThumbs();
    updateThumbsCount();
    loadVisibleThumbs();

    if (Settings::includeSubDirectories) {
        loadSubDirectories();
    }

    phototonic->showBusyAnimation(false);
    isBusy = false;
}

void ThumbsViewer::loadSubDirectories() {
    QDirIterator dirIterator(Settings::currentDirectory, QDirIterator::Subdirectories);

    int processed = 0;
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if (dirIterator.fileInfo().isDir() && dirIterator.fileName() != "." && dirIterator.fileName() != "..") {
            thumbsDir->setPath(dirIterator.filePath());

            initThumbs();
            updateThumbsCount();
            loadVisibleThumbs();

            if (isAbortThumbsLoading) {
                return;
            }
        }
        if (++processed > BATCH_SIZE) {
            QApplication::processEvents();
            processed = 0;
        }
    }

    onSelectionChanged();
}

void ThumbsViewer::applyFilter() {
    fileFilters->clear();
    QString textFilter("*");
    textFilter += filterString;

    // Get all patterns supported by QImageReader
    static QStringList imageTypeGlobs;
    // Not threadsafe, but whatever
    if (imageTypeGlobs.isEmpty()) {
        QMimeDatabase db;
        for (const QString &type : QImageReader::supportedMimeTypes()) {
            imageTypeGlobs.append(db.mimeTypeForName(type).globPatterns());
        }
    }
    for (const QString &glob : imageTypeGlobs) {
        fileFilters->append(textFilter + glob);
    }

    thumbsDir->setNameFilters(*fileFilters);
    thumbsDir->setFilter(QDir::Files);
    if (Settings::showHiddenFiles) {
        thumbsDir->setFilter(thumbsDir->filter() | QDir::Hidden);
    }

    thumbsDir->setPath(Settings::currentDirectory);
    QDir::SortFlags tempThumbsSortFlags = thumbsSortFlags;
    if (tempThumbsSortFlags & QDir::Size || tempThumbsSortFlags & QDir::Time) {
        tempThumbsSortFlags ^= QDir::Reversed;
    }

    if (thumbsSortFlags & QDir::Time || thumbsSortFlags & QDir::Size || thumbsSortFlags & QDir::Type) {
        thumbsDir->setSorting(tempThumbsSortFlags);
    } else { // by name
        thumbsDir->setSorting(QDir::NoSort);
    }
}

void ThumbsViewer::loadPrepare() {

    thumbsViewerModel->clear();
    setIconSize(QSize(thumbSize, thumbSize));
    if (Settings::thumbsLayout == Squares) {
        setSpacing(0);
        setUniformItemSizes(true);
    } else {
        setSpacing(QFontMetrics(font()).height());
        setUniformItemSizes(false);
    }

    if (isNeedToScroll) {
        scrollToTop();
    }

    isAbortThumbsLoading = false;

    thumbsRangeFirst = -1;
    thumbsRangeLast = -1;

    imageTags->resetTagsState();
}

void ThumbsViewer::loadDuplicates()
{
    isBusy = true;
    phototonic->showBusyAnimation(true);
    loadPrepare();

    phototonic->setStatus(tr("Searching duplicate images..."));

    dupImageHashes.clear();
    findDupes(true);

    if (Settings::includeSubDirectories) {
        int processed = 0;
        QDirIterator iterator(Settings::currentDirectory, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..") {
                thumbsDir->setPath(iterator.filePath());

                findDupes(false);
                if (isAbortThumbsLoading) {
                    goto finish;
                }
            }
            if (++processed > BATCH_SIZE) {
                QApplication::processEvents();
                processed = 0;
            }
        }
    }

finish:
    isBusy = false;
    phototonic->showBusyAnimation(false);
    return;
}

void ThumbsViewer::initThumbs() {
    thumbFileInfoList = thumbsDir->entryInfoList();

    if (!(thumbsSortFlags & QDir::Time) && !(thumbsSortFlags & QDir::Size) && !(thumbsSortFlags & QDir::Type)) {
        QCollator collator;
        if (thumbsSortFlags & QDir::IgnoreCase) {
            collator.setCaseSensitivity(Qt::CaseInsensitive);
        }

        collator.setNumericMode(true);

        if (thumbsSortFlags & QDir::Reversed) {
            std::sort(thumbFileInfoList.begin(), thumbFileInfoList.end(), [&](const QFileInfo &a, const QFileInfo &b) {
                    return collator.compare(a.fileName(), b.fileName()) > 0;
                    });
        } else {
            std::sort(thumbFileInfoList.begin(), thumbFileInfoList.end(), [&](const QFileInfo &a, const QFileInfo &b) {
                    return collator.compare(a.fileName(), b.fileName()) < 0;
                    });
        }
    }

    static QStandardItem *thumbItem;
    static int fileIndex;
    static QPixmap emptyPixMap;
    static QSize hintSize;
    int processed = 0;

    emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbSize, thumbSize);
    hintSize = Settings::thumbsLayout == Classic ?
        QSize(thumbSize, thumbSize + ((int) (QFontMetrics(font()).height() * 1.5))) :
        QSize(thumbSize, thumbSize);

    for (fileIndex = 0; fileIndex < thumbFileInfoList.size(); ++fileIndex) {
        thumbFileInfo = thumbFileInfoList.at(fileIndex);

        metadataCache->loadImageMetadata(thumbFileInfo.filePath());
        if (imageTags->dirFilteringActive && imageTags->isImageFilteredOut(thumbFileInfo.filePath())) {
            continue;
        }

        thumbItem = new QStandardItem();
        thumbItem->setData(false, LoadedRole);
        thumbItem->setData(fileIndex, SortRole);
        thumbItem->setData(thumbFileInfo.size(), SizeRole);
        thumbItem->setData(thumbFileInfo.suffix(), TypeRole);
        thumbItem->setData(thumbFileInfo.lastModified(), TimeRole);
        thumbItem->setData(thumbFileInfo.filePath(), FileNameRole);
        thumbItem->setSizeHint(hintSize);
        if (Settings::thumbsLayout == Classic) {
            thumbItem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
            thumbItem->setText(thumbFileInfo.fileName());
        }

        thumbsViewerModel->appendRow(thumbItem);

        if (++processed > BATCH_SIZE) {
            QApplication::processEvents();
            processed = 0;
        }
    }

    imageTags->populateTagsTree();

    if (thumbFileInfoList.size() && selectionModel()->selectedIndexes().size() == 0) {
        selectThumbByRow(0);
    }
}

void ThumbsViewer::updateThumbsCount() {
    QString state;

    if (thumbsViewerModel->rowCount() > 0) {
        state = tr("%n image(s)", "", thumbsViewerModel->rowCount());
    } else {
        state = tr("No images");
    }
    thumbsDir->setPath(Settings::currentDirectory);
    phototonic->setStatus(state);
}

void ThumbsViewer::selectThumbByRow(int row) {
    setCurrentIndexByRow(row);
    selectCurrentIndex();
}

void ThumbsViewer::updateFoundDupesState(int duplicates, int filesScanned, int originalImages)
{
    QString state;
    state = tr("Scanned %1, displaying %2 (%3 and %4)")
                .arg(tr("%n image(s)", "", filesScanned))
                .arg(tr("%n image(s)", "", originalImages + duplicates))
                .arg(tr("%n original(s)", "", originalImages))
                .arg(tr("%n duplicate(s)", "", duplicates));
    phototonic->setStatus(state);
}

void ThumbsViewer::findDupes(bool resetCounters)
{
    thumbFileInfoList = thumbsDir->entryInfoList();
    static int originalImages;
    static int foundDups;
    static int totalFiles;
    if (resetCounters) {
        originalImages = totalFiles = foundDups = 0;
    }

    int processed = 0;
    for (int currThumb = 0; currThumb < thumbFileInfoList.size(); ++currThumb) {
        if (++processed > BATCH_SIZE) {
            QApplication::processEvents();
            processed = 0;
        }

        thumbFileInfo = thumbFileInfoList.at(currThumb);
        QImage image = QImage(thumbFileInfo.absoluteFilePath());
        if (image.isNull()) {
            qWarning() << "invalid image" << thumbFileInfo.fileName();
            continue;
        }

        QBitArray imageHash(64);
        image = image.convertToFormat(QImage::Format_Grayscale8).scaled(9, 9, Qt::KeepAspectRatioByExpanding);
        for (int y=0; y<8; y++) {
            const uchar *line = image.scanLine(y);
            //const uchar *nextLine = image.scanLine(y+1);
            for (int x=0; x<8; x++) {
                imageHash.setBit(y * 8 + x, line[x] > line[x+1]);
                //imageHash.setBit(y * 8 + x + 64, line[x] > nextLine[x]);
            }
        }

        QString currentFilePath = thumbFileInfo.filePath();

        totalFiles++;

        if (dupImageHashes.contains(imageHash)) {
            if (dupImageHashes[imageHash].duplicates < 1) {
                addThumb(dupImageHashes[imageHash].filePath);
                originalImages++;
            }

            foundDups++;
            dupImageHashes[imageHash].duplicates++;
            addThumb(currentFilePath);
        } else {
            DuplicateImage dupImage;
            dupImage.filePath = currentFilePath;
            dupImage.duplicates = 0;
            dupImageHashes.insert(imageHash, dupImage);
        }


        updateFoundDupesState(foundDups, totalFiles, originalImages);

        if (isAbortThumbsLoading) {
            break;
        }
    }

    updateFoundDupesState(foundDups, totalFiles, originalImages);
}

void ThumbsViewer::selectByBrightness(qreal min, qreal max) {
    loadAllThumbs();
    QItemSelection sel;
    for (int row = 0; row < thumbsViewerModel->rowCount(); ++row) {
        QModelIndex idx = thumbsViewerModel->index(row, 0);
        QVariant brightness = thumbsViewerModel->data(idx, BrightnessRole);
        if (brightness.isValid()) {
            qreal val = brightness.toReal();
            if (val >= min && val <= max)
                sel.select(idx, idx);
        }
    }
    selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
}

void ThumbsViewer::loadAllThumbs() {
    QProgressDialog progress(tr("Loading thumbnails..."), tr("Abort"), 0, thumbFileInfoList.count(), this);
    int processed = 0;
    for (int i = 0; i < thumbFileInfoList.count(); ++i) {
        progress.setValue(i);
        if (progress.wasCanceled())
            break;
        if (thumbsViewerModel->item(i)->data(LoadedRole).toBool())
            continue;
        loadThumb(i);

        if (isAbortThumbsLoading) {
            break;
        }

        if (++processed > BATCH_SIZE) {
            QApplication::processEvents();
            processed = 0;
        }
    }
}

void ThumbsViewer::loadThumbsRange() {
    static bool isInProgress = false;
    static int currentRowCount;
    int currThumb;

    if (isInProgress) {
        isAbortThumbsLoading = true;
        QTimer::singleShot(0, this, SLOT(loadThumbsRange()));
        return;
    }

    isInProgress = true;
    currentRowCount = thumbsViewerModel->rowCount();

    int processed = 0;
    for (scrolledForward ? currThumb = thumbsRangeFirst : currThumb = thumbsRangeLast;
         (scrolledForward ? currThumb <= thumbsRangeLast : currThumb >= thumbsRangeFirst);
         scrolledForward ? ++currThumb : --currThumb) {

        if (isAbortThumbsLoading || thumbsViewerModel->rowCount() != currentRowCount || currThumb < 0)
            break;

        if (thumbsViewerModel->item(currThumb)->data(LoadedRole).toBool())
            continue;

        loadThumb(currThumb);

        if (++processed > BATCH_SIZE) {
            QApplication::processEvents();
            processed = 0;
        }
    }

    isInProgress = false;
    isAbortThumbsLoading = false;
}

bool ThumbsViewer::loadThumb(int currThumb) {
    static QSize currentThumbSize;
    QImageReader thumbReader;
    QString imageFileName = thumbsViewerModel->item(currThumb)->data(FileNameRole).toString();
    QImage thumb;
    bool imageReadOk = false;

    thumbReader.setFileName(imageFileName);
    currentThumbSize = thumbReader.size();

    if (currentThumbSize.isValid()) {
        if (currentThumbSize.width() != thumbSize || currentThumbSize.height() != thumbSize) {
            currentThumbSize.scale(QSize(thumbSize, thumbSize), Settings::thumbsLayout == Squares ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
        }

        thumbReader.setScaledSize(currentThumbSize);
        imageReadOk = thumbReader.read(&thumb);
    }

    if (imageReadOk) {
        if (Settings::exifThumbRotationEnabled) {
            imageViewer->rotateByExifRotation(thumb, imageFileName);
            currentThumbSize = thumb.size();
            currentThumbSize.scale(QSize(thumbSize, thumbSize), Settings::thumbsLayout == Squares ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
        }

        if (Settings::thumbsLayout == Squares) {
            const QRect subRect((currentThumbSize.width() - thumbSize) / 2, (currentThumbSize.height() - thumbSize) / 2, thumbSize, thumbSize);
            thumb = thumb.scaled(QSize(thumbSize, thumbSize), Qt::KeepAspectRatioByExpanding).copy(subRect);
        }

        thumbsViewerModel->item(currThumb)->setIcon(QPixmap::fromImage(thumb));
        thumbsViewerModel->item(currThumb)->setData(qGray(thumb.scaled(1, 1).pixel(0, 0)) / 255.0, BrightnessRole);
        thumbsViewerModel->item(currThumb)->setData(true, LoadedRole);
        if (Settings::thumbsLayout == Squares) {
            thumbsViewerModel->item(currThumb)->setSizeHint(QSize(thumbSize, thumbSize));
        }
    } else {
        thumbsViewerModel->item(currThumb)->setIcon(QIcon::fromTheme("image-missing",
                                                                     QIcon(":/images/error_image.png")).pixmap(
                BAD_IMAGE_SIZE, BAD_IMAGE_SIZE));
        currentThumbSize.setHeight(BAD_IMAGE_SIZE);
        currentThumbSize.setWidth(BAD_IMAGE_SIZE);
        return false;
    }
    return true;
}

void ThumbsViewer::addThumb(QString &imageFullPath) {

    metadataCache->loadImageMetadata(imageFullPath);
    if (imageTags->dirFilteringActive && imageTags->isImageFilteredOut(imageFullPath)) {
        return;
    }

    QStandardItem *thumbItem = new QStandardItem();
    QImageReader thumbReader;
    QSize hintSize;
    QSize currThumbSize;
    static QImage thumb;

    if (Settings::thumbsLayout == Squares) {
        hintSize = QSize(thumbSize, thumbSize);
    } else {
        hintSize = QSize(thumbSize, thumbSize + ((int) (QFontMetrics(font()).height() * 1.5)));
    }

    thumbFileInfo = QFileInfo(imageFullPath);
    thumbItem->setData(true, LoadedRole);
    thumbItem->setData(0, SortRole);
    thumbItem->setData(thumbFileInfo.size(), SizeRole);
    thumbItem->setData(thumbFileInfo.lastModified(), TimeRole);
    thumbItem->setData(thumbFileInfo.suffix(), TypeRole);
    thumbItem->setData(thumbFileInfo.filePath(), FileNameRole);
    thumbItem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
    thumbItem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);
    thumbItem->setSizeHint(hintSize);

    thumbReader.setFileName(imageFullPath);
    currThumbSize = thumbReader.size();
    if (currThumbSize.isValid()) {
        if (currThumbSize.width() > thumbSize || currThumbSize.height() > thumbSize) {
            currThumbSize.scale(QSize(thumbSize, thumbSize), Settings::thumbsLayout == Squares ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
        }

        thumbReader.setScaledSize(currThumbSize);
        thumb = thumbReader.read();

        if (Settings::exifThumbRotationEnabled) {
            imageViewer->rotateByExifRotation(thumb, imageFullPath);
            currThumbSize = thumb.size();
            currThumbSize.scale(QSize(thumbSize, thumbSize), Settings::thumbsLayout == Squares ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
        }
        thumbItem->setData(qGray(thumb.scaled(1, 1).pixel(0, 0)) / 255.0, BrightnessRole);

        thumbItem->setIcon(QPixmap::fromImage(thumb));
    } else {
        thumbItem->setIcon(
                QIcon::fromTheme("image-missing", QIcon(":/images/error_image.png")).pixmap(BAD_IMAGE_SIZE,
                                                                                            BAD_IMAGE_SIZE));
        currThumbSize.setHeight(BAD_IMAGE_SIZE);
        currThumbSize.setWidth(BAD_IMAGE_SIZE);
    }

    thumbsViewerModel->appendRow(thumbItem);
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

void ThumbsViewer::setNeedToScroll(bool needToScroll) {
    this->isNeedToScroll = needToScroll;
}

void ThumbsViewer::setImageViewer(ImageViewer *imageViewer) {
    this->imageViewer = imageViewer;
}

