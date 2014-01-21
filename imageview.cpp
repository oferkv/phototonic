/*
 *  Copyright (C) 2013 Ofer Kashayov - oferkv@live.com
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

#include <QImageReader>
#include <QScrollBar>
#include "imageview.h"
#include "thumbview.h"
#include "global.h"

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
	grid = new QGridLayout();
	grid->setContentsMargins(0,0,0,0);
	grid->setSpacing(0);
	this->setLayout(grid);

	imgLabel1 = new QLabel;
	imgLabel1->setScaledContents(true);
	imgLabel1->setFixedSize(0,0);

	QPalette pal = palette();
	pal.setColor(backgroundRole(), QColor(0,0,0));
	setPalette(pal);

	scrlArea = new QScrollArea;
	scrlArea->setContentsMargins(0,0,0,0);
	scrlArea->setAlignment(Qt::AlignCenter);
	scrlArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->setFrameStyle(0);
	scrlArea->setWidget(imgLabel1);
	scrlArea->setWidgetResizable(true);
	
	grid->addWidget(scrlArea, 0, 0, 0, 0);
}

ImageView::~ImageView()
{

}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
	resizeImage();
}

void ImageView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
	resizeImage();
}


static unsigned int getHeightByWidth(int imgWidth, int imgHeight, int newWidth)
{
	float aspect;
	aspect = (float) imgWidth / (float) newWidth;
	return(imgHeight / aspect);
}

static unsigned int getWidthByHeight(int imgHeight, int imgWidth, int newHeight)
{
	float aspect;
	aspect = (float) imgHeight / (float) newHeight;
	return(imgWidth / aspect);
}

void ImageView::resizeImage()
{
    QSize imgSize = imgLabel1->pixmap()->size();

	if (GData::zoomInFlags)
	{
		switch(GData::zoomInFlags)
		{
			case WidthNHeight:
				if (imgSize.width() < size().width() && imgSize.height() < size().height())
				    imgSize.scale(size(), Qt::KeepAspectRatio);
				break;

			case Width:
				if (imgSize.width() < size().width())
				    imgSize.scale(size().width(), 
				    	getHeightByWidth(imgSize.width(), imgSize.height(), size().width()),
			    		Qt::KeepAspectRatio);
				break;
				
			case Height:
				if (imgSize.height() < size().height())
				    imgSize.scale(getWidthByHeight(imgSize.height(), imgSize.width(), size().height()),
				    	size().height(),
			    		Qt::KeepAspectRatio);
			
				break;

			case Disprop:
				int newWidth = imgSize.width(), newHeight = imgSize.height();
				if (newWidth < size().width())
					newWidth = size().width();
				if (newHeight < size().height())
				    newHeight = size().height();
			    imgSize.scale(newWidth, newHeight, Qt::IgnoreAspectRatio);
				break;

		}
	}

	if (GData::zoomOutFlags)
	{
		switch(GData::zoomOutFlags)
		{
			case WidthNHeight:
				if (imgSize.width() > size().width() || imgSize.height() > size().height())
				    imgSize.scale(size(), Qt::KeepAspectRatio);
				break;

			case Width:
				if (imgSize.width() > size().width())
				    imgSize.scale(size().width(), 
				    	getHeightByWidth(imgSize.width(), imgSize.height(), size().width()),
			    		Qt::KeepAspectRatio);
				break;
				
			case Height:
				if (imgSize.height() > size().height())
				    imgSize.scale(getWidthByHeight(imgSize.height(), imgSize.width(), size().height()),
				    	size().height(),
			    		Qt::KeepAspectRatio);
			
				break;

			case Disprop:
				int newWidth = imgSize.width(), newHeight = imgSize.height();
				if (newWidth > size().width())
					newWidth = size().width();
				if (newHeight > size().height())
				    newHeight = size().height();
			    imgSize.scale(newWidth, newHeight, Qt::IgnoreAspectRatio);
				break;

		}
	}

	imgLabel1->setFixedSize(imgSize);
    scrlArea->horizontalScrollBar()->setValue(scrlArea->horizontalScrollBar()->maximum() / 2);
	scrlArea->verticalScrollBar()->setValue(scrlArea->verticalScrollBar()->maximum() / 2);
}

void ImageView::loadImage(QString &imagePath)
{
	QImageReader thumbReader;
	thumbReader.setFileName(imagePath);
	QPixmap pixmap1;

	if (!thumbReader.size().isValid())
	{
		pixmap1.load(":/images/error_image.png");
	}
	else
	{
		pixmap1.load(imagePath);
	}
	imgLabel1->setPixmap(pixmap1);
}

