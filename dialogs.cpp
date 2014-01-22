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

#include <QtGui>
#include "dialogs.h"
#include "global.h"

CpMvDialog::CpMvDialog(QWidget *parent) : QDialog(parent)
{
	// setAttribute(Qt::WA_DeleteOnClose);
	abortOp = false;

    opLabel = new QLabel("");
    
    cancelButton = new QPushButton(tr("Cancel"));
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

void CpMvDialog::exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir)
{
	int res = 0;
	nfiles = 0;

	show();

	if (!pasteInCurrDir)
	{
		QList<int> rowList;
		int tn = 0;
		for (tn = GData::copyCutIdxList.size() - 1; tn >= 0 ; tn--)
		{
			QString currFile = thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->text();
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
	else
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
		}
	}

	close();	
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("Phototonic Settings");
	resize(640, 480);
	QVBoxLayout *optsLayout = new QVBoxLayout;
	QWidget* optsWidgetArea = new QWidget(this);
	optsWidgetArea->setLayout(optsLayout);

	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(optsWidgetArea);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShadow(QFrame::Plain);

	QHBoxLayout *okCancelLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	QPushButton *closeButton = new QPushButton(tr("Cancel"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
   	connect(closeButton, SIGNAL(clicked()), this, SLOT(abort()));
	okCancelLayout->addWidget(closeButton, 1, Qt::AlignRight);
	okCancelLayout->addWidget(okButton, 0, Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(scrollArea);
	mainLayout->addLayout(okCancelLayout);
	setLayout(mainLayout);

	// Background color
	QGroupBox *colorsGroupBox = new QGroupBox(tr("Colors"));
	QLabel *bgTxtLab = new QLabel("Viewer background color:");
	colButton = new QPushButton("");
	QHBoxLayout *bgColBox = new QHBoxLayout;
	bgColBox->addWidget(bgTxtLab);
	bgColBox->addWidget(colButton);
	bgColBox->addStretch(1);
	colorsGroupBox->setLayout(bgColBox);
	connect(colButton, SIGNAL(clicked()), this, SLOT(pickColor()));
	colButton->setPalette(QPalette(	GData::backgroundColor));
	colButton->setAutoFillBackground(true);

	// Zoom options
	QGroupBox *zoomOpts = new QGroupBox(tr("Zoom Options"));
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOpts->setLayout(zoomOptsBox);

	// Zoom large images
	QGroupBox *fitLargeGroupBox = new QGroupBox(tr("Fit Large Images"));
	fitLargeRadios[0] = new QRadioButton(tr("Disable"));
	fitLargeRadios[1] = new QRadioButton(tr("By width and height"));
	fitLargeRadios[2] = new QRadioButton(tr("By width"));
	fitLargeRadios[3] = new QRadioButton(tr("By height"));
	fitLargeRadios[4] = new QRadioButton(tr("Stretch disproportionly"));
	QVBoxLayout *fitLargeVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; i++)
	{
		fitLargeVbox->addWidget(fitLargeRadios[i]);
		fitLargeRadios[i]->setChecked(false);
	}
	fitLargeVbox->addStretch(1);
	fitLargeGroupBox->setLayout(fitLargeVbox);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	fitLargeRadios[GData::zoomOutFlags]->setChecked(true);
 	
	// Zoom small images
	QGroupBox *fitSmallGroupBox = new QGroupBox(tr("Fit Small Images"));
	fitSmallRadios[0] = new QRadioButton(tr("Disable"));
	fitSmallRadios[1] = new QRadioButton(tr("By width and height"));
	fitSmallRadios[2] = new QRadioButton(tr("By width"));
	fitSmallRadios[3] = new QRadioButton(tr("By height"));
	fitSmallRadios[4] = new QRadioButton(tr("Stretch disproportionly"));
		
	QVBoxLayout *fitSmallVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; i++)
	{
		fitSmallVbox->addWidget(fitSmallRadios[i]);
		fitSmallRadios[i]->setChecked(false);
	}
 	fitSmallVbox->addStretch(1);
	fitSmallGroupBox->setLayout(fitSmallVbox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	fitSmallRadios[GData::zoomInFlags]->setChecked(true);
 	
	optsLayout->addWidget(zoomOpts);
	optsLayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Expanding));
	optsLayout->addWidget(colorsGroupBox);
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

	close();
}

void SettingsDialog::abort()
{
	close();
}

void SettingsDialog::pickColor()
{
	QColor userColor = QColorDialog::getColor(GData::backgroundColor, this);
    if (userColor.isValid())
    {	
        colButton->setPalette(QPalette(userColor));
        bgColor = userColor;
    }
}
