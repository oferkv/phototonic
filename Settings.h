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

#include <QSettings>
#include <QModelIndexList>
#include <QStringList>
#include <QColor>
#include <QAction>
#include <QSet>

namespace Settings {

    enum StartupDir {
        DefaultDir = 0,
        RememberLastDir,
        SpecifiedDir
    };

    extern const char optionThumbsSortFlags[];
    extern const char optionThumbsZoomLevel[];
    extern const char optionFullScreenMode[];

    extern QSettings *appSettings;
    extern unsigned int layoutMode;
    extern unsigned int zoomInFlags;
    extern unsigned int zoomOutFlags;
    extern QColor backgroundColor;
    extern QColor thumbsBackgroundColor;
    extern QColor thumbsTextColor;
    extern unsigned int thumbPagesReadahead;
    extern bool exitInsteadOfClose;
    extern bool wrapImageList;
    extern bool enableAnimations;
    extern float imageZoomFactor;
    extern bool keepZoomFactor;
    extern int rotation;
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
    extern QModelIndexList copyCutIdxList;
    extern bool copyOp;
    extern QStringList copyCutFileList;
    extern bool isFullScreen;
    extern int dialogLastX;
    extern int dialogLastY;
    extern StartupDir startupDir;
    extern QString specifiedStartDir;
    extern bool enableImageInfoFS;
    extern bool smallToolbarIcons;
    extern bool hideDockTitlebars;
    extern bool fileSystemDockVisible;
    extern bool bookmarksDockVisible;
    extern bool tagsDockVisible;
    extern bool imageInfoDockVisible;
    extern QString thumbsBackImage;
    extern QString currentViewDir;
}

#endif // SETTINGS_H

