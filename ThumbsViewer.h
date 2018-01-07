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

#ifndef THUMBVIEW_H
#define THUMBVIEW_H

#include <QtWidgets>
#include <exiv2/exiv2.hpp>
#include "Settings.h"
#include "FileSystemTree.h"
#include "Bookmarks.h"
#include "InfoViewer.h"
#include "ImageViewer.h"
#include "Tags.h"
#include "MetadataCache.h"

#define BAD_IMG_SZ    64

class ImageTags;

struct DuplicateImage {
    QString filePath;
    int duplicates;
};

class ThumbsViewer : public QListView {
Q_OBJECT

public:
    enum UserRoles {
        FileNameRole = Qt::UserRole + 1,
        SortRole,
        LoadedRole
    };

    ThumbsViewer(QWidget *parent, MetadataCache *metadataCache);

    void loadPrepare();

    void load();

    void loadDuplicates();

    void setThumbColors();

    bool setCurrentIndexByName(QString &fileName);

    bool setCurrentIndexByRow(int row);

    void setCurrentRow(int row);

    void setImageViewerWindowTitle();

    void setNeedScroll(bool needScroll);

    void selectCurrentIndex();

    void addThumb(QString &imageFullPath);

    void abort();

    void selectThumbByRow(int row);

    int getNextRow();

    int getPrevRow();

    int getLastRow();

    int getRandomRow();

    int getCurrentRow();

    QStringList getSelectedThumbsList();

    QString getSingleSelectionFilename();

    void setImageView(ImageViewer *imageViewer);

    InfoView *infoView;
    ImageTags *imageTags;
    QDir *thumbsDir;
    QStringList *fileFilters;
    QStandardItemModel *thumbsViewerModel;
    QDir::SortFlags thumbsSortFlags;
    int thumbSize;
    QString filterStr;
    bool busy;

protected:
    void startDrag(Qt::DropActions);

    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);

private:
    void initThumbs();

    void findDupes(bool resetCounters);

    void updateFoundDupesState(int duplicates, int filesScanned, int originalImages);

    int getFirstVisibleThumb();

    int getLastVisibleThumb();

    void updateThumbsCount();

    void updateImageInfoViewer(QString imageFullPath);

    QFileInfo thumbFileInfo;
    QFileInfoList thumbFileInfoList;
    QImage emptyImg;
    QModelIndex currentIndex;
    QImageReader imageInfoReader;
    QWidget *mainWindow;
    QMap<QString, DuplicateImage> dupImageHashes;
    MetadataCache *metadataCache;
    ImageViewer *imageViewer;
    bool abortOp;
    bool isNeedScroll;
    int currentRow;
    bool scrolledForward;
    int thumbsRangeFirst;
    int thumbsRangeLast;

signals:

    void setStatus(QString state);

    void showBusy(bool busy);

public slots:

    void loadVisibleThumbs(int scrollBarValue = 0);

    void handleSelectionChanged(const QItemSelection &selection);

    void invertSelection();

private slots:

    void loadThumbsRange();
};

#endif // THUMBVIEW_H

