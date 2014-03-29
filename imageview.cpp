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

#include "imageview.h"
#include "thumbview.h"
#include "global.h"

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
	mainWindow = parent;
	cursorIsHidden = false;
	moveImageLocked = false;
	mirrorLayout = LayNone;

	imageLabel = new QLabel;
	imageLabel->setScaledContents(true);
	imageLabel->setFixedSize(0, 0);

	setPalette(QPalette(GData::backgroundColor));

	QHBoxLayout *mainHLayout = new QHBoxLayout();
	mainHLayout->setContentsMargins(0, 0, 0, 0);
	mainHLayout->setSpacing(0);
	mainHLayout->addWidget(imageLabel);

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

	GData::cropLeft = 0;
	GData::cropTop = 0;
	GData::cropWidth = 0;
	GData::cropHeight = 0;
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
	if (busy || !imageLabel->pixmap())
		return;
	busy = true;

	imageLabel->setVisible(false);
	QSize imgSize = imageLabel->pixmap()->size();

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

	imageLabel->setFixedSize(imgSize);
	imageLabel->setVisible(true);
	centerImage(imgSize);
	busy = false;
}

void ImageView::centerImage(QSize &imgSize)
{
	int newX = imageLabel->pos().x();
	int newY = imageLabel->pos().y();

	newX = (size().width() - imgSize.width()) / 2;
	newY = (size().height() - imgSize.height()) / 2;

	if (newX != imageLabel->pos().x() || newY != imageLabel->pos().y())
		imageLabel->move(newX, newY);
}

void ImageView::transform()
{
	if (GData::cropLeft || GData::cropTop || GData::cropWidth || GData::cropHeight)
	{
		displayImage = displayImage.copy(	
								GData::cropLeft,
								GData::cropTop,
								origImage.width() - GData::cropWidth - GData::cropLeft,
								origImage.height() - GData::cropHeight - GData::cropTop);
	}

	if (GData::rotation)
	{
		QTransform trans;
		trans.rotate(GData::rotation);
		displayImage = displayImage.transformed(trans);
	}

	if (GData::flipH || GData::flipV)
	{
		displayImage = displayImage.mirrored(GData::flipH, GData::flipV);
	}

	if (mirrorLayout)
	{	
		switch(mirrorLayout)
		{
			case LayDual:
			{
				mirrorImage = QImage(displayImage.width() * 2, displayImage.height(), QImage::QImage::Format_ARGB32);
			    QPainter painter(&mirrorImage);
			    painter.drawImage(0, 0, displayImage);
		   	    painter.drawImage(displayImage.width(), 0, displayImage.mirrored(true, false));
		   	    break;
	   	    }

	   	    case LayTriple:
	   	    {
				mirrorImage = QImage(displayImage.width() * 3, displayImage.height(), QImage::QImage::Format_ARGB32);
			    QPainter painter(&mirrorImage);
			    painter.drawImage(0, 0, displayImage);
		   	    painter.drawImage(displayImage.width(), 0, displayImage.mirrored(true, false));
   		   	    painter.drawImage(displayImage.width() * 2, 0, displayImage.mirrored(false, false));
		   	    break;
	   	    }

			case LayQuad:
	   	    {
				mirrorImage = QImage(displayImage.width() * 2, displayImage.height() * 2, QImage::QImage::Format_ARGB32);
			    QPainter painter(&mirrorImage);
			    painter.drawImage(0, 0, displayImage);
		   	    painter.drawImage(displayImage.width(), 0, displayImage.mirrored(true, false));
   		   	    painter.drawImage(0, displayImage.height(), displayImage.mirrored(false, true));
   		   	    painter.drawImage(displayImage.width(), displayImage.height(), displayImage.mirrored(true, true));

		   	    break;
	   	    }

			case LayVDual:
			{
				mirrorImage = QImage(displayImage.width(), displayImage.height() * 2, QImage::QImage::Format_ARGB32);
			    QPainter painter(&mirrorImage);
			    painter.drawImage(0, 0, displayImage);
		   	    painter.drawImage(0, displayImage.height(), displayImage.mirrored(false, true));
		   	    break;
	   	    }
		}

		displayImage = mirrorImage;
	}
}

void ImageView::colorizationTests()
{
/*
    QImage newImage = QImage(displayImage.width(), displayImage.height(), QImage::Format_ARGB32);
 
    for(int y = 0; y < newImage.height(); y++)
    {
        QRgb * line = (QRgb *)displayImage.scanLine(y);
 
        for(int x = 0; x < newImage.width(); x++)
        {
    		QColor col = QColor(line[x]);
	        newImage.setPixel(x,y, qRgb(qRed(line[x]), qRed(line[x]), qGreen(line[x])));
        }
     }
 
    displayImage = newImage;
*/
}

void ImageView::refresh()
{
	displayImage = origImage;
	transform();
	displayPixmap = QPixmap::fromImage(displayImage);
	imageLabel->setPixmap(displayPixmap);
	resizeImage();
}

void ImageView::reload()
{
	if (!GData::keepTransform)
	{
		GData::cropLeft = 0;
		GData::cropTop = 0;
		GData::cropWidth = 0;
		GData::cropHeight = 0;
		GData::rotation = 0;
		GData::flipH = false;
		GData::flipV = false;
	}

	imageReader.setFileName(currentImageFullPath);

	if (imageReader.size().isValid())
	{
		origImage.load(currentImageFullPath);
		displayImage = origImage;
		transform();
		displayPixmap = QPixmap::fromImage(displayImage);
	}
	else
		displayPixmap = QIcon::fromTheme("image-missing").pixmap(128, 128);

	imageLabel->setPixmap(displayPixmap);
	resizeImage();
}

void ImageView::loadImage(QString &imagePath, QString imageFileName)
{
	tempDisableResize = false;
	currentImage = imageFileName;
	currentImageFullPath = imagePath + QDir::separator() + currentImage;

	if (!GData::keepZoomFactor)
		GData::imageZoomFactor = 1.0;

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
	layoutX = imageLabel->pos().x();
	layoutY = imageLabel->pos().y();
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
	if (moveImageLocked) 
	{
		int newX = layoutX + (event->pos().x() - mouseX);
		int newY = layoutY + (event->pos().y() - mouseY);
		bool needToMove = false;

		if (imageLabel->size().width() > mainWindow->size().width())
		{
			if (newX > 0)
				newX = 0;
			else if (newX < (mainWindow->size().width() - imageLabel->size().width()))
				newX = (mainWindow->size().width() - imageLabel->size().width());
			needToMove = true;
		}
		else
			newX = layoutX;

		if (imageLabel->size().height() > mainWindow->size().height())
		{
			if (newY > 0)
				newY = 0;
			else if (newY < (mainWindow->size().height() - imageLabel->size().height()))
				newY = (mainWindow->size().height() - imageLabel->size().height());
			needToMove = true;
		}
		else
			newY = layoutY;

		if (needToMove)
			imageLabel->move(newX, newY);
	}
}

void ImageView::saveImage()
{
	if (!displayPixmap.save(currentImageFullPath, 0, GData::defaultSaveQuality))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to save image");
	}
	else
		reload();
}

void ImageView::setCursorOverrides(bool override)
{
	if (override)
	{
		QApplication::setOverrideCursor(Qt::OpenHandCursor);
		if (mainWindow->isFullScreen())
			setCursorHiding(true);
	}
	else
	{
		setCursorHiding(false);
		while (QApplication::overrideCursor())
			QApplication::restoreOverrideCursor();
	}
}

void ImageView::saveImageAs()
{
	setCursorOverrides(false);

	QString fileName = QFileDialog::getSaveFileName(this,
		"Save image as",
		currentImageFullPath,
		"Image Files (*.jpg *.jpeg *.jpe *.png *.bmp *.tiff *.tif *.ppm *.xbm *.xpm)");
	if (!fileName.isEmpty())
	{
		if (!displayPixmap.save(fileName, 0, GData::defaultSaveQuality))
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to save image");
		}
	}

	setCursorOverrides(true);
}

void ImageView::contextMenuEvent(QContextMenuEvent *)
{
	setCursorOverrides(false);
	ImagePopUpMenu->exec(QCursor::pos());
	setCursorOverrides(true);
}

QSize ImageView::getImageSize()
{
	return QSize(origImage.width(), origImage.height());
}

void ImageView::copyImage()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setImage(displayImage);
}

