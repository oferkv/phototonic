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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QSettings>
#include <QModelIndexList>
#include <QStringList>
#include <QColor>
#include <QAction>

namespace GData
{
	// app settings
	extern QSettings *appSettings;
	extern unsigned int zoomInFlags;
	extern unsigned int zoomOutFlags;
	extern QColor backgroundColor;
	extern QColor thumbsBackgroundColor;
	extern QColor thumbsTextColor;
	extern unsigned int thumbsLayout;
	extern unsigned int thumbSpacing;
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
	extern int defaultSaveQuality;
	extern int cropLeft;
	extern int cropTop;
	extern int cropWidth;
	extern int cropHeight;
	extern bool noEnlargeSmallThumb;
	extern int slideShowDelay;
	extern bool slideShowRandom;
	extern bool slideShowActive;
	extern QMap<QString, QAction *> actionKeys;
	extern bool hueSatEnabled;
	extern int hueVal;
	extern int saturationVal;
	extern int lightnessVal;
	extern bool colorizeEnabled;
	extern bool hueRedChannel;
	extern bool hueGreenChannel;
	extern bool hueBlueChannel;
	extern bool exifRotationEnabled;
	extern bool includeSubFolders;
	extern bool showHiddenFiles;
	extern QMap<QString, QString> externalApps;

	// app data
	extern QModelIndexList copyCutIdxList;
	extern bool copyOp;
	extern QStringList copyCutFileList;
	extern bool isFullScreen;
	extern int dialogLastX;
	extern int dialogLastY;
}

#endif // GLOBAL_H

