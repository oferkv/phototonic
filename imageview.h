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

#include <QApplication>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include <QImageReader>
#include <QScrollBar>
#include <QMouseEvent>
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <QMenu>
#include <QPainter>
#include <QClipboard>

class ImageView : public QWidget
{
    Q_OBJECT

public:
	QString currentImage;
	bool tempDisableResize;
	int mirrorLayout;

	QMenu *ImagePopUpMenu;

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

    ImageView(QWidget *parent = 0);
	void loadImage(QString &imagePath, QString imageFileName);
	void resizeImage();
	void setMouseMoveData(bool lockMove, int lMouseX, int lMouseY);
	void setCursorHiding(bool hide);
	void refresh();
	void reload();
	void setCursorOverrides(bool override);
	QSize getImageSize();

public slots:
	void monitorCursorState();
	void saveImage();
	void saveImageAs();
	void copyImage();

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

private:
	QWidget *mainWindow;
	QScrollArea *scrlArea;
	QImageReader imageReader;
	QLabel *imageLabel;
	QPixmap displayPixmap;
	QImage origImage;
	QImage displayImage;
	QTimer *mouseMovementTimer;

	bool cursorIsHidden;
	bool moveImageLocked;
	int mouseX;
	int mouseY;
	int layoutX;
	int layoutY;
	QString currentImageFullPath;

	void centerImage(QSize &imgSize);
	void transform();
};

#endif // IMAGEVIEW_H

