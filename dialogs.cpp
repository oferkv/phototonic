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
	int tn;

	show();

	if (pasteInCurrDir)
	{
		for (tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			sourceFile = GData::copyCutFileList[tn];
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying \"%1\" to \"%2\".") : tr("Moving \"%1\" to \"%2\"."))
											.arg(sourceFile).arg(destFile));
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
		for (tn = GData::copyCutIdxList.size() - 1; tn >= 0 ; --tn)
		{
			sourceFile = thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->
											data(thumbView->FileNameRole).toString();
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying \"%1\" to \"%2\".") : tr("Moving \"%1\" to \"%2\"."))
											.arg(sourceFile).arg(destFile));
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

	nfiles = GData::copyCutIdxList.size();
	close();	
}

ShortcutsTableView::ShortcutsTableView()
{
	keysModel = new QStandardItemModel();
	keysModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Action")));
	keysModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Shortcut")));
	keysModel->setHorizontalHeaderItem(2, new QStandardItem(""));
	setModel(keysModel);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	verticalHeader()->hide();
	verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	horizontalHeader()->setHighlightSections(false);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	setColumnHidden(2, true);

	shortcutsMenu = new QMenu("");
	clearAction = new QAction(tr("Delete shortcut"), this);
	connect(clearAction, SIGNAL(triggered()), this, SLOT(clearShortcut()));
	shortcutsMenu->addAction(clearAction);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showShortcutsTableMenu(QPoint)));
}

void ShortcutsTableView::addRow(QString action, QString description, QString shortcut)
{
	keysModel->appendRow(QList<QStandardItem*>() << new QStandardItem(description) << new QStandardItem(shortcut) << new QStandardItem(action));
}

void ShortcutsTableView::keyPressEvent(QKeyEvent *e)
{
	if (!this->selectedIndexes().count()) {
		return;
	}
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
		msgBox.warning(this, tr("Set shortcut"),
						tr("\"%1\" is reserved for shortcuts to external applications.").arg(keySeqText));
		return;
	}

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->shortcut().toString() == keySeqText)
		{
			QMessageBox msgBox;
			msgBox.warning(this, tr("Set shortcut"),
						tr("\"%1\" is already assigned to \"%2\" action.").arg(keySeqText).arg(it.key()));
			return;
		}
	}
	
	QStandardItemModel *mod = (QStandardItemModel*)model();
	int row = selectedIndexes().first().row();
	mod->item(row, 1)->setText(keySeqText);
	GData::actionKeys.value(mod->item(row, 2)->text())->setShortcut(QKeySequence(keySeqText));
}

void ShortcutsTableView::clearShortcut()
{
	if (selectedEntry.isValid()) {
		QStandardItemModel *mod = (QStandardItemModel*)model();
		mod->item(selectedEntry.row(), 1)->setText("");
		GData::actionKeys.value(mod->item(selectedEntry.row(), 2)->text())->setShortcut(QKeySequence(""));
	}
}

void ShortcutsTableView::showShortcutsTableMenu(QPoint pt)
{
	selectedEntry = indexAt(pt);
	if (selectedEntry.isValid())
		shortcutsMenu->popup(viewport()->mapToGlobal(pt));

}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Preferences"));
	setWindowIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/phototonic.png")));

	// Image Viewer Options
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

	// imageView background color
	QLabel *backgroundColorLab = new QLabel(tr("Background color:"));
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

	// Exit when opening image
	exitCliCb = new 
				QCheckBox(tr("Exit instead of closing, when image is loaded from command line"), this);
	exitCliCb->setChecked(GData::exitInsteadOfClose);

	// Wrap image list
	wrapListCb = new QCheckBox(tr("Wrap image list when reaching last or first image"), this);
	wrapListCb->setChecked(GData::wrapImageList);

	// Save quality
	QLabel *saveQualityLab = new QLabel(tr("Default quality when saving images:"));
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

	// Enable image Exif rotation
	enableExifCb = new QCheckBox(tr("Rotate image according to Exif orientation"), this);
	enableExifCb->setChecked(GData::exifRotationEnabled);

	// Image Info
	imageInfoCb = new QCheckBox(tr("Show image file name in full screen mode"), this);
	imageInfoCb->setChecked(GData::enableImageInfoFS);

	// Viewer options
	QVBoxLayout *viewerOptsBox = new QVBoxLayout;
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	zoomOptsBox->addStretch(1);

	viewerOptsBox->addLayout(zoomOptsBox);
	viewerOptsBox->addLayout(bgColBox);
	viewerOptsBox->addWidget(enableExifCb);
	viewerOptsBox->addWidget(imageInfoCb);
	viewerOptsBox->addWidget(wrapListCb);
	viewerOptsBox->addWidget(enableAnimCb);
	viewerOptsBox->addLayout(saveQualityHbox);
	viewerOptsBox->addWidget(exitCliCb);
	viewerOptsBox->addStretch(1);

	// thumbView background color
	QLabel *bgThumbTxtLab = new QLabel(tr("Background color:"));
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
	QLabel *txtThumbTxtLab = new QLabel("\t" + tr("Label color:"));
	colThumbTextButton = new QToolButton();
	colThumbTextButton->setFixedSize(48, 24);
	bgThumbColBox->addWidget(txtThumbTxtLab);
	bgThumbColBox->addWidget(colThumbTextButton);
	bgThumbColBox->addStretch(1);
	connect(colThumbTextButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
	setButtonBgColor(GData::thumbsTextColor, colThumbTextButton);
	colThumbTextButton->setAutoFillBackground(true);
	thumbTextColor = GData::thumbsTextColor;

	// thumbview background image
	QLabel *thumbsBackImageLab = new QLabel(tr("Background image:"));
	thumbsBackImageEdit = new QLineEdit;
	thumbsBackImageEdit->setClearButtonEnabled(true);
	thumbsBackImageEdit->setMinimumWidth(200);

	QToolButton *chooseThumbsBackImageButton = new QToolButton();
	chooseThumbsBackImageButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	chooseThumbsBackImageButton->setFixedSize(26, 26);
	chooseThumbsBackImageButton->setIconSize(QSize(16, 16));
	connect(chooseThumbsBackImageButton, SIGNAL(clicked()), this, SLOT(pickBgImage()));
	
	QHBoxLayout *thumbsBackImageEditBox = new QHBoxLayout;
	thumbsBackImageEditBox->addWidget(thumbsBackImageLab);
	thumbsBackImageEditBox->addWidget(thumbsBackImageEdit);
	thumbsBackImageEditBox->addWidget(chooseThumbsBackImageButton);
	thumbsBackImageEditBox->addStretch(1);
	thumbsBackImageEdit->setText(GData::thumbsBackImage);

	// Thumbnail spacing
	QLabel *thumbSpacingLab = new QLabel(tr("Add space between thumbnails:"));
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
	QLabel *thumbPagesLab = new QLabel(tr("Number of thumbnail pages to read ahead:"));
	thumbPagesSpin = new QSpinBox;
	thumbPagesSpin->setRange(1, 10);
	thumbPagesSpin->setValue(GData::thumbPagesReadahead);
	QHBoxLayout *thumbPagesHbox = new QHBoxLayout;
	thumbPagesHbox->addWidget(thumbPagesLab);
	thumbPagesHbox->addWidget(thumbPagesSpin);
	thumbPagesHbox->addStretch(1);

	enableThumbExifCb = new QCheckBox(tr("Rotate thumbnails according to Exif orientation"), this);
	enableThumbExifCb->setChecked(GData::exifThumbRotationEnabled);

	// Thumbnail options
	QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
	thumbsOptsBox->addLayout(bgThumbColBox);
	thumbsOptsBox->addLayout(thumbsBackImageEditBox);
	thumbsOptsBox->addLayout(thumbSpacingHbox);
	thumbsOptsBox->addWidget(enableThumbExifCb);
	thumbsOptsBox->addLayout(thumbPagesHbox);
	thumbsOptsBox->addWidget(noSmallThumbCb);
	thumbsOptsBox->addStretch(1);

	// Slide show delay
	QLabel *slideDelayLab = new QLabel(tr("Delay between slides in seconds:"));
	slideDelaySpin = new QSpinBox;
	slideDelaySpin->setRange(1, 3600);
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
	slideShowVbox->addStretch(1);

	// Startup directory
	QGroupBox *startupDirGroupBox = new QGroupBox(tr("Startup folder"));
	startupDirRadios[GData::defaultDir] = 
					new QRadioButton(tr("Default, or specified by command line argument"));
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

	// Keyboard shortcuts widgets
	ShortcutsTableView *keysTable = new ShortcutsTableView();
	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		keysTable->addRow(it.key(), GData::actionKeys.value(it.key())->text(), GData::actionKeys.value(it.key())->shortcut().toString());
	}

	// Mouse settings
	reverseMouseCb = new QCheckBox(tr("Swap mouse left-click and middle-click actions"), this);
	reverseMouseCb->setChecked(GData::reverseMouseBehavior);

	// Keyboard and mouse
	QGroupBox *keyboardGrp = new QGroupBox(tr("Keyboard Shortcuts"));
	QVBoxLayout *keyboardVbox = new QVBoxLayout;
	keyboardVbox->addWidget(keysTable);
	keyboardGrp->setLayout(keyboardVbox);

	QVBoxLayout *generalVbox = new QVBoxLayout;
	generalVbox->addWidget(keyboardGrp);
	generalVbox->addWidget(reverseMouseCb);
	generalVbox->addWidget(startupDirGroupBox);
	generalVbox->addStretch(1);
	
	/* Confirmation buttons */
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

	/* Tabs */
	QTabWidget *tabs = new QTabWidget;

	QWidget *viewerSettings = new QWidget;
	viewerSettings->setLayout(viewerOptsBox);
	tabs->addTab(viewerSettings, tr("Viewer"));

	QWidget *thumbSettings = new QWidget;
	thumbSettings->setLayout(thumbsOptsBox);
	tabs->addTab(thumbSettings, tr("Thumbnails"));

	QWidget *slideSettings = new QWidget;
	slideSettings->setLayout(slideShowVbox);
	tabs->addTab(slideSettings, tr("Slide Show"));

	QWidget *generalSettings = new QWidget;
	generalSettings->setLayout(generalVbox);
	tabs->addTab(generalSettings, tr("General"));

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(tabs);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);
}

void SettingsDialog::saveSettings()
{
	int i;

	for (i = 0; i < nZoomRadios; ++i) {
		if (fitLargeRadios[i]->isChecked()) {
			GData::zoomOutFlags = i;
			GData::appSettings->setValue("zoomOutFlags", (int)GData::zoomOutFlags);
			break;
		}
	}

	for (i = 0; i < nZoomRadios; ++i) {
		if (fitSmallRadios[i]->isChecked()) {
			GData::zoomInFlags = i;
			GData::appSettings->setValue("zoomInFlags", (int)GData::zoomInFlags);
			break;
		}
	}

	GData::backgroundColor = bgColor;
	GData::thumbsBackgroundColor = thumbBgColor;
	GData::thumbsTextColor = thumbTextColor;
	GData::thumbsBackImage = thumbsBackImageEdit->text();
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
	GData::exifThumbRotationEnabled = enableThumbExifCb->isChecked();
	GData::enableImageInfoFS = imageInfoCb->isChecked();
	GData::reverseMouseBehavior = reverseMouseCb->isChecked();

	if (startupDirRadios[0]->isChecked())
		GData::startupDir = GData::defaultDir;
	else if (startupDirRadios[1]->isChecked())
		GData::startupDir = GData::rememberLastDir;
	else {
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
    if (userColor.isValid()) {	
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
	if (userColor.isValid()) {	
		setButtonBgColor(userColor, colThumbButton);
		thumbBgColor = userColor;
	}
}

void SettingsDialog::pickThumbsTextColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsTextColor, this);
	if (userColor.isValid()) {	
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

void SettingsDialog::pickBgImage()
{
	QString dirName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                   tr("Images") + " (*.jpg *.jpeg *.jpe *.png *.bmp *.tiff *.tif *.ppm *.xbm *.xpm)");
	thumbsBackImageEdit->setText(dirName);
}

CropDialog::CropDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Cropping"));
	setWindowIcon(QIcon(":/images/crop.png"));
	resize(350, 100);
	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *resetButton = new QPushButton(tr("Reset"));
	resetButton->setIcon(QIcon::fromTheme("document-revert"));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
	QPushButton *okButton = new QPushButton(tr("OK"));
	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(resetButton, 0, Qt::AlignLeft);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QSlider *topSlide = new QSlider(Qt::Horizontal);
	topSlide->setTickPosition(QSlider::TicksAbove);
	topSlide->setTickInterval(10);
	topSlide->setTracking(false);

	QSlider *bottomSlide = new QSlider(Qt::Horizontal);
	bottomSlide->setTickPosition(QSlider::TicksAbove);
	bottomSlide->setTickInterval(10);
	bottomSlide->setTracking(false);

	QSlider *leftSlide = new QSlider(Qt::Horizontal);
	leftSlide->setTickPosition(QSlider::TicksAbove);
	leftSlide->setTickInterval(10);
	leftSlide->setTracking(false);

	QSlider *rightSlide = new QSlider(Qt::Horizontal);
	rightSlide->setTickPosition(QSlider::TicksAbove);
	rightSlide->setTickInterval(10);
	rightSlide->setTracking(false);

	topSpin = new QSpinBox;
	topSpin->setPrefix("% ");
	bottomSpin = new QSpinBox;
	bottomSpin->setPrefix("% ");
	leftSpin = new QSpinBox;
	leftSpin->setPrefix("% ");
	rightSpin = new QSpinBox;
	rightSpin->setPrefix("% ");

	QLabel *leftLab = new QLabel(tr("Left"));
	QLabel *rightLab = new QLabel(tr("Right"));
	QLabel *topLab = new QLabel(tr("Top"));
	QLabel *bottomLab = new QLabel(tr("Bottom"));

	QGridLayout *mainGbox = new QGridLayout;
	mainGbox->addWidget(leftLab, 0, 0, 1, 1);
	mainGbox->addWidget(leftSlide, 0, 1, 1, 1);
	mainGbox->addWidget(leftSpin, 0, 2, 1, 1);

	mainGbox->addWidget(rightLab, 1, 0, 1, 1);
	mainGbox->addWidget(rightSlide, 1, 1, 1, 1);
	mainGbox->addWidget(rightSpin, 1, 2, 1, 1);

	mainGbox->addWidget(topLab, 2, 0, 1, 1);
	mainGbox->addWidget(topSlide, 2, 1, 1, 1);
	mainGbox->addWidget(topSpin, 2, 2, 1, 1);

	mainGbox->addWidget(bottomLab, 3, 0, 1, 1);
	mainGbox->addWidget(bottomSlide, 3, 1, 1, 1);
	mainGbox->addWidget(bottomSpin, 3, 2, 1, 1);


	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(mainGbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	topSpin->setRange(0, 100);
	bottomSpin->setRange(0, 100);
	leftSpin->setRange(0, 100);
	rightSpin->setRange(0, 100);
	topSlide->setRange(0, 100);
	bottomSlide->setRange(0, 100);
	leftSlide->setRange(0, 100);
	rightSlide->setRange(0, 100);

	connect(topSlide, SIGNAL(valueChanged(int)), topSpin, SLOT(setValue(int)));
	connect(bottomSlide, SIGNAL(valueChanged(int)), bottomSpin, SLOT(setValue(int)));
	connect(leftSlide, SIGNAL(valueChanged(int)), leftSpin, SLOT(setValue(int)));
	connect(rightSlide, SIGNAL(valueChanged(int)), rightSpin, SLOT(setValue(int)));
	connect(topSpin, SIGNAL(valueChanged(int)), topSlide, SLOT(setValue(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), bottomSlide, SLOT(setValue(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), leftSlide, SLOT(setValue(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), rightSlide, SLOT(setValue(int)));

	connect(topSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
}

void CropDialog::applyCrop(int)
{
	GData::cropLeftPercent = leftSpin->value();
	GData::cropTopPercent = topSpin->value();
	GData::cropWidthPercent = rightSpin->value();
	GData::cropHeightPercent = bottomSpin->value();
	imageView->refresh();
}

void CropDialog::ok()
{
	GData::dialogLastX = pos().x();
	GData::dialogLastY = pos().y(); 
	accept();
}

void CropDialog::reset()
{
	leftSpin->setValue(0);
	rightSpin->setValue(0);
	topSpin->setValue(0);
	bottomSpin->setValue(0);
}

ResizeDialog::ResizeDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Scale Image"));
	setWindowIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/phototonic.png")));
	newWidth = newHeight = 0;

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
	QLabel *origSizeLab = new QLabel(tr("Original size:"));
	QString imageSizeStr = QString::number(width) + " x " + QString::number(height);
	QLabel *origSizePixelsLab = new QLabel(imageSizeStr);
	QLabel *widthLab = new QLabel(tr("Width:"));
	QLabel *heightLab = new QLabel(tr("Height:"));
	QLabel *unitsLab = new QLabel(tr("Units:"));

	QLabel *newSizeLab = new QLabel(tr("New size:"));
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
	if (newWidth || newHeight) {
		GData::scaledWidth = newWidth;
		GData::scaledHeight = newHeight; 
		imageView->refresh();
	}
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
	resize(350, 300);
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

	hueSlide = new QSlider(Qt::Horizontal);
	hueSlide->setTickPosition(QSlider::TicksAbove);
	hueSlide->setTickInterval(25);
	hueSlide->setRange(-100, 100);
	hueSlide->setTracking(false);
	connect(hueSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	colorizeCb = new QCheckBox(tr("Colorize"), this);
	colorizeCb->setCheckState(GData::colorizeEnabled? Qt::Checked : Qt::Unchecked);
	connect(colorizeCb, SIGNAL(stateChanged(int)), this, SLOT(enableColorize(int)));	

	saturationSlide = new QSlider(Qt::Horizontal);
	saturationSlide->setTickPosition(QSlider::TicksAbove);
	saturationSlide->setTickInterval(25);
	saturationSlide->setRange(-100, 100);
	saturationSlide->setTracking(false);
	connect(saturationSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	lightnessSlide = new QSlider(Qt::Horizontal);
	lightnessSlide->setTickPosition(QSlider::TicksAbove);
	lightnessSlide->setTickInterval(25);
	lightnessSlide->setRange(-100, 100);
	lightnessSlide->setTracking(false);
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

	QGridLayout *hueSatLay = new QGridLayout;
	hueSatLay->addWidget(hueLab,				1, 0, 1, 1);
	hueSatLay->addWidget(hueSlide,			1, 1, 1, 1);
	hueSatLay->addWidget(colorizeCb,			2, 1, 1, 1);
	hueSatLay->addWidget(satLab,				3, 0, 1, 1);
	hueSatLay->addWidget(saturationSlide,	3, 1, 1, 1);
	hueSatLay->addWidget(lightLab, 			4, 0, 1, 1);
	hueSatLay->addWidget(lightnessSlide,		4, 1, 1, 1);
	hueSatLay->setColumnMinimumWidth(0, 70);

	QGroupBox *hueSatGroup = new QGroupBox(tr("Hue and Saturation"));
	hueSatGroup->setLayout(hueSatLay);

	QGridLayout *channelsLay = new QGridLayout;
	channelsLay->addLayout(channelsHbox,		5, 1, 1, 1);
	channelsLay->setColumnMinimumWidth(0, 70);
	QGroupBox *channelsGroup = new QGroupBox(tr("Affected Channels"));
	channelsGroup->setLayout(channelsLay);

	/* brightness contrast */
	QLabel *brightLab = new QLabel(tr("Brightness"));
	QLabel *contrastLab = new QLabel(tr("Contrast"));

	brightSlide = new QSlider(Qt::Horizontal);
	brightSlide->setTickPosition(QSlider::TicksAbove);
	brightSlide->setTickInterval(25);
	brightSlide->setRange(-100, 100);
	brightSlide->setTracking(false);
	connect(brightSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	contrastSlide = new QSlider(Qt::Horizontal);
	contrastSlide->setTickPosition(QSlider::TicksAbove);
	contrastSlide->setTickInterval(25);
	contrastSlide->setRange(-100, 100);
	contrastSlide->setTracking(false);
	contrastSlide->setInvertedAppearance(true);
	connect(contrastSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QGridLayout *brightContrastbox = new QGridLayout;
	brightContrastbox->addWidget(brightLab, 1, 0, 1, 1);
	brightContrastbox->addWidget(brightSlide,	1, 1, 1, 1);
	brightContrastbox->addWidget(contrastLab, 2, 0, 1, 1);
	brightContrastbox->addWidget(contrastSlide,	2, 1, 1, 1);
	brightContrastbox->setColumnMinimumWidth(0, 70);

	QGroupBox *brightContrastGroup = new QGroupBox(tr("Brightness and Contrast"));
	brightContrastGroup->setLayout(brightContrastbox);

	/* Channel mixer */
	QLabel *redLab = new QLabel(tr("Red"));
	redSlide = new QSlider(Qt::Horizontal);
	redSlide->setTickPosition(QSlider::TicksAbove);
	redSlide->setTickInterval(25);
	redSlide->setRange(-100, 100);
	redSlide->setTracking(false);
	connect(redSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QLabel *greenLab = new QLabel(tr("Green"));
	greenSlide = new QSlider(Qt::Horizontal);
	greenSlide->setTickPosition(QSlider::TicksAbove);
	greenSlide->setTickInterval(25);
	greenSlide->setRange(-100, 100);
	greenSlide->setTracking(false);
	connect(greenSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QLabel *blueLab = new QLabel(tr("Blue"));
	blueSlide = new QSlider(Qt::Horizontal);
	blueSlide->setTickPosition(QSlider::TicksAbove);
	blueSlide->setTickInterval(25);
	blueSlide->setRange(-100, 100);
	blueSlide->setTracking(false);
	connect(blueSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QGridLayout *channelMixbox = new QGridLayout;
	channelMixbox->addWidget(redLab, 1, 0, 1, 1);
	channelMixbox->addWidget(redSlide,	1, 1, 1, 1);
	channelMixbox->addWidget(greenLab, 2, 0, 1, 1);
	channelMixbox->addWidget(greenSlide, 2, 1, 1, 1);
	channelMixbox->addWidget(blueLab, 3, 0, 1, 1);
	channelMixbox->addWidget(blueSlide,	3, 1, 1, 1);
	channelMixbox->setColumnMinimumWidth(0, 70);

	QGroupBox *channelMixGroup = new QGroupBox(tr("Color Balance"));
	channelMixGroup->setLayout(channelMixbox);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(brightContrastGroup);
	mainVbox->addWidget(channelMixGroup);
	mainVbox->addWidget(hueSatGroup);
	mainVbox->addWidget(channelsGroup);
	mainVbox->addStretch(1);	
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	applyColors(0);
}

void ColorsDialog::applyColors(int)
{
	if (brightSlide->value() >= 0)
		GData::brightVal = (brightSlide->value() * 500 / 100) + 100;
	else
		GData::brightVal = brightSlide->value() + 100;

	if (contrastSlide->value() >= 0)
		GData::contrastVal = (contrastSlide->value() * 79 / 100) + 78;
	else
		GData::contrastVal = contrastSlide->value() + 79;

	GData::hueVal = hueSlide->value() * 127 / 100;

	if (saturationSlide->value() >= 0)
		GData::saturationVal = (saturationSlide->value() * 500 / 100) + 100;
	else
		GData::saturationVal = saturationSlide->value() + 100;

	if (lightnessSlide->value() >= 0)
		GData::lightnessVal = (lightnessSlide->value() * 200 / 100) + 100;
	else
		GData::lightnessVal = lightnessSlide->value() + 100;

	GData::redVal = redSlide->value();
	GData::greenVal = greenSlide->value();
	GData::blueVal = blueSlide->value();
		
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
	saturationSlide->setValue(0);
	lightnessSlide->setValue(0);
	redB->setChecked(true);
	greenB->setChecked(true);
	blueB->setChecked(true);
	GData::hueRedChannel = true;
	GData::hueGreenChannel = true;
	GData::hueBlueChannel = true;

	contrastSlide->setValue(0);
	brightSlide->setValue(0);

	redSlide->setValue(0);
	greenSlide->setValue(0);
	blueSlide->setValue(0);
	
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

CopyMoveToDialog::CopyMoveToDialog(QWidget *parent, QString thumbsPath, bool move) : QDialog(parent)
{
	copyOp = !move;
	if (move) {
		setWindowTitle(tr("Move to..."));
		setWindowIcon(QIcon::fromTheme("go-next"));
	} else {
		setWindowTitle(tr("Copy to..."));
		setWindowIcon(QIcon::fromTheme("edit-copy"));
	}

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

	connect(pathsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(selection(QItemSelection, QItemSelection)));
   	connect(pathsTable, SIGNAL(doubleClicked(const QModelIndex &)), 
				this, SLOT(pathDoubleClick(const QModelIndex &)));

	QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Browse..."));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
    QPushButton *removeButton = new QPushButton(tr("Remove"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	addRemoveHbox->addWidget(removeButton, 0, Qt::AlignLeft);
	addRemoveHbox->addStretch(1);	
	addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(justClose()));

    QPushButton *okButton = new QPushButton(tr("OK"));
   	okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	connect(okButton, SIGNAL(clicked()), this, SLOT(copyOrMove()));

	buttonsHbox->addStretch(1);	
	buttonsHbox->addWidget(cancelButton, 0, Qt::AlignRight);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	destinationLab = new QLabel(tr("Destination:"));
	QFrame *line = new QFrame(this);
	line->setObjectName(QString::fromUtf8("line"));
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(pathsTable);
	mainVbox->addLayout(addRemoveHbox);
	mainVbox->addWidget(line);	
	mainVbox->addWidget(destinationLab);	
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// Load paths list
	QSetIterator<QString> it(GData::bookmarkPaths);
	while (it.hasNext()) {
		QStandardItem *item = new QStandardItem(QIcon(":/images/bookmarks.png"), it.next());
		pathsTableModel->insertRow(pathsTableModel->rowCount(), item);
	}
	pathsTableModel->sort(0);
}

void CopyMoveToDialog::selection(const QItemSelection&, const QItemSelection&)
{
	if (pathsTable->selectionModel()->selectedRows().size() > 0) {
		destinationLab->setText(tr("Destination:") + " " +
			pathsTableModel->item(pathsTable->selectionModel()->selectedRows().at(0).row())->text());
	}
}

void CopyMoveToDialog::pathDoubleClick(const QModelIndex &)
{
	copyOrMove();
}

void CopyMoveToDialog::savePaths()
{
	GData::bookmarkPaths.clear();
    for (int i = 0; i < pathsTableModel->rowCount(); ++i) {
    	GData::bookmarkPaths.insert
    						(pathsTableModel->itemFromIndex(pathsTableModel->index(i, 0))->text());
   	}
}

void CopyMoveToDialog::copyOrMove()
{
	savePaths();

	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
		selectedPath = pathsTableModel->itemFromIndex(indexesList.first())->text();
		accept();
	} else {
		reject();
	}
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
		
	QStandardItem *item = new QStandardItem(QIcon(":/images/bookmarks.png"), dirName);
	pathsTableModel->insertRow(pathsTableModel->rowCount(), item);

	pathsTable->selectionModel()->clearSelection();
	pathsTable->selectionModel()->select(pathsTableModel->index(pathsTableModel->rowCount() - 1, 0),
															QItemSelectionModel::Select);
}

void CopyMoveToDialog::remove()
{
	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
		pathsTableModel->removeRow(indexesList.first().row());
	}
}

