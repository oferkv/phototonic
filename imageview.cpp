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
	layoutMode = LayDual;

	for (int i = 0; i < NLayouts; i++)
	{
		qDebug() << "doing " << i;
		imgLabels[i] = new QLabel;
		imgLabels[i]->setScaledContents(true);
		imgLabels[i]->setFixedSize(0, 0);
	}

	setPalette(QPalette(GData::backgroundColor));

	grid = new QGridLayout();
	grid->setContentsMargins(0,0,0,0);
	grid->setSpacing(0);
	grid->addWidget(imgLabels[0], 0, 0, Qt::AlignLeft);
	grid->addWidget(imgLabels[1], 0, 1, Qt::AlignLeft);
	grid->addWidget(imgLabels[2], 0, 2);
	grid->addWidget(imgLabels[3], 1, 0);
	grid->addWidget(imgLabels[4], 1, 1);

	scrlArea = new QScrollArea;
	scrlArea->setContentsMargins(0,0,0,0);
	scrlArea->setAlignment(Qt::AlignCenter);
	scrlArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrlArea->verticalScrollBar()->blockSignals(true);
	scrlArea->horizontalScrollBar()->blockSignals(true);
	scrlArea->setFrameStyle(0);
	scrlArea->setLayout(grid);
	scrlArea->setWidgetResizable(true);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0,0,0,0);
	mainLayout->addWidget(scrlArea);
		
	this->setLayout(mainLayout);
	
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
	QSize imgSize = imgLabels[0]->pixmap()->size();

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

	for (int i = 0; i <= layoutMode; i++)
		imgLabels[i]->setFixedSize(imgSize);
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
		for (int i = 0; i <= layoutMode; i++)
			pixmaps[0].load(":/images/error_image.png");
	}
	else
	{
		for (int i = 0; i <= layoutMode; i++)
		{
			pixmaps[0].load(imageFullPath);
			imgLabels[i]->setPixmap(pixmaps[0]);
		}
	}

	if (!GData::keepZoomFactor)
		GData::imageZoomFactor = 1.0;
	
	resizeImage();
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
	layoutX = imgLabels[0]->pos().x();
	layoutY = imgLabels[0]->pos().y();
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
	if (moveImageLocked) 
	{
		int newX = layoutX + (event->pos().x() - mouseX);
		int newY = layoutY + (event->pos().y() - mouseY);
		bool needToMove = false;

		if (imgLabels[0]->size().width() > mainWindow->size().width())
		{
			if (newX > 0)
				newX = 0;
			else if (newX < (mainWindow->size().width() - imgLabels[0]->size().width()))
				newX = (mainWindow->size().width() - imgLabels[0]->size().width());
			needToMove = true;
		}
		else
			newX = layoutX;

		if (imgLabels[0]->size().height() > mainWindow->size().height())
		{
			if (newY > 0)
				newY = 0;
			else if (newY < (mainWindow->size().height() - imgLabels[0]->size().height()))
				newY = (mainWindow->size().height() - imgLabels[0]->size().height());
			needToMove = true;
		}
		else
			newY = layoutY;

		if (needToMove)
			imgLabels[0]->move(newX, newY);
	}
}

