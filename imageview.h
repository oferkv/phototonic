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

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QtWidgets>
#include "croprubberband.h"

class ImageView : public QWidget
{
    Q_OBJECT

public:
	bool tempDisableResize;
	int mirrorLayout;
	QString currentImageFullPath;
	QMenu *ImagePopUpMenu;
	QScrollArea *scrlArea;
	QLabel *infoLabel;
	CropRubberBand *cropBand;

	enum ZoomMethods
	{
		Disable = 0,
		WidthNHeight,
		Width,
		Height,
		Disprop
	};

	enum MirrorLayouts
	{
		LayNone = 0,
		LayDual,
		LayTriple,
		LayQuad,
		LayVDual
	};

	enum Movement
	{
		MoveUp = 0,
		MoveDown,
		MoveLeft,
		MoveRight
	};

	ImageView(QWidget *parent = 0);
	void loadImage(QString imageFileName);
	void resizeImage();
	void setCursorHiding(bool hide);
	void refresh();
	void refreshColors();
	void reload();
	int getImageWidthPreCropped();
	int getImageHeightPreCropped();
	bool isNewImage();
	void keyMoveEvent(int direction);
	static void rotateByExifRotation(QImage &image, const QString &imageFullPath);
	void setInfo(QString infoString);
	void setFeedback(QString feedbackString);

public slots:
	void monitorCursorState();
	void saveImage();
	void saveImageAs();
	void copyImage();
	void pasteImage();
	void cropToSelection();

private slots:
	void unsetFeedback();

protected:
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	QWidget *mainWindow;
	QImageReader imageReader;
	QLabel *imageLabel;
	QPixmap displayPixmap;
	QImage origImage;
	QImage displayImage;
	QImage mirrorImage;
	QTimer *mouseMovementTimer;
	QMovie *anim;
	bool newImage;
	bool cursorIsHidden;
	bool moveImageLocked;
	int mouseX;
	int mouseY;
	int layoutX;
	int layoutY;
	bool isAnimation;
	QLabel *feedbackLabel;
	QPoint cropOrigin;

	void setMouseMoveData(bool lockMove, int lMouseX, int lMouseY);
	void centerImage(QSize &imgSize);
	void transform();
	void mirror();
	void colorize();
};

#endif // IMAGEVIEW_H

