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

#ifndef THUMBS_VIEWER_H
#define THUMBS_VIEWER_H

#include <QtWidgets>
#include <exiv2/exiv2.hpp>
#include "Settings.h"
#include "FileSystemTree.h"
#include "Bookmarks.h"
#include "InfoViewer.h"
#include "Tags.h"
#include "MetadataCache.h"
#include "ImagePreview.h"

class Phototonic;

class ImageViewer;

#define BAD_IMAGE_SIZE 64
#define WINDOW_ICON_SIZE 48

class ImageTags;

class ThumbsViewer : public QListView {
Q_OBJECT

public:
    enum UserRoles {
        FileNameRole = Qt::UserRole + 1,
        SortRole,
        LoadedRole,
        BrightnessRole
    };

    ThumbsViewer(QWidget *parent, MetadataCache *metadataCache);

    void loadPrepare();

    void applyFilter();

    void reLoad();

    void loadFileList();

    void loadSubDirectories();

    void setThumbColors();

    bool setCurrentIndexByName(QString &fileName);

    bool setCurrentIndexByRow(int row);

    void setCurrentRow(int row);

    void setImageViewerWindowTitle();

    void setNeedToScroll(bool needToScroll);

    void selectCurrentIndex();

    void addThumb(QString &imageFullPath);

    void abort();

    void selectThumbByRow(int row);

    void selectByBrightness(qreal min, qreal max);

    int getNextRow();

    int getPrevRow();

    int getLastRow();

    int getRandomRow();

    int getCurrentRow();

    QStringList getSelectedThumbsList();

    QString getSingleSelectionFilename();

    void setImageViewer(ImageViewer *imageViewer);

    InfoView *infoView;
    ImagePreview *imagePreview;
    ImageTags *imageTags;
    QDir *thumbsDir;
    QStringList *fileFilters;
    QStandardItemModel *thumbsViewerModel;
    QDir::SortFlags thumbsSortFlags;
    int thumbSize;
    QString filterString;
    bool isBusy;

protected:
    void startDrag(Qt::DropActions);

    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);

private:
    void initThumbs();

    bool loadThumb(int row);

    int getFirstVisibleThumb();

    int getLastVisibleThumb();

    void updateThumbsCount();

    void updateImageInfoViewer(int row);

    QFileInfo thumbFileInfo;
    QFileInfoList thumbFileInfoList;
    QImage emptyImg;
    QModelIndex currentIndex;
    Phototonic *phototonic;
    MetadataCache *metadataCache;
    ImageViewer *imageViewer;
    bool isAbortThumbsLoading;
    bool isNeedToScroll;
    int currentRow;
    bool scrolledForward;
    int thumbsRangeFirst;
    int thumbsRangeLast;

public slots:

    void loadVisibleThumbs(int scrollBarValue = 0);

    void onSelectionChanged(const QItemSelection &selection);

    void invertSelection();

private slots:

    void loadThumbsRange();

    void loadAllThumbs();
};

#endif // THUMBS_VIEWER_H

