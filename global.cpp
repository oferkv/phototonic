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

#include "global.h"

namespace GData
{
	// app settings
	QSettings *appSettings;
	unsigned int zoomInFlags;
	unsigned int zoomOutFlags;
	QColor backgroundColor;
	QColor thumbsBackgroundColor;
	QColor thumbsTextColor;
	unsigned int thumbsLayout;
	unsigned int thumbSpacing;
	bool exitInsteadOfClose;
	float imageZoomFactor;
	bool keepZoomFactor;
	int rotation;
	bool keepTransform;

	// app data
	QModelIndexList copyCutIdxList;
	bool copyOp;
	QStringList copyCutFileList;
	bool isFullScreen;
}

