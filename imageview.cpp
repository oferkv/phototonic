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

#define ROUND(x) ((int) ((x) + 0.5))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
	mainWindow = parent;
	cursorIsHidden = false;
	moveImageLocked = false;
	mirrorLayout = LayNone;
	imageLabel = new QLabel;
	imageLabel->setScaledContents(true);
	isAnimation = false;
	anim = 0;

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

	GData::hueVal = 0;
	GData::saturationVal = 100;
	GData::lightnessVal = 100;
	GData::hueSatEnabled = false;
	GData::hueRedChannel = true;
	GData::hueGreenChannel = true;
	GData::hueBlueChannel = true;

	GData::dialogLastX = 0;
	GData::dialogLastY = 0;

	newImage = false;
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
	if (busy || (!imageLabel->pixmap() && !anim))
		return;
	busy = true;

	imageLabel->setVisible(false);
	QSize imgSize = isAnimation? anim->currentPixmap().size() : imageLabel->pixmap()->size();

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

long ImageView::getExifOrientation()
{
	Exiv2::Image::AutoPtr exifImage;

	try
	{
		exifImage = Exiv2::ImageFactory::open(currentImageFullPath.toStdString());
	}
	catch (Exiv2::Error &error)
	{
		return 0;
	}

    exifImage->readMetadata();
    Exiv2::ExifData &exifData = exifImage->exifData();
    long orientation;

    if (!exifData.empty())
    {
		try
		{
			orientation = exifData["Exif.Image.Orientation"].value().toLong();
		}
		catch (Exiv2::Error &error)
		{
			return 0;
		}
	}
	return orientation;
}

void ImageView::transform()
{
	if (GData::exifRotationEnabled)
	{
		QTransform trans;
		switch(getExifOrientation())
		{
			case 2:
				displayImage = displayImage.mirrored(true, false);
				break;
 			case 3:
				trans.rotate(180);
				displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
				break;
 			case 4:
				displayImage = displayImage.mirrored(false, true);
				break;
 			case 5:
				trans.rotate(90);
				displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
				displayImage = displayImage.mirrored(true, false);
				break;
 			case 6:
				trans.rotate(90);
				displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
				break;
 			case 7:
				trans.rotate(90);
				displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
				displayImage = displayImage.mirrored(false, true);
				break;
 			case 8:
				trans.rotate(270);
				displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
				break;
		}
	}

	if (GData::rotation)
	{
		QTransform trans;
		trans.rotate(GData::rotation);
		displayImage = displayImage.transformed(trans, Qt::SmoothTransformation);
	}

	if (GData::flipH || GData::flipV)
	{
		displayImage = displayImage.mirrored(GData::flipH, GData::flipV);
	}

	if (GData::cropLeft || GData::cropTop || GData::cropWidth || GData::cropHeight)
	{
		displayImage = displayImage.copy(	
								GData::cropLeft,
								GData::cropTop,
								displayImage.width() - GData::cropWidth - GData::cropLeft,
								displayImage.height() - GData::cropHeight - GData::cropTop);
	}
}

void ImageView::mirror()
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

static inline int bound0_255(int val)
{
	return ((val > 255)? 255 : (val < 0)? 0 : val);
}

static inline int hslValue(double n1, double n2, double hue)
{
	double value;

	if (hue > 255)
		hue -= 255;
	else if (hue < 0)
		hue += 255;

	if (hue < 42.5)
		value = n1 + (n2 - n1) * (hue / 42.5);
	else if (hue < 127.5)
		value = n2;
	else if (hue < 170)
		value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
	else
		value = n1;

	return ROUND(value * 255.0);
}

void rgbToHsl(int r, int g, int b, unsigned char *hue, unsigned char *sat, unsigned char *light)
{
	double h, s, l;
	int    min, max;
	int    delta;

	if (r > g)	
	{
		max = MAX(r, b);
		min = MIN(g, b);
	}
	else
	{
		max = MAX(g, b);
		min = MIN(r, b);
	}

	l = (max + min) / 2.0;

	if (max == min)
	{
		s = 0.0;
		h = 0.0;
	}
	else
	{
		delta = (max - min);

		if (l < 128)
			s = 255 * (double) delta / (double) (max + min);
		else
			s = 255 * (double) delta / (double) (511 - max - min);

		if (r == max)
			h = (g - b) / (double) delta;
		else if (g == max)
			h = 2 + (b - r) / (double) delta;
		else
			h = 4 + (r - g) / (double) delta;

		h = h * 42.5;

		if (h < 0)
			h += 255;
		else if (h > 255)
			h -= 255;
	}

	*hue = ROUND(h);
	*sat = ROUND(s);
	*light  = ROUND(l);
}

void hslToRgb(double h, double s, double l,
					unsigned char *red, unsigned char *green, unsigned char *blue)
{
	if (s == 0)
	{
		/* achromatic case */
		*red = l;
		*green = l;
		*blue = l;
	}
	else
	{
		double m1, m2;

		if (l < 128)
			m2 = (l * (255 + s)) / 65025.0;
		else
			m2 = (l + s - (l * s) / 255.0) / 255.0;

		m1 = (l / 127.5) - m2;

		/* chromatic case  */
		*red = hslValue(m1, m2, h + 85);
		*green = hslValue(m1, m2, h);
		*blue = hslValue(m1, m2, h - 85);
	}
}

void ImageView::colorize()
{
	int y, x;
	unsigned char hr, hg, hb;
	int r, g, b;
	QRgb *line;
	unsigned char h, s, l;
 
	for(y = 0; y < displayImage.height(); ++y)
	{
		line = (QRgb *)displayImage.scanLine(y);
 
		for(x = 0; x < displayImage.width(); ++x)
		{
			rgbToHsl(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), &h, &s, &l);
								
			if (GData::colorizeEnabled)
				h = GData::hueVal;
			else
				h += GData::hueVal;

			s = bound0_255(((s * GData::saturationVal) / 100));
			l = bound0_255(((l * GData::lightnessVal) / 100));

			hslToRgb(h, s, l, &hr, &hg, &hb);

			r = GData::hueRedChannel? hr : qRed(line[x]);
			g = GData::hueGreenChannel? hg : qGreen(line[x]);
			b = GData::hueBlueChannel? hb : qBlue(line[x]);
			
			displayImage.setPixel(x, y, qRgb(r, g, b));
		}
	}
}

void ImageView::refresh()
{
	if (isAnimation)
		return;

	displayImage = origImage;
	transform();
	if (GData::hueSatEnabled)
		colorize();
	if (mirrorLayout)
		mirror();
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

	if (newImage || currentImageFullPath.isEmpty())
	{
		newImage = true;
		currentImageFullPath = "new_image.png";
		origImage.load(":/images/no_image.png");
		displayImage = origImage;
		displayPixmap = QPixmap::fromImage(displayImage);
		imageLabel->setPixmap(displayPixmap);
		mainWindow->setWindowTitle("New image - Phototonic");
		isAnimation = false;
		return;
	}

	imageReader.setFileName(currentImageFullPath);

	isAnimation = GData::enableAnimations? imageReader.supportsAnimation() : false;
	if (isAnimation)
	{
		if (anim)
			delete anim;
		anim = new QMovie(currentImageFullPath);
		imageLabel->setMovie(anim);
		anim->start();
	}
	else
	{
		if (imageReader.size().isValid())
		{
			origImage.load(currentImageFullPath);
			displayImage = origImage;
			transform();
			if (GData::hueSatEnabled)
				colorize();
			if (mirrorLayout)
				mirror();
			displayPixmap = QPixmap::fromImage(displayImage);
		}
		else
			displayPixmap = QIcon::fromTheme("image-missing", QIcon(":/images/error_image.png")).pixmap(128, 128);

		imageLabel->setPixmap(displayPixmap);
	}

	resizeImage();
}

void ImageView::loadImage(QString &imageFileName)
{
	newImage = false;
	tempDisableResize = false;
	currentImageFullPath = imageFileName;

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
		if (!cursorIsHidden)
		{
			QApplication::setOverrideCursor(Qt::BlankCursor);
			cursorIsHidden = true;
		}
	}
}

void ImageView::setCursorHiding(bool hide)
{
	if (hide)
	{
		mouseMovementTimer->start(500);
	}
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

void ImageView::keyMoveEvent(int direction)
{
	int newX = imageLabel->pos().x();
	int newY = imageLabel->pos().y();
	bool needToMove = false;

	switch (direction)
	{
		case MoveLeft:
			newX += 50;
			break;
		case MoveRight:
			newX -= 50;
			break;
		case MoveUp:
			newY += 50;
			break;
		case MoveDown:
			newY -= 50;
			break;
	}

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
	{
		int i;

		switch (direction)
		{
			case MoveLeft:
				for (i = imageLabel->pos().x(); i <= newX; ++i)
					imageLabel->move(newX, newY);
				break;
			case MoveRight:
				for (i = imageLabel->pos().x(); i >= newX; --i)
					imageLabel->move(newX, newY);
				break;
			case MoveUp:
				for (i = imageLabel->pos().y(); i <= newY; ++i)
					imageLabel->move(newX, newY);
				break;
			case MoveDown:
				for (i = imageLabel->pos().y(); i >= newY; --i)
					imageLabel->move(newX, newY);
				break;
		}
	}
}

void ImageView::saveImage()
{
	Exiv2::Image::AutoPtr image;
	bool exifError = false;

	if (newImage)
	{
		saveImageAs();
		return;
	}

	popMessage("Saving...");

	try
	{
		image = Exiv2::ImageFactory::open(currentImageFullPath.toStdString());
		image->readMetadata();
	}
	catch (Exiv2::Error &error)
	{
		exifError = true;
	}

	if (!displayPixmap.save(currentImageFullPath, 0, GData::defaultSaveQuality))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to save image");
		return;
	}

	if (!exifError)
	{
		try
		{
			image->writeMetadata();
		}
		catch (Exiv2::Error &error)
		{
			exifError = true;
		}
	}

	reload();
	popMessage("Image saved");
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

	Exiv2::Image::AutoPtr exifImage;
	Exiv2::Image::AutoPtr newExifImage;
	bool exifError = false;

	QString fileName = QFileDialog::getSaveFileName(this,
		"Save image as",
		currentImageFullPath,
		"Image Files (*.jpg *.jpeg *.jpe *.png *.bmp *.tiff *.tif *.ppm *.xbm *.xpm)");
		
	if (!fileName.isEmpty())
	{
		try
		{
			exifImage = Exiv2::ImageFactory::open(currentImageFullPath.toStdString());
			exifImage->readMetadata();
		}
		catch (Exiv2::Error &error)
		{
			exifError = true;
		}

	
		if (!displayPixmap.save(fileName, 0, GData::defaultSaveQuality))
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to save image");
		}
		else
		{
			if (!exifError)
			{
				try
				{
					newExifImage = Exiv2::ImageFactory::open(fileName.toStdString());
					newExifImage->setMetadata(*exifImage);
					newExifImage->writeMetadata();
				}
				catch (Exiv2::Error &error)
				{
					exifError = true;
				}
			}
		
			popMessage("Image saved");
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
	return QSize(displayImage.width(), displayImage.height());
}

void ImageView::copyImage()
{
	QApplication::clipboard()->setImage(displayImage);
}

void ImageView::pasteImage()
{
	if (isAnimation)
		return;

	if (!QApplication::clipboard()->image().isNull())
	{
		origImage = QApplication::clipboard()->image();
		refresh();
	}
}

void ImageView::hideMessage()
{
	QToolTip::hideText();
}

void ImageView::popMessage(const char *message)
{
	QToolTip::showText(QPoint((mainWindow->pos().x() + 10), (mainWindow->pos().y() + 10)), QString(message));
	QTimer::singleShot(1000, this, SLOT(hideMessage()));
}

