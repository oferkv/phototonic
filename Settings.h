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

#ifndef SETTINGS_H
#define SETTINGS_H

#define THUMB_SIZE_MIN 25
#define THUMB_SIZE_MAX 450

#include <QSettings>
#include <QModelIndexList>
#include <QStringList>
#include <QColor>
#include <QAction>
#include <QSet>

namespace Settings {

    enum StartupDir {
        RememberLastDir = 0,
        DefaultDir,
        SpecifiedDir
    };

    extern const char optionThumbsSortFlags[];
    extern const char optionThumbsZoomLevel[];
    extern const char optionFullScreenMode[];
    extern const char optionViewerBackgroundColor[];
    extern const char optionThumbsBackgroundColor[];
    extern const char optionThumbsTextColor[];
    extern const char optionThumbsPagesReadCount[];
    extern const char optionViewerZoomOutFlags[];
    extern const char optionViewerZoomInFlags[];
    extern const char optionShowImageName[];
    extern const char optionEnableAnimations[];
    extern const char optionWrapImageList[];
    extern const char optionExifRotationEnabled[];
    extern const char optionExifThumbRotationEnabled[];
    extern const char optionReverseMouseBehavior[];
    extern const char optionDeleteConfirm[];
    extern const char optionShowHiddenFiles[];
    extern const char optionImageZoomFactor[];
    extern const char optionShouldMaximize[];
    extern const char optionDefaultSaveQuality[];
    extern const char optionSlideShowDelay[];
    extern const char optionSlideShowRandom[];
    extern const char optionEditToolBarVisible[];
    extern const char optionGoToolBarVisible[];
    extern const char optionViewToolBarVisible[];
    extern const char optionImageToolBarVisible[];
    extern const char optionFileSystemDockVisible[];
    extern const char optionBookmarksDockVisible[];
    extern const char optionImagePreviewDockVisible[];
    extern const char optionTagsDockVisible[];
    extern const char optionImageInfoDockVisible[];
    extern const char optionSmallToolbarIcons[];
    extern const char optionHideDockTitlebars[];
    extern const char optionStartupDir[];
    extern const char optionSpecifiedStartDir[];
    extern const char optionThumbsBackgroundImage[];
    extern const char optionShowViewerToolbar[];
    extern const char optionLastDir[];
    extern const char optionGeometry[];
    extern const char optionWindowState[];
    extern const char optionShortcuts[];
    extern const char optionExternalApps[];
    extern const char optionCopyMoveToPaths[];
    extern const char optionKnownTags[];
    extern const char optionSetWindowIcon[];

    extern QSettings *appSettings;
    extern unsigned int layoutMode;
    extern unsigned int zoomInFlags;
    extern unsigned int zoomOutFlags;
    extern QColor viewerBackgroundColor;
    extern QColor thumbsBackgroundColor;
    extern QColor thumbsTextColor;
    extern unsigned int thumbsPagesReadCount;
    extern bool wrapImageList;
    extern bool enableAnimations;
    extern float imageZoomFactor;
    extern bool keepZoomFactor;
    extern qreal rotation;
    extern bool keepTransform;
    extern bool flipH;
    extern bool flipV;
    extern int scaledWidth;
    extern int scaledHeight;
    extern int defaultSaveQuality;
    extern int cropLeft;
    extern int cropTop;
    extern int cropWidth;
    extern int cropHeight;
    extern int cropLeftPercent;
    extern int cropTopPercent;
    extern int cropWidthPercent;
    extern int cropHeightPercent;
    extern int slideShowDelay;
    extern bool slideShowRandom;
    extern bool slideShowActive;
    extern QMap<QString, QAction *> actionKeys;
    extern int hueVal;
    extern int saturationVal;
    extern int lightnessVal;
    extern int contrastVal;
    extern int brightVal;
    extern int redVal;
    extern int greenVal;
    extern int blueVal;
    extern bool colorsActive;
    extern bool colorizeEnabled;
    extern bool rNegateEnabled;
    extern bool gNegateEnabled;
    extern bool bNegateEnabled;
    extern bool hueRedChannel;
    extern bool hueGreenChannel;
    extern bool hueBlueChannel;
    extern bool exifRotationEnabled;
    extern bool exifThumbRotationEnabled;
    extern bool includeSubDirectories;
    extern bool showHiddenFiles;
    extern bool showViewerToolbar;
    extern QMap<QString, QString> externalApps;
    extern QSet<QString> bookmarkPaths;
    extern QSet<QString> knownTags;
    extern bool reverseMouseBehavior;
    extern bool deleteConfirm;
    extern QModelIndexList copyCutIndexList;
    extern bool isCopyOperation;
    extern QStringList copyCutFileList;
    extern bool isFullScreen;
    extern int dialogLastX;
    extern int dialogLastY;
    extern StartupDir startupDir;
    extern QString specifiedStartDir;
    extern bool showImageName;
    extern bool smallToolbarIcons;
    extern bool hideDockTitlebars;
    extern bool fileSystemDockVisible;
    extern bool bookmarksDockVisible;
    extern bool imagePreviewDockVisible;
    extern bool tagsDockVisible;
    extern bool imageInfoDockVisible;
    extern QString currentDirectory;
    extern QString thumbsBackgroundImage;
    extern QStringList filesList;
    extern bool isFileListLoaded;
    extern bool setWindowIcon;
}

#endif // SETTINGS_H

