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

#include "Settings.h"

namespace Settings {
    QSettings *appSettings;
    unsigned int layoutMode;
    unsigned int zoomInFlags;
    unsigned int zoomOutFlags;
    QColor backgroundColor;
    QColor thumbsBackgroundColor;
    QColor thumbsTextColor;
    unsigned int thumbSpacing;
    unsigned int thumbPagesReadahead;
    bool exitInsteadOfClose;
    bool wrapImageList;
    bool enableAnimations;
    float imageZoomFactor;
    bool keepZoomFactor;
    int rotation;
    bool keepTransform;
    bool flipH;
    bool flipV;
    int scaledWidth;
    int scaledHeight;
    int defaultSaveQuality;
    int cropLeft;
    int cropTop;
    int cropWidth;
    int cropHeight;
    int cropLeftPercent;
    int cropTopPercent;
    int cropWidthPercent;
    int cropHeightPercent;
    int slideShowDelay;
    bool slideShowRandom;
    bool slideShowActive;
    QMap<QString, QAction *> actionKeys;
    int hueVal;
    int saturationVal;
    int lightnessVal;
    int contrastVal;
    int brightVal;
    int redVal;
    int greenVal;
    int blueVal;
    bool colorsActive;
    bool colorizeEnabled;
    bool rNegateEnabled;
    bool gNegateEnabled;
    bool bNegateEnabled;
    bool hueRedChannel;
    bool hueGreenChannel;
    bool hueBlueChannel;
    bool exifRotationEnabled;
    bool exifThumbRotationEnabled;
    bool includeSubDirectories;
    bool showHiddenFiles;
    bool showViewerToolbar;
    QMap<QString, QString> externalApps;
    QSet<QString> bookmarkPaths;
    QSet<QString> knownTags;
    bool reverseMouseBehavior;
    bool deleteConfirm;
    QModelIndexList copyCutIdxList;
    bool copyOp;
    QStringList copyCutFileList;
    bool isFullScreen;
    int dialogLastX;
    int dialogLastY;
    StartupDir startupDir;
    QString specifiedStartDir;
    bool enableImageInfoFS;
    bool smallToolbarIcons;
    bool hideDockTitlebars;
    bool tagsDockVisible;
    bool fileSystemDockVisible;
    bool bookmarksDockVisible;
    bool imageInfoDockVisible;
    QString currentViewDir;
    QString thumbsBackImage;
}

