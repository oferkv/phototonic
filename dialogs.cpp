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

void CpMvDialog::exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir)
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

KeyGrabLineEdit::KeyGrabLineEdit(QWidget *parent, QComboBox *combo) : QLineEdit(parent)
{
	setReadOnly(true);
	keysCombo = combo;
}

void KeyGrabLineEdit::keyPressEvent(QKeyEvent *e)
{
	QString keySeqText;
	QString keyText("");
	QString modifierText("");

	if (e->modifiers() & Qt::ShiftModifier)
		modifierText += "Shift+";		
	if (e->modifiers() & Qt::ControlModifier)
		modifierText += "Ctrl+";
	if (e->modifiers() & Qt::AltModifier)
		modifierText += "Alt+";

	if ((e->key() >= Qt::Key_Shift &&  e->key() <= Qt::Key_ScrollLock) || 
			(e->key() >= Qt::Key_Super_L &&  e->key() <= Qt::Key_Direction_R) ||
			e->key() == Qt::Key_AltGr ||
			e->key() < 0) 
		return;

	keyText = QKeySequence(e->key()).toString();
	keySeqText = modifierText + keyText;

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->shortcut().toString() == keySeqText)
		{
			QMessageBox msgBox;
			msgBox.warning(this, "Set shortcut", "Already assigned to \"" + it.key() + "\" action");
			return;
		}
	}
	
	setText(keySeqText);
	GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(keySeqText));
}

void KeyGrabLineEdit::clearShortcut()
{
	clear();
	GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(""));
}

void SettingsDialog::setActionKeyText(const QString &text)
{
	keyLine->setText(GData::actionKeys.value(text)->shortcut().toString());
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("Phototonic Settings");

//	if (parent->isFullScreen())
		resize(600, 480);
//	else
//		resize(600, parent->size().height() - 50);

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
	backgroundColorButton = new QToolButton();
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
	colThumbButton = new QToolButton();
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
	colThumbTextButton = new QToolButton();
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
	QLabel *thumbSpacingLab = new QLabel("Add space between thumbnails: ");
	thumbSpacingSpin = new QSpinBox;
	thumbSpacingSpin->setRange(0, 15);
	thumbSpacingSpin->setValue(GData::thumbSpacing);
	QHBoxLayout *thumbSpacingHbox = new QHBoxLayout;
	thumbSpacingHbox->addWidget(thumbSpacingLab);
	thumbSpacingHbox->addWidget(thumbSpacingSpin);
	thumbSpacingHbox->addStretch(1);

	// Do not enlarge small thumbs
	noSmallThumbCb = new QCheckBox("Do not enlarge thumbnails for images smaller than thumbnail size", this);
	noSmallThumbCb->setChecked(GData::noEnlargeSmallThumb);

	// Thumbnail options
	QGroupBox *thumbOptsGroupBox = new QGroupBox("Thumbnails");
	QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
	thumbsOptsBox->addLayout(thumbSpacingHbox);
	thumbsOptsBox->addWidget(noSmallThumbCb);
	thumbOptsGroupBox->setLayout(thumbsOptsBox);

	// Zoom large images
	QGroupBox *fitLargeGroupBox = new QGroupBox("Fit Large Images");
	fitLargeRadios[0] = new QRadioButton("Disable");
	fitLargeRadios[1] = new QRadioButton("By width and height");
	fitLargeRadios[2] = new QRadioButton("By width");
	fitLargeRadios[3] = new QRadioButton("By height");
	fitLargeRadios[4] = new QRadioButton("Stretch disproportionately");
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
	fitSmallRadios[4] = new QRadioButton("Stretch disproportionately");
	QVBoxLayout *fitSmallVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; i++)
	{
		fitSmallVbox->addWidget(fitSmallRadios[i]);
		fitSmallRadios[i]->setChecked(false);
	}
	fitSmallVbox->addStretch(1);
	fitSmallGroupBox->setLayout(fitSmallVbox);
	fitSmallRadios[GData::zoomInFlags]->setChecked(true);

	// Exit when opening image
	exitCliCb = new QCheckBox("Exit instead of returning to thumbnails, after loading a single image", this);
	exitCliCb->setChecked(GData::exitInsteadOfClose);

	// Save quality
	QLabel *saveQualityLab = new QLabel("Default quality when saving images: ");
	saveQualitySpin = new QSpinBox;
	saveQualitySpin->setRange(0, 100);
	saveQualitySpin->setValue(GData::defaultSaveQuality);
	QHBoxLayout *saveQualityHbox = new QHBoxLayout;
	saveQualityHbox->addWidget(saveQualityLab);
	saveQualityHbox->addWidget(saveQualitySpin);
	saveQualityHbox->addStretch(1);

	// Viewer options
	QVBoxLayout *viewerOptsBox = new QVBoxLayout;
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	viewerOptsBox->addLayout(zoomOptsBox);
	viewerOptsBox->addWidget(exitCliCb);
	viewerOptsBox->addLayout(saveQualityHbox);
	QGroupBox *viewerOptsGrp = new QGroupBox("Viewer");
	viewerOptsGrp->setLayout(viewerOptsBox);

	// Slide show delay
	QLabel *slideDelayLab = new QLabel("Delay between slides in seconds: ");
	slideDelaySpin = new QSpinBox;
	slideDelaySpin->setRange(1, 3600);
	slideDelaySpin->setValue(GData::slideShowDelay);
	QHBoxLayout *slideDelayHbox = new QHBoxLayout;
	slideDelayHbox->addWidget(slideDelayLab);
	slideDelayHbox->addWidget(slideDelaySpin);
	slideDelayHbox->addStretch(1);

	// Slide show random
	slideRandomCb = new QCheckBox("Show random images", this);
	slideRandomCb->setChecked(GData::slideShowRandom);

	// Slide show options
	QVBoxLayout *slideShowVbox = new QVBoxLayout;
	slideShowVbox->addLayout(slideDelayHbox);
	slideShowVbox->addWidget(slideRandomCb);
	QGroupBox *slideShowGbox = new QGroupBox("Slide Show");
	slideShowGbox->setLayout(slideShowVbox);

	// Keyboard shortcuts widgets
	QComboBox *keysCombo = new QComboBox();
	keyLine = new KeyGrabLineEdit(this, keysCombo);
	connect(keysCombo, SIGNAL(activated(const QString &)),
							this, SLOT(setActionKeyText(const QString &)));

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		keysCombo->addItem(it.key());
	}
	keyLine->setText(GData::actionKeys.value(keysCombo->currentText())->shortcut().toString());

	QToolButton *clearShortCutButton = new QToolButton();
	clearShortCutButton->setStyleSheet("QToolButton {padding: 0px;}");
	clearShortCutButton->setIcon(QIcon::fromTheme("edit-delete"));
	connect(clearShortCutButton, SIGNAL(clicked()), keyLine, SLOT(clearShortcut()));

	// Keyboard shortcuts
	QHBoxLayout *keyboardVbox = new QHBoxLayout;
	keyboardVbox->addWidget(keysCombo);
	keyboardVbox->addWidget(keyLine);
	keyboardVbox->addWidget(clearShortCutButton);
	keyboardVbox->addStretch(1);
	QGroupBox *keyboardGbox = new QGroupBox("Set Keyboard Shortcuts");
	keyboardGbox->setLayout(keyboardVbox);

	// General
	QVBoxLayout *optsLayout = new QVBoxLayout;
	optsWidgetArea->setLayout(optsLayout);
	optsLayout->addWidget(viewerOptsGrp);
	optsLayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Expanding));
	optsLayout->addWidget(thumbOptsGroupBox);
	optsLayout->addWidget(colorsGbox);
	optsLayout->addWidget(slideShowGbox);
	optsLayout->addWidget(keyboardGbox);
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
	GData::exitInsteadOfClose = exitCliCb->isChecked();
	GData::defaultSaveQuality = saveQualitySpin->value();
	GData::noEnlargeSmallThumb = noSmallThumbCb->isChecked();
	GData::slideShowDelay = slideDelaySpin->value();
	GData::slideShowRandom = slideRandomCb->isChecked();

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

CropDialog::CropDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle("Cropping");
	resize(340, 180);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton("OK");
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QSlider *topSlide = new QSlider(Qt::Horizontal);
	QSlider *bottomSlide = new QSlider(Qt::Horizontal);
	QSlider *leftSlide = new QSlider(Qt::Horizontal);
	QSlider *rightSlide = new QSlider(Qt::Horizontal);

	topSlide->setTickPosition(QSlider::TicksAbove);
	topSlide->setTickInterval(100);
	bottomSlide->setTickPosition(QSlider::TicksAbove);
	bottomSlide->setTickInterval(100);
	leftSlide->setTickPosition(QSlider::TicksAbove);
	leftSlide->setTickInterval(100);
	rightSlide->setTickPosition(QSlider::TicksAbove);
	rightSlide->setTickInterval(100);

	QLabel *topLab = new QLabel("Top");
	QLabel *leftLab = new QLabel("Left");
	QLabel *rightLab = new QLabel("Right");
	QLabel *bottomLab = new QLabel("Bottom");
	QLabel *topPixLab = new QLabel;
	QLabel *leftPixLab = new QLabel;
	QLabel *rightPixLab = new QLabel;
	QLabel *bottomPixLab = new QLabel;

	QPixmap topPix(":/images/top.png");
	topPixLab->setPixmap(topPix);
	QPixmap leftPix(":/images/first.png");
	leftPixLab->setPixmap(leftPix);
	QPixmap rightPix(":/images/last.png");
	rightPixLab->setPixmap(rightPix);
	QPixmap bottomPix(":/images/bottom.png");
	bottomPixLab->setPixmap(bottomPix);

	topSpin = new QSpinBox;
	bottomSpin = new QSpinBox;
	leftSpin = new QSpinBox;
	rightSpin = new QSpinBox;

	QGridLayout *mainGbox = new QGridLayout;
	mainGbox->setColumnStretch(2, 1);

	mainGbox->addWidget(topPixLab, 0, 0, 1, 1);
	mainGbox->addWidget(topLab, 0, 1, 1, 1);
	mainGbox->addWidget(topSlide, 0, 2, 1, 3);
	mainGbox->addWidget(topSpin, 0, 5, 1, 1);

	mainGbox->addWidget(bottomPixLab, 1, 0, 1, 1);
	mainGbox->addWidget(bottomLab, 1, 1, 1, 1);
	mainGbox->addWidget(bottomSlide, 1, 2, 1, 3);
	mainGbox->addWidget(bottomSpin, 1, 5, 1, 1);

	mainGbox->addWidget(leftPixLab, 2, 0, 1, 1);
	mainGbox->addWidget(leftLab, 2, 1, 1, 1);
	mainGbox->addWidget(leftSlide, 2, 2, 1, 3);
	mainGbox->addWidget(leftSpin, 2, 5, 1, 1);
	
	mainGbox->addWidget(rightPixLab, 3, 0, 1, 1);
	mainGbox->addWidget(rightLab, 3, 1, 1, 1);
	mainGbox->addWidget(rightSlide, 3, 2, 1, 3);
	mainGbox->addWidget(rightSpin, 3, 5, 1, 1);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(mainGbox);
	mainVbox->addStretch(1);	
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	int width = imageView->getImageSize().width();
	int height = imageView->getImageSize().height();
	
	topSpin->setRange(0, height);
	bottomSpin->setRange(0, height);
	leftSpin->setRange(0, width);
	rightSpin->setRange(0, width);
	topSlide->setRange(0, height);
	bottomSlide->setRange(0, height);
	leftSlide->setRange(0, width);
	rightSlide->setRange(0, width);

	connect(topSlide, SIGNAL(valueChanged(int)), topSpin, SLOT(setValue(int)));
	connect(bottomSlide, SIGNAL(valueChanged(int)), bottomSpin, SLOT(setValue(int)));
	connect(leftSlide, SIGNAL(valueChanged(int)), leftSpin, SLOT(setValue(int)));
	connect(rightSlide, SIGNAL(valueChanged(int)), rightSpin, SLOT(setValue(int)));
	connect(topSpin, SIGNAL(valueChanged(int)), topSlide, SLOT(setValue(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), bottomSlide, SLOT(setValue(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), leftSlide, SLOT(setValue(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), rightSlide, SLOT(setValue(int)));

	topSpin->setValue(GData::cropTop);
	bottomSpin->setValue(GData::cropHeight);
	leftSpin->setValue(GData::cropLeft);
	rightSpin->setValue(GData::cropWidth);

	connect(topSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
}

void CropDialog::applyCrop(int)
{
	GData::cropLeft = leftSpin->value();
	GData::cropTop = topSpin->value();
	GData::cropWidth = rightSpin->value();
	GData::cropHeight = bottomSpin->value();
	imageView->refresh();
}

void CropDialog::ok()
{
	accept();
}

