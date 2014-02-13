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

#include "dialogs.h"
#include "global.h"

CpMvDialog::CpMvDialog(QWidget *parent) : QDialog(parent)
{
	// setAttribute(Qt::WA_DeleteOnClose);
	abortOp = false;

    opLabel = new QLabel("");
    
    cancelButton = new QPushButton("Cancel");
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(abort()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(opLabel);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(buttonsLayout, Qt::AlignRight);
    setLayout(mainLayout);
}

void CpMvDialog::abort()
{
	abortOp = true;
}

static QString autoRename(QString &destDir, QString &currFile)
{
	int extSep = currFile.lastIndexOf(".");
	QString nameOnly = currFile.left(extSep);
	QString extOnly = currFile.right(currFile.size() - extSep - 1);
	QString newFile;

	int idx = 1;

	do
	{
		newFile = QString(nameOnly + "_copy_%1." + extOnly).arg(idx);
		idx++;
	} 
	while (idx && (QFile::exists(destDir + QDir::separator() + newFile)));

	return newFile;
}

static int cpMvFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir)
{
	int res;
	
	if (isCopy)
		res = QFile::copy(srcPath, dstPath);
	else
		res = QFile::rename(srcPath, dstPath);
		
	if (!res && QFile::exists(dstPath))
	{
		QString newName = autoRename(dstDir, srcFile);
		if (isCopy)
			res = QFile::copy(srcPath, dstDir + QDir::separator() + newName);
		else
			res = QFile::rename(srcPath, dstDir + QDir::separator() + newName);
	}

	return res;
}

void CpMvDialog::exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir, bool thumbViewBusy)
{
	int res = 0;
	nfiles = 0;

	show();

	if (pasteInCurrDir)
	{
		int tn = 0;
		for (tn = 0; tn < GData::copyCutFileList.size(); tn++)
		{
			QString sourceFile = GData::copyCutFileList[tn];
			QFileInfo fInfo = QFileInfo(sourceFile);
			QString currFile = fInfo.fileName();
			QString destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? "Copying ":"Moving ") + sourceFile + " to " + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				nfiles++;
				break;
			}
			nfiles++;

			// The name is wrong here because cpMvFile might change the dest name, fix this when implementing interactive copy dialog
			if (!thumbViewBusy)
				thumbView->addNewThumb(destFile);
		}
	}
	else
	{
		QList<int> rowList;
		int tn = 0;
		for (tn = GData::copyCutIdxList.size() - 1; tn >= 0 ; tn--)
		{
			QString currFile = thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->data(thumbView->FileNameRole).toString();
			QString sourceFile = thumbView->currentViewDir + QDir::separator() + currFile;
			QString destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? "Copying ":"Moving ") + sourceFile + " to " + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				nfiles++;
				break;
			}

			nfiles++;
			rowList.append(GData::copyCutIdxList[tn].row());
		}

		if (!GData::copyOp)
		{
			qSort(rowList);
			for (int t = rowList.size() - 1; t >= 0; t--)
				thumbView->thumbViewModel->removeRow(rowList.at(t));
		}
	}

	close();	
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("Phototonic Settings");
	resize(640, 480);

	QWidget* optsWidgetArea = new QWidget(this);
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(optsWidgetArea);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShadow(QFrame::Plain);
	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton("OK");
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	QPushButton *closeButton = new QPushButton("Cancel");
	closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(abort()));
	buttonsHbox->addWidget(closeButton, 1, Qt::AlignRight);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(scrollArea);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// imageView background color
	QLabel *backgroundColorLab = new QLabel("Viewer background color: ");
	backgroundColorButton = new QPushButton("");
	QHBoxLayout *bgColBox = new QHBoxLayout;
	bgColBox->addWidget(backgroundColorLab);
	bgColBox->addWidget(backgroundColorButton);
	bgColBox->addStretch(1);
	connect(backgroundColorButton, SIGNAL(clicked()), this, SLOT(pickColor()));
	backgroundColorButton->setPalette(QPalette(GData::backgroundColor));
	backgroundColorButton->setAutoFillBackground(true);
	bgColor = GData::backgroundColor;

	// thumbView background color
	QLabel *bgThumbTxtLab = new QLabel("Thumbnails background color: ");
	colThumbButton = new QPushButton("");
	QHBoxLayout *bgThumbColBox = new QHBoxLayout;
	bgThumbColBox->addWidget(bgThumbTxtLab);
	bgThumbColBox->addWidget(colThumbButton);
	bgThumbColBox->addStretch(1);
	connect(colThumbButton, SIGNAL(clicked()), this, SLOT(pickThumbsColor()));
	colThumbButton->setPalette(QPalette(GData::thumbsBackgroundColor));
	colThumbButton->setAutoFillBackground(true);
	thumbBgColor = GData::thumbsBackgroundColor;

	// thumbView text color
	QLabel *txtThumbTxtLab = new QLabel("Thumbnail text color: ");
	colThumbTextButton = new QPushButton("");
	QHBoxLayout *txtThumbColBox = new QHBoxLayout;
	txtThumbColBox->addWidget(txtThumbTxtLab);
	txtThumbColBox->addWidget(colThumbTextButton);
	txtThumbColBox->addStretch(1);
	connect(colThumbTextButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
	colThumbTextButton->setPalette(QPalette(GData::thumbsTextColor));
	colThumbTextButton->setAutoFillBackground(true);
	thumbTextColor = GData::thumbsTextColor;

	// Colors
	QVBoxLayout *colorsVbox = new QVBoxLayout;
	colorsVbox->addLayout(bgColBox);
	colorsVbox->addLayout(bgThumbColBox);
	colorsVbox->addLayout(txtThumbColBox);
	QGroupBox *colorsGbox = new QGroupBox("Colors");
	colorsGbox->setLayout(colorsVbox);

	// Thumbnail spacing
	QLabel *thumbSpacingLab = new QLabel("Thumbnail spacing: ");
	thumbSpacingSpin = new QSpinBox;
	thumbSpacingSpin->setRange(0, 15);
	thumbSpacingSpin->setValue(GData::thumbSpacing);
	QHBoxLayout *thumbSpacingHbox = new QHBoxLayout;
	thumbSpacingHbox->addWidget(thumbSpacingLab);
	thumbSpacingHbox->addWidget(thumbSpacingSpin);
	thumbSpacingHbox->addStretch(1);

	// Thumbnail options
	QGroupBox *thumbOptsGroupBox = new QGroupBox("Thumbnail Options");
	QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
	thumbsOptsBox->addLayout(thumbSpacingHbox);
	thumbOptsGroupBox->setLayout(thumbsOptsBox);

	// Zoom large images
	QGroupBox *fitLargeGroupBox = new QGroupBox("Fit Large Images");
	fitLargeRadios[0] = new QRadioButton("Disable");
	fitLargeRadios[1] = new QRadioButton("By width and height");
	fitLargeRadios[2] = new QRadioButton("By width");
	fitLargeRadios[3] = new QRadioButton("By height");
	fitLargeRadios[4] = new QRadioButton("Stretch disproportionly");
	QVBoxLayout *fitLargeVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; i++)
	{
		fitLargeVbox->addWidget(fitLargeRadios[i]);
		fitLargeRadios[i]->setChecked(false);
	}
	fitLargeVbox->addStretch(1);
	fitLargeGroupBox->setLayout(fitLargeVbox);
	fitLargeRadios[GData::zoomOutFlags]->setChecked(true);
 	
	// Zoom small images
	QGroupBox *fitSmallGroupBox = new QGroupBox("Fit Small Images");
	fitSmallRadios[0] = new QRadioButton("Disable");
	fitSmallRadios[1] = new QRadioButton("By width and height");
	fitSmallRadios[2] = new QRadioButton("By width");
	fitSmallRadios[3] = new QRadioButton("By height");
	fitSmallRadios[4] = new QRadioButton("Stretch disproportionly");
	QVBoxLayout *fitSmallVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; i++)
	{
		fitSmallVbox->addWidget(fitSmallRadios[i]);
		fitSmallRadios[i]->setChecked(false);
	}
	fitSmallVbox->addStretch(1);
	fitSmallGroupBox->setLayout(fitSmallVbox);
	fitSmallRadios[GData::zoomInFlags]->setChecked(true);

	// Zoom options
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	QGroupBox *zoomOpts = new QGroupBox("Zoom Options");
	zoomOpts->setLayout(zoomOptsBox);

	// General
	QVBoxLayout *optsLayout = new QVBoxLayout;
	optsWidgetArea->setLayout(optsLayout);
	optsLayout->addWidget(zoomOpts);
	optsLayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Expanding));
	optsLayout->addWidget(thumbOptsGroupBox);
	optsLayout->addWidget(colorsGbox);
	optsLayout->addStretch(1);
}

void SettingsDialog::saveSettings()
{
	for (int i = 0; i < nZoomRadios; i++)
	{
		if (fitLargeRadios[i]->isChecked())
		{
			GData::zoomOutFlags = i;
			GData::appSettings->setValue("zoomOutFlags", (int)GData::zoomOutFlags);
			break;
		}
	}

	for (int i = 0; i < nZoomRadios; i++)
	{
		if (fitSmallRadios[i]->isChecked())
		{
			GData::zoomInFlags = i;
			GData::appSettings->setValue("zoomInFlags", (int)GData::zoomInFlags);
			break;
		}
	}

	GData::backgroundColor = bgColor;
	GData::thumbsBackgroundColor = thumbBgColor;
	GData::thumbsTextColor = thumbTextColor;
	GData::thumbSpacing = thumbSpacingSpin->value();

	accept();
}

void SettingsDialog::abort()
{
	reject();
}

void SettingsDialog::pickColor()
{
	QColor userColor = QColorDialog::getColor(GData::backgroundColor, this);
    if (userColor.isValid())
    {	
        backgroundColorButton->setPalette(QPalette(userColor));
        bgColor = userColor;
    }
}

void SettingsDialog::pickThumbsColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsBackgroundColor, this);
    if (userColor.isValid())
    {	
        colThumbButton->setPalette(QPalette(userColor));
        thumbBgColor = userColor;
    }
}

void SettingsDialog::pickThumbsTextColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsTextColor, this);
    if (userColor.isValid())
    {	
        colThumbTextButton->setPalette(QPalette(userColor));
        thumbTextColor = userColor;
    }
}

