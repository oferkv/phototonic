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

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QDebug>
#include <QModelIndex>
#include <QScrollArea>

class ImageView : public QWidget
{
    Q_OBJECT

public:
    ImageView(QWidget *parent = 0);
    ~ImageView();
	void loadImage(QString &imagePath);
	void resizeImage();

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
	QGridLayout *grid;
	QLabel *imgLabel1;
	QLabel *imgLabel2;
	QScrollArea *scrlArea;
};

#endif // IMAGEVIEW_H

