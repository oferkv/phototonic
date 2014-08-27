/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
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

#include "preview.h"
#include "imageview.h"
#include "global.h"

ImagePreview::ImagePreview(QWidget *parent) : QWidget(parent)
{
	imageLabel = new QLabel;
	imageLabel->setScaledContents(true);

	clear();

	QHBoxLayout *mainHLayout = new QHBoxLayout();
	mainHLayout->setContentsMargins(0, 0, 0, 0);
	mainHLayout->setSpacing(0);
	mainHLayout->addWidget(imageLabel);

	QScrollArea *scrlArea = new QScrollArea;
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
	setPalette(QPalette(GData::thumbsBackgroundColor));
}

void ImagePreview::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	resizeImage();
}

void ImagePreview::resizeImage()
{
	QSize imgSize = imageLabel->pixmap()->size();

	if (imgSize.width() >= size().width() || imgSize.height() >= size().height())
		imgSize.scale(size().width(), size().height(), Qt::KeepAspectRatio);

	imageLabel->setFixedSize(imgSize);
}

void ImagePreview::load(QString &imagePath)
{
	if (!isVisible())
		return;
		
	if (prevImage.load(imagePath))
	{
		if (GData::exifRotationEnabled)
			ImageView::rotateByExifRotation(prevImage, imagePath);
	
		prevPixmap = QPixmap::fromImage(prevImage);
	}
	else 
	{
		prevPixmap = QIcon::fromTheme("image-missing", 
								QIcon(":/images/error_image.png")).pixmap(128, 128);
	}

	imageLabel->setPixmap(prevPixmap);
	resizeImage();
}

void ImagePreview::clear()
{
	prevImage.load(":/images/no_image.png");
	prevPixmap = QPixmap::fromImage(prevImage);
	imageLabel->setPixmap(prevPixmap);
}

