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
#include <QTimer>
#include "imageview.h"
#include "thumbview.h"
#include "global.h"

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
	mainWindow = parent;
	cursorIsHidden = false;
	moveImageLocked = false;
	layoutMode = LaySingle;

	for (int i = 0; i < NLayouts; i++)
	{
		imageLabel[i] = new QLabel;
		imageLabel[i]->setScaledContents(true);
		imageLabel[i]->setFixedSize(0, 0);
	}

	setPalette(QPalette(GData::backgroundColor));

	QHBoxLayout *mainHLayout = new QHBoxLayout();
	mainHLayout->setContentsMargins(0, 0, 0, 0);
	mainHLayout->setSpacing(0);
	mainHLayout->addWidget(imageLabel[0]);

	scrlArea = new QScrollArea;
	scrlArea->setContentsMargins(0, 0, 0, 0);
	scrlArea->setAlignment(Qt::AlignCenter);
	scrlArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->verticalScrollBar()->blockSignals(true);
	scrlArea->horizontalScrollBar()->blockSignals(true);
	scrlArea->setFrameStyle(0);
	scrlArea->setLayout(mainHLayout);
	scrlArea->setWidgetResizable(true);

	QVBoxLayout *mainVLayout = new QVBoxLayout;
	mainVLayout->setContentsMargins(0, 0, 0, 0);
	mainVLayout->addWidget(scrlArea);
	this->setLayout(mainVLayout);
	
	mouseMovementTimer = new QTimer(this);
	connect(mouseMovementTimer, SIGNAL(timeout()), this, SLOT(monitorCursorState()));
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
	static bool busy = false;
	if (busy || !imageLabel[0]->pixmap())
		return;
	busy = true;

	imageLabel[0]->setVisible(false);
	QSize imgSize = imageLabel[0]->pixmap()->size();

	if (tempDisableResize)
		imgSize.scale(calcZoom(imgSize.width()), calcZoom(imgSize.height()), Qt::KeepAspectRatio);
	else
	{
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
	}

	imageLabel[0]->setFixedSize(imgSize);
	imageLabel[0]->setVisible(true);
	centerImage(imgSize);
	busy = false;
}

void ImageView::centerImage(QSize &imgSize)
{
	int newX = imageLabel[0]->pos().x();
	int newY = imageLabel[0]->pos().y();

	newX = (size().width() - imgSize.width()) / 2;
	newY = (size().height() - imgSize.height()) / 2;

	if (newX !=imageLabel[0]->pos().x() || newY !=imageLabel[0]->pos().y())
		imageLabel[0]->move(newX, newY);
}

void ImageView::transform()
{
	if (GData::rotation)
	{
		QTransform trans;
		trans.rotate(GData::rotation);
		images[0] = images[0].transformed(trans);
	}

	if (GData::flipH || GData::flipV)
	{
		images[0] = images[0].mirrored(GData::flipH, GData::flipV);
	}
}

void ImageView::reload()
{
	imageReader.setFileName(currentImageFullPath);

	if (imageReader.size().isValid())
	{
		images[0].load(currentImageFullPath);
		transform();
		pixmaps[0] = QPixmap::fromImage(images[0]);
	}
	else
		pixmaps[0].load(":/images/error_image.png");

	imageLabel[0]->setPixmap(pixmaps[0]);
	resizeImage();
}

void ImageView::loadImage(QString &imagePath, QString imageFileName)
{
	tempDisableResize = false;
	currentImage = imageFileName;
	currentImageFullPath = imagePath + QDir::separator() + currentImage;

	if (!GData::keepZoomFactor)
		GData::imageZoomFactor = 1.0;

	if (!GData::keepTransform)
	{
		GData::rotation = 0;
		GData::flipH = false;
		GData::flipV = false;
	}

	reload();
}

void ImageView::monitorCursorState()
{
	static QPoint lastPos;
	
	if (QCursor::pos() != lastPos)
	{
		lastPos = QCursor::pos();
		if (cursorIsHidden) 
		{
			QApplication::restoreOverrideCursor();
			cursorIsHidden = false;
		}
	}
	else
	{
		if (!cursorIsHidden && !moveImageLocked)
		{
			QApplication::setOverrideCursor(Qt::BlankCursor);
			cursorIsHidden = true;
		}
	}
}

void ImageView::setCursorHiding(bool hide)
{
	if (hide)
		mouseMovementTimer->start(500);
	else
	{
		mouseMovementTimer->stop();
		if (cursorIsHidden) 
		{
			QApplication::restoreOverrideCursor();
			cursorIsHidden = false;
		}
	}
}

void ImageView::setMouseMoveData(bool lockMove, int lMouseX, int lMouseY)
{
	moveImageLocked = lockMove;
	mouseX = lMouseX;
	mouseY = lMouseY;
	layoutX = imageLabel[0]->pos().x();
	layoutY = imageLabel[0]->pos().y();
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
	if (moveImageLocked) 
	{
		int newX = layoutX + (event->pos().x() - mouseX);
		int newY = layoutY + (event->pos().y() - mouseY);
		bool needToMove = false;

		if (imageLabel[0]->size().width() > mainWindow->size().width())
		{
			if (newX > 0)
				newX = 0;
			else if (newX < (mainWindow->size().width() - imageLabel[0]->size().width()))
				newX = (mainWindow->size().width() - imageLabel[0]->size().width());
			needToMove = true;
		}
		else
			newX = layoutX;

		if (imageLabel[0]->size().height() > mainWindow->size().height())
		{
			if (newY > 0)
				newY = 0;
			else if (newY < (mainWindow->size().height() - imageLabel[0]->size().height()))
				newY = (mainWindow->size().height() - imageLabel[0]->size().height());
			needToMove = true;
		}
		else
			newY = layoutY;

		if (needToMove)
			imageLabel[0]->move(newX, newY);
	}
}

