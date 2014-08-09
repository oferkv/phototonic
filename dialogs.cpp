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

#include "dialogs.h"
#include "global.h"

CpMvDialog::CpMvDialog(QWidget *parent) : QDialog(parent)
{
	abortOp = false;

    opLabel = new QLabel("");
    
    cancelButton = new QPushButton(tr("Cancel"));
   	cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
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
		++idx;
	} 
	while (idx && (QFile::exists(destDir + QDir::separator() + newFile)));

	return newFile;
}

int cpMvFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir)
{
	int res;
	
	if (isCopy)
		res = QFile::copy(srcPath, dstPath);
	else
		res = QFile::rename(srcPath, dstPath);
		
	if (!res && QFile::exists(dstPath))
	{
		QString newName = autoRename(dstDir, srcFile);
		QString newDestPath = dstDir + QDir::separator() + newName;
		
		if (isCopy)
			res = QFile::copy(srcPath, newDestPath);
		else
			res = QFile::rename(srcPath, newDestPath);
		dstPath = newDestPath;
	}

	return res;
}

void CpMvDialog::exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir)
{
	int res = 0;
	QString sourceFile;
	QFileInfo fileInfo;
	QString currFile;
	QString destFile;

	show();

	if (pasteInCurrDir)
	{
		int tn = 0;
		for (tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			sourceFile = GData::copyCutFileList[tn];
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying "):tr("Moving ")) + sourceFile + tr(" to ") + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				break;
			}
			else 
			{
				GData::copyCutFileList[tn] = destFile;
			}
		}
	}
	else
	{
		QList<int> rowList;
		int tn = 0;
		for (tn = GData::copyCutIdxList.size() - 1; tn >= 0 ; --tn)
		{
			sourceFile = thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->
																data(thumbView->FileNameRole).toString();
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying ") : 
												tr("Moving ")) + sourceFile + tr(" to ") + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				break;
			}

			rowList.append(GData::copyCutIdxList[tn].row());
		}

		if (!GData::copyOp)
		{
			qSort(rowList);
			for (int t = rowList.size() - 1; t >= 0; --t)
				thumbView->thumbViewModel->removeRow(rowList.at(t));
		}
	}

	close();	
}

KeyGrabLineEdit::KeyGrabLineEdit(QWidget *parent, QComboBox *combo) : QLineEdit(parent)
{
	keysCombo = combo;
	setClearButtonEnabled(true);
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

	if (	(e->key() >= Qt::Key_Shift &&  e->key() <= Qt::Key_ScrollLock)
		|| 	(e->key() >= Qt::Key_Super_L &&  e->key() <= Qt::Key_Direction_R)
		||	e->key() == Qt::Key_AltGr
		||	e->key() < 0) 
	{
		return;
	}

	keyText = QKeySequence(e->key()).toString();
	keySeqText = modifierText + keyText;

	if (e->modifiers() & Qt::AltModifier && (e->key() > Qt::Key_0 &&  e->key() <= Qt::Key_Colon))
	{
		QMessageBox msgBox;
		msgBox.warning(this, tr("Set shortcut"), keySeqText +
											tr(" is reserved for shortcuts to external applications"));
		return;
	}

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->shortcut().toString() == keySeqText)
		{
			QMessageBox msgBox;
			msgBox.warning(this, tr("Set shortcut"), keySeqText + tr(" is already assigned to \"")
																		+ it.key() + tr("\" action"));
			return;
		}
	}
	
	setText(keySeqText);
	GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(keySeqText));
}

void KeyGrabLineEdit::clearShortcut()
{
	if (text() == "")
		GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(""));
}

void SettingsDialog::setActionKeyText(const QString &text)
{
	keyLine->setText(GData::actionKeys.value(text)->shortcut().toString());
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Preferences"));
	setWindowIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/phototonic.png")));

	int height = parent->size().height() - 50;
	if (height > 800)
		height = 800;
	resize(600, height);

	QWidget *optsWidgetArea = new QWidget(this);
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(optsWidgetArea);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShadow(QFrame::Plain);
	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
   	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	QPushButton *closeButton = new QPushButton(tr("Cancel"));
   	closeButton->setIcon(QIcon::fromTheme("dialog-cancel"));
	closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(abort()));
	buttonsHbox->addWidget(closeButton, 1, Qt::AlignRight);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(scrollArea);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// imageView background color
	QLabel *backgroundColorLab = new QLabel(tr("Background color: "));
	backgroundColorButton = new QToolButton();
	backgroundColorButton->setFixedSize(48, 24);
	QHBoxLayout *bgColBox = new QHBoxLayout;
	bgColBox->addWidget(backgroundColorLab);
	bgColBox->addWidget(backgroundColorButton);
	bgColBox->addStretch(1);
	connect(backgroundColorButton, SIGNAL(clicked()), this, SLOT(pickColor()));
	setButtonBgColor(GData::backgroundColor, backgroundColorButton);
	backgroundColorButton->setAutoFillBackground(true);
	bgColor = GData::backgroundColor;

	// thumbView background color
	QLabel *bgThumbTxtLab = new QLabel(tr("Background color: "));
	colThumbButton = new QToolButton();
	colThumbButton->setFixedSize(48, 24);
	QHBoxLayout *bgThumbColBox = new QHBoxLayout;
	bgThumbColBox->addWidget(bgThumbTxtLab);
	bgThumbColBox->addWidget(colThumbButton);
	connect(colThumbButton, SIGNAL(clicked()), this, SLOT(pickThumbsColor()));
	setButtonBgColor(GData::thumbsBackgroundColor, colThumbButton);
	colThumbButton->setAutoFillBackground(true);
	thumbBgColor = GData::thumbsBackgroundColor;

	// thumbView text color
	QLabel *txtThumbTxtLab = new QLabel(tr("\tLabel color: "));
	colThumbTextButton = new QToolButton();
	colThumbTextButton->setFixedSize(48, 24);
	bgThumbColBox->addWidget(txtThumbTxtLab);
	bgThumbColBox->addWidget(colThumbTextButton);
	bgThumbColBox->addStretch(1);
	connect(colThumbTextButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
	setButtonBgColor(GData::thumbsTextColor, colThumbTextButton);
	colThumbTextButton->setAutoFillBackground(true);
	thumbTextColor = GData::thumbsTextColor;

	// Thumbnail spacing
	QLabel *thumbSpacingLab = new QLabel(tr("Add space between thumbnails: "));
	thumbSpacingSpin = new QSpinBox;
	thumbSpacingSpin->setRange(0, 15);
	thumbSpacingSpin->setValue(GData::thumbSpacing);
	QHBoxLayout *thumbSpacingHbox = new QHBoxLayout;
	thumbSpacingHbox->addWidget(thumbSpacingLab);
	thumbSpacingHbox->addWidget(thumbSpacingSpin);
	thumbSpacingHbox->addStretch(1);

	// Do not enlarge small thumbs
	noSmallThumbCb = new 
				QCheckBox(tr("Show original size of images smaller than the thumbnail size"), this);
	noSmallThumbCb->setChecked(GData::noEnlargeSmallThumb);

	// Thumbnail pages to read ahead
	QLabel *thumbPagesLab = new QLabel(tr("Number of thumbnail pages to read ahead: "));
	thumbPagesSpin = new QSpinBox;
	thumbPagesSpin->setRange(1, 10);
	thumbPagesSpin->setValue(GData::thumbPagesReadahead);
	QHBoxLayout *thumbPagesHbox = new QHBoxLayout;
	thumbPagesHbox->addWidget(thumbPagesLab);
	thumbPagesHbox->addWidget(thumbPagesSpin);
	thumbPagesHbox->addStretch(1);

	// Thumbnail options
	QGroupBox *thumbOptsGroupBox = new QGroupBox(tr("Thumbnails"));
	QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
	thumbsOptsBox->addLayout(thumbSpacingHbox);
	thumbsOptsBox->addWidget(noSmallThumbCb);
	thumbsOptsBox->addLayout(bgThumbColBox);
	thumbsOptsBox->addLayout(thumbPagesHbox);
	thumbOptsGroupBox->setLayout(thumbsOptsBox);

	// Zoom large images
	QGroupBox *fitLargeGroupBox = new QGroupBox(tr("Fit Large Images"));
	fitLargeRadios[0] = new QRadioButton(tr("Disable"));
	fitLargeRadios[1] = new QRadioButton(tr("By width and height"));
	fitLargeRadios[2] = new QRadioButton(tr("By width"));
	fitLargeRadios[3] = new QRadioButton(tr("By height"));
	fitLargeRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
	QVBoxLayout *fitLargeVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; ++i)
	{
		fitLargeVbox->addWidget(fitLargeRadios[i]);
		fitLargeRadios[i]->setChecked(false);
	}
	fitLargeVbox->addStretch(1);
	fitLargeGroupBox->setLayout(fitLargeVbox);
	fitLargeRadios[GData::zoomOutFlags]->setChecked(true);
 	
	// Zoom small images
	QGroupBox *fitSmallGroupBox = new QGroupBox(tr("Fit Small Images"));
	fitSmallRadios[0] = new QRadioButton(tr("Disable"));
	fitSmallRadios[1] = new QRadioButton(tr("By width and height"));
	fitSmallRadios[2] = new QRadioButton(tr("By width"));
	fitSmallRadios[3] = new QRadioButton(tr("By height"));
	fitSmallRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
	QVBoxLayout *fitSmallVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; ++i)
	{
		fitSmallVbox->addWidget(fitSmallRadios[i]);
		fitSmallRadios[i]->setChecked(false);
	}
	fitSmallVbox->addStretch(1);
	fitSmallGroupBox->setLayout(fitSmallVbox);
	fitSmallRadios[GData::zoomInFlags]->setChecked(true);

	// Exit when opening image
	exitCliCb = new 
				QCheckBox(tr("Exit instead of closing, when image is loaded from command line"), this);
	exitCliCb->setChecked(GData::exitInsteadOfClose);

	// Exit when opening image
	wrapListCb = new QCheckBox(tr("Wrap image list when reaching last or first image"), this);
	wrapListCb->setChecked(GData::wrapImageList);

	// Save quality
	QLabel *saveQualityLab = new QLabel(tr("Default quality when saving images: "));
	saveQualitySpin = new QSpinBox;
	saveQualitySpin->setRange(0, 100);
	saveQualitySpin->setValue(GData::defaultSaveQuality);
	QHBoxLayout *saveQualityHbox = new QHBoxLayout;
	saveQualityHbox->addWidget(saveQualityLab);
	saveQualityHbox->addWidget(saveQualitySpin);
	saveQualityHbox->addStretch(1);

	// Enable animations
	enableAnimCb = new QCheckBox(tr("Enable GIF animation"), this);
	enableAnimCb->setChecked(GData::enableAnimations);

	// Enable Exif
	enableExifCb = new QCheckBox(tr("Rotate according to Exif orientation"), this);
	enableExifCb->setChecked(GData::exifRotationEnabled);

	// Image Info
	imageInfoCb = new QCheckBox(tr("Show image file name in full screen mode"), this);
	imageInfoCb->setChecked(GData::enableImageInfoFS);

	// Startup directory
	QGroupBox *startupDirGroupBox = new QGroupBox(tr("Startup folder"));
	startupDirRadios[GData::defaultDir] = new QRadioButton(tr("Default, or specified by command line argument"));
	startupDirRadios[GData::rememberLastDir] = new QRadioButton(tr("Remember last"));
	startupDirRadios[GData::specifiedDir] = new QRadioButton(tr("Specify:"));
	
	startupDirEdit = new QLineEdit;
	startupDirEdit->setClearButtonEnabled(true);
	startupDirEdit->setMinimumWidth(300);
	startupDirEdit->setMaximumWidth(400);

	QToolButton *chooseStartupDirButton = new QToolButton();
	chooseStartupDirButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	chooseStartupDirButton->setFixedSize(26, 26);
	chooseStartupDirButton->setIconSize(QSize(16, 16));
	connect(chooseStartupDirButton, SIGNAL(clicked()), this, SLOT(pickStartupDir()));
	
	QHBoxLayout *startupDirEditBox = new QHBoxLayout;
	startupDirEditBox->addWidget(startupDirRadios[2]);
	startupDirEditBox->addWidget(startupDirEdit);
	startupDirEditBox->addWidget(chooseStartupDirButton);
	startupDirEditBox->addStretch(1);

	QVBoxLayout *startupDirVbox = new QVBoxLayout;
	for (int i = 0; i < 2; ++i)
	{
		startupDirVbox->addWidget(startupDirRadios[i]);
		startupDirRadios[i]->setChecked(false);
	}
	startupDirVbox->addLayout(startupDirEditBox);
	startupDirVbox->addStretch(1);
	startupDirGroupBox->setLayout(startupDirVbox);

	if (GData::startupDir == GData::specifiedDir)
		startupDirRadios[GData::specifiedDir]->setChecked(true);
	else if (GData::startupDir == GData::rememberLastDir)
		startupDirRadios[GData::rememberLastDir]->setChecked(true);
	else
		startupDirRadios[GData::defaultDir]->setChecked(true);
	startupDirEdit->setText(GData::specifiedStartDir);

	// Viewer options
	QVBoxLayout *viewerOptsBox = new QVBoxLayout;
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	zoomOptsBox->addStretch(1);
	viewerOptsBox->addLayout(zoomOptsBox);
	viewerOptsBox->addLayout(bgColBox);
	viewerOptsBox->addWidget(wrapListCb);
	viewerOptsBox->addLayout(saveQualityHbox);
	viewerOptsBox->addWidget(enableAnimCb);
	viewerOptsBox->addWidget(enableExifCb);
	viewerOptsBox->addWidget(imageInfoCb);
	viewerOptsBox->addWidget(exitCliCb);
	QGroupBox *viewerOptsGrp = new QGroupBox(tr("Viewer"));
	viewerOptsGrp->setLayout(viewerOptsBox);

	// Slide show delay
	QLabel *slideDelayLab = new QLabel(tr("Delay between slides in seconds: "));
	slideDelaySpin = new QDoubleSpinBox;
	slideDelaySpin->setRange(0.1, 3600.0);
	slideDelaySpin->setSingleStep(0.1);
	slideDelaySpin->setValue(GData::slideShowDelay);
	QHBoxLayout *slideDelayHbox = new QHBoxLayout;
	slideDelayHbox->addWidget(slideDelayLab);
	slideDelayHbox->addWidget(slideDelaySpin);
	slideDelayHbox->addStretch(1);

	// Slide show random
	slideRandomCb = new QCheckBox(tr("Show random images"), this);
	slideRandomCb->setChecked(GData::slideShowRandom);

	// Slide show options
	QVBoxLayout *slideShowVbox = new QVBoxLayout;
	slideShowVbox->addLayout(slideDelayHbox);
	slideShowVbox->addWidget(slideRandomCb);
	QGroupBox *slideShowGbox = new QGroupBox(tr("Slide Show"));
	slideShowGbox->setLayout(slideShowVbox);

	// Keyboard shortcuts widgets
	QComboBox *keysCombo = new QComboBox();
	keyLine = new KeyGrabLineEdit(this, keysCombo);
	connect(keyLine, SIGNAL(textChanged(const QString&)), keyLine, SLOT(clearShortcut()));
	connect(keysCombo, SIGNAL(activated(const QString &)),
							this, SLOT(setActionKeyText(const QString &)));

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		keysCombo->addItem(it.key());
	}
	keyLine->setText(GData::actionKeys.value(keysCombo->currentText())->shortcut().toString());

	// Mouse settings
	reverseMouseCb = new QCheckBox(tr("Swap mouse left-click and middle-click actions"), this);
	reverseMouseCb->setChecked(GData::reverseMouseBehavior);

	// Keyboard and mouse group
	QLabel *changeKeysLab = new QLabel(tr("Keyboard Shortcuts:"));
	QHBoxLayout *keyboardHbox = new QHBoxLayout;
	keyboardHbox->addWidget(changeKeysLab);
	keyboardHbox->addWidget(keysCombo);
	keyboardHbox->addWidget(keyLine);
	keyboardHbox->addStretch(1);

	QVBoxLayout *mouseVbox = new QVBoxLayout;
	mouseVbox->addLayout(keyboardHbox);
	mouseVbox->addWidget(reverseMouseCb);

	QGroupBox *keyboardGbox = new QGroupBox(tr("Keyboard and Mouse"));
	keyboardGbox->setLayout(mouseVbox);

	QVBoxLayout *generalVbox = new QVBoxLayout;
	generalVbox->addWidget(startupDirGroupBox);
	QGroupBox *generalGbox = new QGroupBox(tr("General"));
	generalGbox->setLayout(generalVbox);
	
	// General
	QVBoxLayout *optsLayout = new QVBoxLayout;
	optsWidgetArea->setLayout(optsLayout);
	optsLayout->addWidget(viewerOptsGrp);
	optsLayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Expanding));
	optsLayout->addWidget(thumbOptsGroupBox);
	optsLayout->addWidget(slideShowGbox);
	optsLayout->addWidget(keyboardGbox);
	optsLayout->addWidget(generalGbox);
	optsLayout->addStretch(1);
}

void SettingsDialog::saveSettings()
{
	int i;

	for (i = 0; i < nZoomRadios; ++i)
	{
		if (fitLargeRadios[i]->isChecked())
		{
			GData::zoomOutFlags = i;
			GData::appSettings->setValue("zoomOutFlags", (int)GData::zoomOutFlags);
			break;
		}
	}

	for (i = 0; i < nZoomRadios; ++i)
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
	GData::thumbPagesReadahead = thumbPagesSpin->value();
	GData::exitInsteadOfClose = exitCliCb->isChecked();
	GData::wrapImageList = wrapListCb->isChecked();
	GData::defaultSaveQuality = saveQualitySpin->value();
	GData::noEnlargeSmallThumb = noSmallThumbCb->isChecked();
	GData::slideShowDelay = slideDelaySpin->value();
	GData::slideShowRandom = slideRandomCb->isChecked();
	GData::enableAnimations = enableAnimCb->isChecked();
	GData::exifRotationEnabled = enableExifCb->isChecked();
	GData::enableImageInfoFS = imageInfoCb->isChecked();
	GData::reverseMouseBehavior = reverseMouseCb->isChecked();

	if (startupDirRadios[0]->isChecked())
		GData::startupDir = GData::defaultDir;
	else if (startupDirRadios[1]->isChecked())
		GData::startupDir = GData::rememberLastDir;
	else 
	{
		GData::startupDir = GData::specifiedDir;
		GData::specifiedStartDir = startupDirEdit->text();
	}

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
		setButtonBgColor(userColor, backgroundColorButton);
        bgColor = userColor;
    }
}

void SettingsDialog::setButtonBgColor(QColor &color, QToolButton *button)
{
	QString style = "background: rgb(%1, %2, %3);";
	style = style.arg(color.red()).arg(color.green()).arg(color.blue());
	button->setStyleSheet(style);
}

void SettingsDialog::pickThumbsColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsBackgroundColor, this);
	if (userColor.isValid())
	{	
		setButtonBgColor(userColor, colThumbButton);
		thumbBgColor = userColor;
	}
}

void SettingsDialog::pickThumbsTextColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsTextColor, this);
	if (userColor.isValid())
	{	
		setButtonBgColor(userColor, colThumbTextButton);
		thumbTextColor = userColor;
	}
}

void SettingsDialog::pickStartupDir()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Startup Folder"), "",
									QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	startupDirEdit->setText(dirName);
}

CropDialog::CropDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Cropping"));
	resize(400, 400);
	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *okButton = new QPushButton(tr("OK"));
	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
	okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QSlider *topSlide = new QSlider(Qt::Vertical);
	QSlider *bottomSlide = new QSlider(Qt::Vertical);
	QSlider *leftSlide = new QSlider(Qt::Horizontal);
	QSlider *rightSlide = new QSlider(Qt::Horizontal);

	topSlide->setTickPosition(QSlider::TicksAbove);
	topSlide->setInvertedAppearance(true);
	topSlide->setTickInterval(100);
	bottomSlide->setTickPosition(QSlider::TicksBelow);
	bottomSlide->setTickInterval(100);
	leftSlide->setTickPosition(QSlider::TicksAbove);
	leftSlide->setTickInterval(100);
	rightSlide->setTickPosition(QSlider::TicksBelow);
	rightSlide->setInvertedAppearance(true);
	rightSlide->setTickInterval(100);

	topSpin = new QSpinBox;
	bottomSpin = new QSpinBox;
	leftSpin = new QSpinBox;
	rightSpin = new QSpinBox;

	QGridLayout *mainGbox = new QGridLayout;

	QLabel *topLab = new QLabel(tr("Top"));
	QLabel *leftLab = new QLabel(tr("Left"));
	QLabel *rightLab = new QLabel(tr("Right"));
	QLabel *bottomLab = new QLabel(tr("Bottom"));

	QHBoxLayout *topBox = new QHBoxLayout;
	topBox->addWidget(topLab);
	topBox->addWidget(topSpin);
	topBox->addStretch(1);	

	QHBoxLayout *bottomBox = new QHBoxLayout;
	bottomBox->addWidget(bottomLab);
	bottomBox->addWidget(bottomSpin);
	bottomBox->addStretch(1);	

	QHBoxLayout *leftBox = new QHBoxLayout;
	leftBox->addWidget(leftLab);
	leftBox->addWidget(leftSpin);
	leftBox->addStretch(1);	

	QHBoxLayout *rightBox = new QHBoxLayout;
	rightBox->addWidget(rightLab);
	rightBox->addWidget(rightSpin);
	rightBox->addStretch(1);	

	mainGbox->addWidget(topSlide, 2, 1, 5, 1);
	mainGbox->addLayout(topBox, 4, 2, 1, 1);
	mainGbox->addWidget(bottomSlide, 2, 7, 5, 1);
	mainGbox->addLayout(bottomBox, 4, 6, 1, 1);
	mainGbox->addWidget(leftSlide, 1, 2, 1, 5);
	mainGbox->addLayout(leftBox, 2, 4, 1, 1);
	mainGbox->addWidget(rightSlide, 7, 2, 1, 5);
	mainGbox->addLayout(rightBox, 6, 4, 1, 1);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(mainGbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	int width = imageView->getImageWidthPreCropped();
	int height = imageView->getImageHeightPreCropped();

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
	GData::dialogLastX = pos().x();
	GData::dialogLastY = pos().y(); 
	accept();
}

ResizeDialog::ResizeDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Scale Image"));
	setWindowIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/phototonic.png")));
	resize(300, 200);

	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	width = lastWidth = imageView->getImageWidthPreCropped();
	height = lastHeight = imageView->getImageHeightPreCropped();

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *okButton = new QPushButton(tr("Scale"));
	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(abort()));
	buttonsHbox->addWidget(cancelButton, 1, Qt::AlignRight);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	widthSpin = new QSpinBox;
	widthSpin->setRange(0, width * 10);
	widthSpin->setValue(width);
	connect(widthSpin, SIGNAL(valueChanged(int)), this, SLOT(adjustSizes()));
	heightSpin = new QSpinBox;
	heightSpin->setRange(0, height * 10);
	heightSpin->setValue(height);
	connect(heightSpin, SIGNAL(valueChanged(int)), this, SLOT(adjustSizes()));

	QGridLayout *mainGbox = new QGridLayout;
	QLabel *origSizeLab = new QLabel(tr("Original size: "));
	QString imageSizeStr = QString::number(width) + " x " + QString::number(height);
	QLabel *origSizePixelsLab = new QLabel(imageSizeStr);
	QLabel *widthLab = new QLabel(tr("Width: "));
	QLabel *heightLab = new QLabel(tr("Height: "));
	QLabel *unitsLab = new QLabel(tr("Units: "));

	QLabel *newSizeLab = new QLabel(tr("New size: "));
	newSizePixelsLab = new QLabel(imageSizeStr);

	pixelsRadio = new QRadioButton(tr("Pixels"));
	connect(pixelsRadio, SIGNAL(clicked()), this, SLOT(setUnits()));
	percentRadio = new QRadioButton(tr("Percent"));
	connect(percentRadio, SIGNAL(clicked()), this, SLOT(setUnits()));
	pixelsRadio->setChecked(true);
	pixelUnits = true;

	QCheckBox *lockAspectCb = new QCheckBox(tr("Lock aspect ratio"), this);
	lockAspectCb->setChecked(true);
	connect(lockAspectCb, SIGNAL(clicked()), this, SLOT(setAspectLock()));
	aspectLocked = true;

	QHBoxLayout *radiosHbox = new QHBoxLayout;
	radiosHbox->addStretch(1);
	radiosHbox->addWidget(pixelsRadio);
	radiosHbox->addWidget(percentRadio);

	mainGbox->addWidget(origSizeLab, 2, 2, 1, 1);
	mainGbox->addWidget(origSizePixelsLab, 2, 4, 1, 1);
	mainGbox->addWidget(widthLab, 6, 2, 1, 1);
	mainGbox->addWidget(heightLab, 7, 2, 1, 1);
	mainGbox->addWidget(unitsLab, 3, 2, 1, 1);
	mainGbox->addWidget(widthSpin, 6, 4, 1, 2);
	mainGbox->addWidget(heightSpin, 7, 4, 1, 2);
	mainGbox->addLayout(radiosHbox, 3, 4, 1, 3);
	mainGbox->addWidget(lockAspectCb, 5, 2, 1, 3);
	mainGbox->addWidget(newSizeLab, 8, 2, 1, 1);
	mainGbox->addWidget(newSizePixelsLab, 8, 4, 1, 1);
	mainGbox->setRowStretch(9, 1);
	mainGbox->setColumnStretch(3, 1);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(mainGbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);
	widthSpin->setFocus(Qt::OtherFocusReason);
}

void ResizeDialog::applyResize(int)
{
	imageView->refresh();
}

void ResizeDialog::setAspectLock()
{
	aspectLocked = ((QCheckBox*)QObject::sender())->isChecked();
	adjustSizes();
}

void ResizeDialog::setUnits()
{
	int newWidth;
	int newHeight;

	if (pixelsRadio->isChecked() && !pixelUnits)
	{
		newWidth = (width * widthSpin->value()) / 100;
		newHeight = (height * heightSpin->value()) / 100;
		widthSpin->setRange(0, width * 10);
		heightSpin->setRange(0, height * 10);
		pixelUnits = true;
	}
	else
	{
		newWidth = (100 * widthSpin->value()) / width;
		newHeight = (100 * heightSpin->value()) / height;
		widthSpin->setRange(0, 100 * 10);
		heightSpin->setRange(0, 100 * 10);
		pixelUnits = false;
	}

	widthSpin->setValue(newWidth);
	if (!aspectLocked)
		heightSpin->setValue(newHeight);
}

void ResizeDialog::adjustSizes()
{
	static bool busy = false;
	if (busy)
		return;
	busy = true;

	if (aspectLocked)
	{
		if (pixelUnits)
		{
			QSize imageSize(width, height);
			if (widthSpin->value() > lastWidth || heightSpin->value() > lastHeight)
			{
				imageSize.scale(widthSpin->value(), heightSpin->value(), Qt::KeepAspectRatioByExpanding);
			}
			else
			{
				imageSize.scale(widthSpin->value(), heightSpin->value(), Qt::KeepAspectRatio);
			}

			widthSpin->setValue(imageSize.width());
			heightSpin->setValue(imageSize.height());
			lastWidth = widthSpin->value();
			lastHeight = heightSpin->value();
			newWidth = imageSize.width();
			newHeight = imageSize.height();
		}
		else
		{
			if (widthSpin->value() != lastWidth)
			{
				heightSpin->setValue(widthSpin->value());
			}
			else
			{
				widthSpin->setValue(heightSpin->value());
			}


			lastWidth = widthSpin->value();
			lastHeight = heightSpin->value();

			newWidth = (width * widthSpin->value()) / 100;
			newHeight = (height * heightSpin->value()) / 100;
		}
	}
	else
	{
		if (pixelUnits)
		{
			newWidth = widthSpin->value();
			newHeight = heightSpin->value();
		}
		else
		{
			newWidth = (width * widthSpin->value()) / 100;
			newHeight = (height * heightSpin->value()) / 100;
		}
	}

	newSizePixelsLab->setText(QString::number(newWidth) + " x " + QString::number(newHeight));
	busy = false;
}

void ResizeDialog::ok()
{
	GData::scaledWidth = newWidth;
	GData::scaledHeight = newHeight; 
	accept();
}

void ResizeDialog::abort()
{
	reject();
}

ColorsDialog::ColorsDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Colors"));
	setWindowIcon(QIcon(":/images/colors.png"));
	
	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *resetButton = new QPushButton(tr("Reset"));
	resetButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	resetButton->setIcon(QIcon::fromTheme("document-revert"));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
	buttonsHbox->addWidget(resetButton, 0, Qt::AlignLeft);
	QPushButton *okButton = new QPushButton(tr("OK"));
	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
	okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	/* hue saturation */
	QLabel *hueLab = new QLabel(tr("Hue"));
	QLabel *satLab = new QLabel(tr("Saturation"));
	QLabel *lightLab = new QLabel(tr("Lightness"));
	QLabel *channelsLab = new QLabel(tr("Channels"));

	hueSatEnabledCb = new QCheckBox(tr("Enable"), this);
	hueSatEnabledCb->setCheckState(GData::hueSatEnabled? Qt::Checked : Qt::Unchecked);
	connect(hueSatEnabledCb, SIGNAL(stateChanged(int)), this, SLOT(enableHueSat(int)));	

	hueSlide = new QSlider(Qt::Horizontal);
	hueSlide->setTickPosition(QSlider::TicksAbove);
	hueSlide->setTickInterval(64);
	hueSlide->setRange(0, 255);
	hueSlide->setTracking(false);
	hueSlide->setValue(GData::hueVal);
	connect(hueSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	colorizeCb = new QCheckBox(tr("Colorize"), this);
	colorizeCb->setCheckState(GData::colorizeEnabled? Qt::Checked : Qt::Unchecked);
	connect(colorizeCb, SIGNAL(stateChanged(int)), this, SLOT(enableColorize(int)));	

	saturationSlide = new QSlider(Qt::Horizontal);
	saturationSlide->setTickPosition(QSlider::TicksAbove);
	saturationSlide->setTickInterval(100);
	saturationSlide->setRange(0, 500);
	saturationSlide->setTracking(false);
	saturationSlide->setValue(GData::saturationVal);
	connect(saturationSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	lightnessSlide = new QSlider(Qt::Horizontal);
	lightnessSlide->setTickPosition(QSlider::TicksAbove);
	lightnessSlide->setTickInterval(100);
	lightnessSlide->setRange(0, 500);
	lightnessSlide->setTracking(false);
	lightnessSlide->setValue(GData::lightnessVal);
	connect(lightnessSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QHBoxLayout *channelsHbox = new QHBoxLayout;
	redB = new QCheckBox(tr("Red"));
	redB->setCheckable(true);
	redB->setChecked(GData::hueRedChannel);
	connect(redB, SIGNAL(clicked()), this, SLOT(setRedChannel()));
	channelsHbox->addWidget(redB, 0, Qt::AlignLeft);
	greenB = new QCheckBox(tr("Green"));
	greenB->setCheckable(true);
	greenB->setChecked(GData::hueGreenChannel);
	connect(greenB, SIGNAL(clicked()), this, SLOT(setGreenChannel()));
	channelsHbox->addWidget(greenB, 0, Qt::AlignLeft);
	blueB = new QCheckBox(tr("Blue"));
	blueB->setCheckable(true);
	blueB->setChecked(GData::hueBlueChannel);
	connect(blueB, SIGNAL(clicked()), this, SLOT(setBlueChannel()));
	channelsHbox->addWidget(blueB, 0, Qt::AlignLeft);
	channelsHbox->addStretch(1);

	QGridLayout *hueSatbox = new QGridLayout;
	hueSatbox->addWidget(hueSatEnabledCb,	0, 0, Qt::AlignLeft);
	hueSatbox->addWidget(hueLab,				1, 0, 1, 1);
	hueSatbox->addWidget(hueSlide,			1, 1, 1, 1);
	hueSatbox->addWidget(colorizeCb,			2, 1, 1, 1);
	hueSatbox->addWidget(satLab,				3, 0, 1, 1);
	hueSatbox->addWidget(saturationSlide,	3, 1, 1, 1);
	hueSatbox->addWidget(lightLab, 			4, 0, 1, 1);
	hueSatbox->addWidget(lightnessSlide,		4, 1, 1, 1);
	hueSatbox->addWidget(channelsLab,		5, 0, 1, 1);
	hueSatbox->addLayout(channelsHbox,		5, 1, 1, 1);

	QGroupBox *hueSatGroup = new QGroupBox(tr("Hue and Saturation"));
	hueSatGroup->setLayout(hueSatbox);

	/* brightness contrast */
	QLabel *brightLab = new QLabel(tr("Brightness"));
	QLabel *contrastLab = new QLabel(tr("Contrast"));

	brightContrastEnabledCb = new QCheckBox(tr("Enable"), this);
	brightContrastEnabledCb->setCheckState(GData::brightContrastEnabled? Qt::Checked : Qt::Unchecked);
	connect(brightContrastEnabledCb, SIGNAL(stateChanged(int)), this, SLOT(enableBrightContrast(int)));	

	brightSlide = new QSlider(Qt::Horizontal);
	brightSlide->setTickPosition(QSlider::TicksAbove);
	brightSlide->setTickInterval(100);
	brightSlide->setRange(0, 400);
	brightSlide->setTracking(false);
	brightSlide->setValue(GData::brightVal);
	connect(brightSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	contrastSlide = new QSlider(Qt::Horizontal);
	contrastSlide->setTickPosition(QSlider::TicksAbove);
	contrastSlide->setTickInterval(39);
	contrastSlide->setRange(0, 157);
	contrastSlide->setTracking(false);
	contrastSlide->setValue(GData::contrastVal);
	contrastSlide->setInvertedAppearance(true);
	connect(contrastSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QGridLayout *brightContrastbox = new QGridLayout;
	brightContrastbox->addWidget(brightContrastEnabledCb, 0, 0, Qt::AlignLeft);
	brightContrastbox->addWidget(brightLab, 1, 0, 1, 1);
	brightContrastbox->addWidget(brightSlide,	1, 1, 1, 1);
	brightContrastbox->addWidget(contrastLab, 2, 0, 1, 1);
	brightContrastbox->addWidget(contrastSlide,	2, 1, 1, 1);

	QGroupBox *brightContrastGroup = new QGroupBox(tr("Brightness and Contrast"));
	brightContrastGroup->setLayout(brightContrastbox);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(brightContrastGroup);
	mainVbox->addWidget(hueSatGroup);
	mainVbox->addStretch(1);	
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);
}

void ColorsDialog::applyColors(int)
{
	GData::hueVal = hueSlide->value();
	GData::saturationVal = saturationSlide->value();
	GData::lightnessVal = lightnessSlide->value();
	GData::contrastVal = contrastSlide->value();
	GData::brightVal = brightSlide->value();

	imageView->refresh();
}

void ColorsDialog::ok()
{
	GData::dialogLastX = pos().x();
	GData::dialogLastY = pos().y(); 
	accept();
}

void ColorsDialog::reset()
{
	hueSlide->setValue(0);
	colorizeCb->setChecked(false);
	saturationSlide->setValue(100);
	lightnessSlide->setValue(100);
	redB->setChecked(true);
	greenB->setChecked(true);
	blueB->setChecked(true);
	GData::hueRedChannel = true;
	GData::hueGreenChannel = true;
	GData::hueBlueChannel = true;

	contrastSlide->setValue(CONTRAST_MID);
	brightSlide->setValue(BRIGHTNESS_MID);
	
	imageView->refresh();
}

void ColorsDialog::enableHueSat(int state)
{
	GData::hueSatEnabled = state;
	imageView->refresh();
}

void ColorsDialog::enableBrightContrast(int state)
{
	GData::brightContrastEnabled = state;
	imageView->refresh();
}

void ColorsDialog::enableColorize(int state)
{
	GData::colorizeEnabled = state;
	imageView->refresh();
}

void ColorsDialog::setRedChannel()
{
	GData::hueRedChannel = redB->isChecked();
	imageView->refresh();
}

void ColorsDialog::setGreenChannel()
{
	GData::hueGreenChannel = greenB->isChecked();
	imageView->refresh();
}

void ColorsDialog::setBlueChannel()
{
	GData::hueBlueChannel = blueB->isChecked();
	imageView->refresh();
}

void AppMgmtDialog::addTableModelItem(QStandardItemModel *model, QString &key, QString &val)
{
	int atRow = model->rowCount();
	QStandardItem *itemKey = new QStandardItem(key);
	QStandardItem *itemKey2 = new QStandardItem(val);
	model->insertRow(atRow, itemKey);
	model->setItem(atRow, 1, itemKey2);
}

AppMgmtDialog::AppMgmtDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Manage External Applications"));
	setWindowIcon(QIcon::fromTheme("document-properties", QIcon(":/images/phototonic.png")));
	resize(350, 250);

	appsTable = new QTableView(this);
	appsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
	appsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	appsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	appsTableModel = new QStandardItemModel(this);
	appsTable->setModel(appsTableModel);
	appsTable->verticalHeader()->setVisible(false);
	appsTable->verticalHeader()->setDefaultSectionSize(appsTable->verticalHeader()->minimumSectionSize());
	appsTableModel->setHorizontalHeaderItem(0, new QStandardItem(QString(tr("Name"))));
	appsTableModel->setHorizontalHeaderItem(1,
									new QStandardItem(QString(tr("Application path and arguments"))));
	appsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
	appsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	appsTable->	setShowGrid(false);

	QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Choose"));
   	addButton->setIcon(QIcon::fromTheme("list-add"));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
	addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);
    QPushButton *entryButton = new QPushButton(tr("Add manually"));
	entryButton->setIcon(QIcon::fromTheme("list-add"));
	connect(entryButton, SIGNAL(clicked()), this, SLOT(entry()));
	addRemoveHbox->addWidget(entryButton, 0, Qt::AlignRight);
    QPushButton *removeButton = new QPushButton(tr("Remove"));
   	removeButton->setIcon(QIcon::fromTheme("list-remove"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	addRemoveHbox->addWidget(removeButton, 0, Qt::AlignRight);
	addRemoveHbox->addStretch(1);	

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
   	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);
	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(appsTable);
	mainVbox->addLayout(addRemoveHbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// Load external apps list
	QString key, val;
	QMapIterator<QString, QString> it(GData::externalApps);
	while (it.hasNext())
	{
		it.next();
		key = it.key();
		val = it.value();
		addTableModelItem(appsTableModel, key, val);
	}
}

void AppMgmtDialog::ok()
{
	int row = appsTableModel->rowCount();
	GData::externalApps.clear();
    for (int i = 0; i < row ; ++i)
    {
	    if (!appsTableModel->itemFromIndex(appsTableModel->index(i, 1))->text().isEmpty())
	    {
			GData::externalApps[appsTableModel->itemFromIndex(appsTableModel->index(i, 0))->text()] =
						appsTableModel->itemFromIndex(appsTableModel->index(i, 1))->text();
		}
    }
	accept();
}

void AppMgmtDialog::add()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Application"), "", "");
	if (fileName.isEmpty())
		return;
		
	QFileInfo fileInfo = QFileInfo(fileName);
	QString appName = fileInfo.fileName();
	addTableModelItem(appsTableModel, appName, fileName);
}

void AppMgmtDialog::entry()
{
	int atRow = appsTableModel->rowCount();
	QStandardItem *itemKey = new QStandardItem(QString(tr("New Application")));
	appsTableModel->insertRow(atRow, itemKey);
}

void AppMgmtDialog::remove()
{
	QModelIndexList indexesList;
	while((indexesList = appsTable->selectionModel()->selectedIndexes()).size())
	{
		appsTableModel->removeRow(indexesList.first().row());
	}
}

CopyMoveToDialog::CopyMoveToDialog(QWidget *parent, QString thumbsPath) : QDialog(parent)
{
	setWindowTitle(tr("Copy or Move Images to..."));
	resize(350, 250);
	currentPath = thumbsPath;

	pathsTable = new QTableView(this);
	pathsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
	pathsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	pathsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	pathsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	pathsTable->setSelectionMode(QAbstractItemView::SingleSelection);
	pathsTableModel = new QStandardItemModel(this);
	pathsTable->setModel(pathsTableModel);
	pathsTable->verticalHeader()->setVisible(false);
	pathsTable->horizontalHeader()->setVisible(false);
	pathsTable->verticalHeader()->setDefaultSectionSize(pathsTable->verticalHeader()->
																				minimumSectionSize());
	pathsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	pathsTable->	setShowGrid(false);

	QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Add"));
   	addButton->setIcon(QIcon::fromTheme("list-add"));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
	addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);
    QPushButton *removeButton = new QPushButton(tr("Remove"));
   	removeButton->setIcon(QIcon::fromTheme("list-remove"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	addRemoveHbox->addWidget(removeButton, 0, Qt::AlignRight);
	addRemoveHbox->addStretch(1);	

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *closeButton = new QPushButton(tr("Close"));
	closeButton->setIcon(QIcon::fromTheme("dialog-ok"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(justClose()));
    QPushButton *copyButton = new QPushButton(tr("Copy"));
   	copyButton->setIcon(QIcon::fromTheme("edit-copy"));
    copyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copy()));
    QPushButton *moveButton = new QPushButton(tr("Move"));
   	moveButton->setIcon(QIcon::fromTheme("go-next"));
    moveButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(moveButton, SIGNAL(clicked()), this, SLOT(move()));
	buttonsHbox->addStretch(1);	
	buttonsHbox->addWidget(closeButton, 0, Qt::AlignRight);
	buttonsHbox->addWidget(copyButton, 0, Qt::AlignRight);
	buttonsHbox->addWidget(moveButton, 0, Qt::AlignRight);
	
	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(pathsTable);
	mainVbox->addLayout(addRemoveHbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// Load paths list
	QString key, val;
	QSetIterator<QString> it(GData::copyMoveToPaths);
	while (it.hasNext())
	{
		QStandardItem *item = new QStandardItem(it.next());
		pathsTableModel->insertRow(pathsTableModel->rowCount(), item);
	}
}

void CopyMoveToDialog::savePaths()
{
	GData::copyMoveToPaths.clear();
    for (int i = 0; i < pathsTableModel->rowCount(); ++i)
    {
    	GData::copyMoveToPaths.insert
    						(pathsTableModel->itemFromIndex(pathsTableModel->index(i, 0))->text());
   	}
}

void CopyMoveToDialog::copyOrMove(bool copy)
{
	savePaths();
	copyOp = copy;

	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size())
	{
		selectedPath = pathsTableModel->itemFromIndex(indexesList.first())->text();
		accept();
	}
	else
		reject();
}

void CopyMoveToDialog::copy()
{
	copyOrMove(true);
}

void CopyMoveToDialog::move()
{
	copyOrMove(false);
}

void CopyMoveToDialog::justClose()
{
	savePaths();
	reject();
}

void CopyMoveToDialog::add()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), currentPath,
									QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dirName.isEmpty())
		return;
		
	QStandardItem *item = new QStandardItem(dirName);
	pathsTableModel->insertRow(pathsTableModel->rowCount(), item);

	pathsTable->selectionModel()->clearSelection();
	pathsTable->selectionModel()->select(pathsTableModel->index(pathsTableModel->rowCount() - 1, 0),
																			QItemSelectionModel::Select);
}

void CopyMoveToDialog::remove()
{
	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size())
	{
		pathsTableModel->removeRow(indexesList.first().row());
	}
}

