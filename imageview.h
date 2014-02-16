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

#include <QLabel>
#include <QGridLayout>
#include <QDebug>
#include <QScrollArea>
#include <QImageReader>
#include <QScrollBar>
#include <QMouseEvent>

class ImageView : public QWidget
{
    Q_OBJECT

public:
    ImageView(QWidget *parent = 0);
	void loadImage(QString &imagePath, QString imageFileName);
	void resizeImage();
	QString currentImage;
	void setMouseMoveData(bool lockMove, int lMouseX, int lMouseY);

	enum ZoomMethods
	{
		Disable = 0,
		WidthNHeight,
		Width,
		Height,
		Disprop
	};

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private:
	QWidget *mainWindow;
	QGridLayout *grid;
	QLabel *imgLabel1;
	QLabel *imgLabel2;
	QScrollArea *scrlArea;
	QImageReader imageReader;
	QPixmap pixmap0_0;
	bool moveImageLocked;
	int mouseX;
	int mouseY;
	int layoutX;
	int layoutY;

	void mouseMoveEvent(QMouseEvent *event);
};

#endif // IMAGEVIEW_H

