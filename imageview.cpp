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

#include <QApplication>
#include "imageview.h"
#include "thumbview.h"
#include "global.h"

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
	mainWindow = parent;
	grid = new QGridLayout();
	grid->setContentsMargins(0,0,0,0);
	grid->setSpacing(0);
	this->setLayout(grid);

	imgLabel1 = new QLabel;
	imgLabel1->setScaledContents(true);
	imgLabel1->setFixedSize(0,0);

	setPalette(QPalette(GData::backgroundColor));

	scrlArea = new QScrollArea;
	scrlArea->setContentsMargins(0,0,0,0);
	scrlArea->setAlignment(Qt::AlignCenter);
	scrlArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->verticalScrollBar()->blockSignals(true);
	scrlArea->horizontalScrollBar()->blockSignals(true);
	scrlArea->setFrameStyle(0);
	scrlArea->setWidget(imgLabel1);
	scrlArea->setWidgetResizable(true);
	
	grid->addWidget(scrlArea, 0, 0, 0, 0);
	moveImageLocked = false;
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

static inline int calcZoom(int size)
{
	return size * GData::imageZoomFactor;
}

void ImageView::resizeImage()
{
	QSize imgSize = imgLabel1->pixmap()->size();

	switch(GData::zoomInFlags)
	{
		case Disable:
			if (imgSize.width() < size().width() && imgSize.height() < size().height())
			    imgSize.scale(calcZoom(imgSize.width()), calcZoom(imgSize.height()), Qt::KeepAspectRatio);
		    break;
			
		case WidthNHeight:
			if (imgSize.width() < size().width() && imgSize.height() < size().height())
			    imgSize.scale(calcZoom(size().width()), calcZoom(size().height()), Qt::KeepAspectRatio);
			break;

		case Width:
			if (imgSize.width() < size().width())
			    imgSize.scale(calcZoom(size().width()), 
			    	calcZoom(getHeightByWidth(imgSize.width(), imgSize.height(), size().width())),
		    		Qt::KeepAspectRatio);
			break;
			
		case Height:
			if (imgSize.height() < size().height())
			    imgSize.scale(calcZoom(getWidthByHeight(imgSize.height(), imgSize.width(), size().height())),
			    	calcZoom(size().height()), Qt::KeepAspectRatio);
			break;

		case Disprop:
			int newWidth = imgSize.width(), newHeight = imgSize.height();
			if (newWidth < size().width())
				newWidth = size().width();
			if (newHeight < size().height())
			    newHeight = size().height();
		    imgSize.scale(calcZoom(newWidth), calcZoom(newHeight), Qt::IgnoreAspectRatio);
			break;
	}

	switch(GData::zoomOutFlags)
	{
		case Disable:
			if (imgSize.width() >= size().width() || imgSize.height() >= size().height())
				imgSize.scale(calcZoom(imgSize.width()), calcZoom(imgSize.height()), Qt::KeepAspectRatio);
			break;
	
		case WidthNHeight:
			if (imgSize.width() >= size().width() || imgSize.height() >= size().height())
				imgSize.scale(calcZoom(size().width()), calcZoom(size().height()), Qt::KeepAspectRatio);
			break;

		case Width:
			if (imgSize.width() > size().width())
			    imgSize.scale(calcZoom(size().width()), 
					calcZoom(getHeightByWidth(imgSize.width(), imgSize.height(), size().width())), Qt::KeepAspectRatio);
			break;
			
		case Height:
			if (imgSize.height() > size().height())
			    imgSize.scale(calcZoom(getWidthByHeight(imgSize.height(), imgSize.width(), size().height())),
			    	calcZoom(size().height()), Qt::KeepAspectRatio);
		
			break;

		case Disprop:
			int newWidth = imgSize.width(), newHeight = imgSize.height();
			if (newWidth > size().width())
				newWidth = size().width();
			if (newHeight > size().height())
			    newHeight = size().height();
		    imgSize.scale(calcZoom(newWidth), calcZoom(newHeight), Qt::IgnoreAspectRatio);
			break;
	}

	imgLabel1->setFixedSize(imgSize);
	scrlArea->horizontalScrollBar()->setValue(scrlArea->horizontalScrollBar()->maximum() / 2);
	scrlArea->verticalScrollBar()->setValue(scrlArea->verticalScrollBar()->maximum() / 2);
}

void ImageView::loadImage(QString &imagePath, QString imageFileName)
{
	currentImage = imageFileName;
	QString imageFullPath = imagePath + QDir::separator() + currentImage;

	imageReader.setFileName(imageFullPath);
	if (!imageReader.size().isValid())
	{
		pixmap0_0.load(":/images/error_image.png");
	}
	else
	{
		pixmap0_0.load(imageFullPath);
	}
	imgLabel1->setPixmap(pixmap0_0);
	if (!GData::keepZoomFactor)
		GData::imageZoomFactor = 1.0;
	
	resizeImage();
}

void ImageView::setMouseMoveData(bool lockMove, int lMouseX, int lMouseY)
{
	moveImageLocked = lockMove;
	mouseX = lMouseX;
	mouseY = lMouseY;
	layoutX = imgLabel1->pos().x();
	layoutY = imgLabel1->pos().y();
}

// pic from cli not centering image!

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
	if (moveImageLocked) 
	{
		bool needCorrection = false;
		int newX = layoutX + (event->pos().x() - mouseX);
		int newY = layoutY + (event->pos().y() - mouseY);

		if (imgLabel1->size().width() > mainWindow->size().width())
		{
			if (imgLabel1->pos().x() < (mainWindow->size().width() - imgLabel1->size().width()))
			{
				newX = mainWindow->size().width() - imgLabel1->size().width();
				needCorrection = true;
			}

			if (imgLabel1->pos().x() > 0)
			{
				newX = 0;
				needCorrection = true;
			}
		}

		if (imgLabel1->size().height() > mainWindow->size().height())
		{
			if (imgLabel1->pos().y() < (mainWindow->size().height() - imgLabel1->size().height()))
			{
				newY = mainWindow->size().height() - imgLabel1->size().height();
				needCorrection = true;
			}


			if (imgLabel1->pos().y() > 0)
			{
				newY = 0;
				needCorrection = true;
			}
		}

		imgLabel1->move(newX, newY);

		if (needCorrection)
			setMouseMoveData(true, event->pos().x(), event->pos().y());
	}
}

