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

#include "Dialogs.h"
#include "Settings.h"

CopyMoveDialog::CopyMoveDialog(QWidget *parent) : QDialog(parent) {
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

void CopyMoveDialog::abort() {
    abortOp = true;
}

static QString autoRename(QString &destDir, QString &currFile) {
    int extSep = currFile.lastIndexOf(".");
    QString nameOnly = currFile.left(extSep);
    QString extOnly = currFile.right(currFile.size() - extSep - 1);
    QString newFile;

    int idx = 1;
    do {
        newFile = QString(nameOnly + "_copy_%1." + extOnly).arg(idx);
        ++idx;
    } while (idx && (QFile::exists(destDir + QDir::separator() + newFile)));

    return newFile;
}

int copyMoveFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir) {
    int res;

    if (isCopy) {
        res = QFile::copy(srcPath, dstPath);
    } else {
        res = QFile::rename(srcPath, dstPath);
    }

    if (!res && QFile::exists(dstPath)) {
        QString newName = autoRename(dstDir, srcFile);
        QString newDestPath = dstDir + QDir::separator() + newName;

        if (isCopy) {
            res = QFile::copy(srcPath, newDestPath);
        } else {
            res = QFile::rename(srcPath, newDestPath);
        }
        dstPath = newDestPath;
    }

    return res;
}

void CopyMoveDialog::exec(ThumbsViewer *thumbView, QString &destDir, bool pasteInCurrDir) {
    int res = 0;
    QString sourceFile;
    QFileInfo fileInfo;
    QString currFile;
    QString destFile;
    int tn;

    show();

    if (pasteInCurrDir) {
        for (tn = 0; tn < Settings::copyCutFileList.size(); ++tn) {
            sourceFile = Settings::copyCutFileList[tn];
            fileInfo = QFileInfo(sourceFile);
            currFile = fileInfo.fileName();
            destFile = destDir + QDir::separator() + currFile;

            opLabel->setText((Settings::copyOp ? tr("Copying \"%1\" to \"%2\".") : tr("Moving \"%1\" to \"%2\"."))
                                     .arg(sourceFile).arg(destFile));
            QApplication::processEvents();

            res = copyMoveFile(Settings::copyOp, currFile, sourceFile, destFile, destDir);

            if (!res || abortOp) {
                break;
            } else {
                Settings::copyCutFileList[tn] = destFile;
            }
        }
    } else {
        QList<int> rowList;
        for (tn = Settings::copyCutIdxList.size() - 1; tn >= 0; --tn) {
            sourceFile = thumbView->thumbsViewerModel->item(Settings::copyCutIdxList[tn].row())->
                    data(thumbView->FileNameRole).toString();
            fileInfo = QFileInfo(sourceFile);
            currFile = fileInfo.fileName();
            destFile = destDir + QDir::separator() + currFile;

            opLabel->setText((Settings::copyOp ?
                              tr("Copying \"%1\" to \"%2\".")
                                               : tr("Moving \"%1\" to \"%2\".")).arg(sourceFile).arg(destFile));
            QApplication::processEvents();

            res = copyMoveFile(Settings::copyOp, currFile, sourceFile, destFile, destDir);

            if (!res || abortOp) {
                break;
            }

            rowList.append(Settings::copyCutIdxList[tn].row());
        }

        if (!Settings::copyOp) {
            qSort(rowList);
            for (int t = rowList.size() - 1; t >= 0; --t)
                thumbView->thumbsViewerModel->removeRow(rowList.at(t));
        }
        latestRow = rowList.at(0);
    }

    nFiles = Settings::copyCutIdxList.size();
    close();
}

ShortcutsTableView::ShortcutsTableView() {
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

void ShortcutsTableView::addRow(QString action, QString description, QString shortcut) {
    keysModel->appendRow(QList<QStandardItem *>() << new QStandardItem(description) << new QStandardItem(shortcut)
                                                  << new QStandardItem(action));
}

void ShortcutsTableView::keyPressEvent(QKeyEvent *e) {
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

    if ((e->key() >= Qt::Key_Shift && e->key() <= Qt::Key_ScrollLock)
        || (e->key() >= Qt::Key_Super_L && e->key() <= Qt::Key_Direction_R)
        || e->key() == Qt::Key_AltGr
        || e->key() < 0) {
        return;
    }

    keyText = QKeySequence(e->key()).toString();
    keySeqText = modifierText + keyText;

    if (e->modifiers() & Qt::AltModifier && (e->key() > Qt::Key_0 && e->key() <= Qt::Key_Colon)) {
        QMessageBox msgBox;
        msgBox.warning(this, tr("Set shortcut"),
                       tr("\"%1\" is reserved for shortcuts to external applications.").arg(keySeqText));
        return;
    }

    QMapIterator<QString, QAction *> it(Settings::actionKeys);
    while (it.hasNext()) {
        it.next();
        if (it.value()->shortcut().toString() == keySeqText) {
            QMessageBox msgBox;
            msgBox.warning(this, tr("Set shortcut"),
                           tr("\"%1\" is already assigned to \"%2\" action.").arg(keySeqText).arg(it.key()));
            return;
        }
    }

    QStandardItemModel *mod = (QStandardItemModel *) model();
    int row = selectedIndexes().first().row();
    mod->item(row, 1)->setText(keySeqText);
    Settings::actionKeys.value(mod->item(row, 2)->text())->setShortcut(QKeySequence(keySeqText));
}

void ShortcutsTableView::clearShortcut() {
    if (selectedEntry.isValid()) {
        QStandardItemModel *mod = (QStandardItemModel *) model();
        mod->item(selectedEntry.row(), 1)->setText("");
        Settings::actionKeys.value(mod->item(selectedEntry.row(), 2)->text())->setShortcut(QKeySequence(""));
    }
}

void ShortcutsTableView::showShortcutsTableMenu(QPoint pt) {
    selectedEntry = indexAt(pt);
    if (selectedEntry.isValid())
        shortcutsMenu->popup(viewport()->mapToGlobal(pt));

}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Preferences"));
    setWindowIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/phototonic.png")));

    // Image Viewer Options
    // Zoom large images
    QGroupBox *fitLargeGroupBox = new QGroupBox(tr("Fit Large Images"));
    fitLargeRadios[0] = new QRadioButton(tr("Disable"));
    fitLargeRadios[1] = new QRadioButton(tr("By width or height"));
    fitLargeRadios[2] = new QRadioButton(tr("By width"));
    fitLargeRadios[3] = new QRadioButton(tr("By height"));
    fitLargeRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
    QVBoxLayout *fitLargeVbox = new QVBoxLayout;
    for (int i = 0; i < nZoomRadios; ++i) {
        fitLargeVbox->addWidget(fitLargeRadios[i]);
        fitLargeRadios[i]->setChecked(false);
    }
    fitLargeVbox->addStretch(1);
    fitLargeGroupBox->setLayout(fitLargeVbox);
    fitLargeRadios[Settings::zoomOutFlags]->setChecked(true);

    // Zoom small images
    QGroupBox *fitSmallGroupBox = new QGroupBox(tr("Fit Small Images"));
    fitSmallRadios[0] = new QRadioButton(tr("Disable"));
    fitSmallRadios[1] = new QRadioButton(tr("By width or height"));
    fitSmallRadios[2] = new QRadioButton(tr("By width"));
    fitSmallRadios[3] = new QRadioButton(tr("By height"));
    fitSmallRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
    QVBoxLayout *fitSmallVbox = new QVBoxLayout;
    for (int i = 0; i < nZoomRadios; ++i) {
        fitSmallVbox->addWidget(fitSmallRadios[i]);
        fitSmallRadios[i]->setChecked(false);
    }
    fitSmallVbox->addStretch(1);
    fitSmallGroupBox->setLayout(fitSmallVbox);
    fitSmallRadios[Settings::zoomInFlags]->setChecked(true);

    // imageViewer background color
    QLabel *backgroundColorLabel = new QLabel(tr("Background color:"));
    backgroundColorButton = new QToolButton();
    backgroundColorButton->setFixedSize(48, 24);
    QHBoxLayout *backgroundColorHBox = new QHBoxLayout;
    backgroundColorHBox->addWidget(backgroundColorLabel);
    backgroundColorHBox->addWidget(backgroundColorButton);
    backgroundColorHBox->addStretch(1);
    connect(backgroundColorButton, SIGNAL(clicked()), this, SLOT(pickColor()));
    setButtonBgColor(Settings::backgroundColor, backgroundColorButton);
    backgroundColorButton->setAutoFillBackground(true);
    imageViewerBackgroundColor = Settings::backgroundColor;

    // Exit when opening image
    exitCliCheckBox = new QCheckBox(tr("Exit instead of closing, when image is loaded from command line"), this);
    exitCliCheckBox->setChecked(Settings::exitInsteadOfClose);

    // Wrap image list
    wrapListCheckBox = new QCheckBox(tr("Wrap image list when reaching last or first image"), this);
    wrapListCheckBox->setChecked(Settings::wrapImageList);

    // Save quality
    QLabel *saveQualityLabel = new QLabel(tr("Default quality when saving images:"));
    saveQualitySpinBox = new QSpinBox;
    saveQualitySpinBox->setRange(0, 100);
    saveQualitySpinBox->setValue(Settings::defaultSaveQuality);
    QHBoxLayout *saveQualityHbox = new QHBoxLayout;
    saveQualityHbox->addWidget(saveQualityLabel);
    saveQualityHbox->addWidget(saveQualitySpinBox);
    saveQualityHbox->addStretch(1);

    // Enable animations
    enableAnimCheckBox = new QCheckBox(tr("Enable GIF animation"), this);
    enableAnimCheckBox->setChecked(Settings::enableAnimations);

    // Enable image Exif rotation
    enableExifCheckBox = new QCheckBox(tr("Rotate image according to Exif orientation"), this);
    enableExifCheckBox->setChecked(Settings::exifRotationEnabled);

    // Image Info
    imageInfoCheckBox = new QCheckBox(tr("Show image file name in viewer"), this);
    imageInfoCheckBox->setChecked(Settings::enableImageInfoFS);

    // Viewer options
    QVBoxLayout *viewerOptsBox = new QVBoxLayout;
    QHBoxLayout *zoomOptsBox = new QHBoxLayout;
    zoomOptsBox->setAlignment(Qt::AlignTop);
    zoomOptsBox->addWidget(fitLargeGroupBox);
    zoomOptsBox->addWidget(fitSmallGroupBox);
    zoomOptsBox->addStretch(1);

    viewerOptsBox->addLayout(zoomOptsBox);
    viewerOptsBox->addLayout(backgroundColorHBox);
    viewerOptsBox->addWidget(enableExifCheckBox);
    viewerOptsBox->addWidget(imageInfoCheckBox);
    viewerOptsBox->addWidget(wrapListCheckBox);
    viewerOptsBox->addWidget(enableAnimCheckBox);
    viewerOptsBox->addLayout(saveQualityHbox);
    viewerOptsBox->addWidget(exitCliCheckBox);
    viewerOptsBox->addStretch(1);

    // thumbsViewer background color
    QLabel *thumbsBackgroundColorLabel = new QLabel(tr("Background color:"));
    thumbsColorPickerButton = new QToolButton();
    thumbsColorPickerButton->setFixedSize(48, 24);
    QHBoxLayout *thumbsBackgroundColorHbox = new QHBoxLayout;
    thumbsBackgroundColorHbox->addWidget(thumbsBackgroundColorLabel);
    thumbsBackgroundColorHbox->addWidget(thumbsColorPickerButton);
    connect(thumbsColorPickerButton, SIGNAL(clicked()), this, SLOT(pickThumbsColor()));
    setButtonBgColor(Settings::thumbsBackgroundColor, thumbsColorPickerButton);
    thumbsColorPickerButton->setAutoFillBackground(true);
    thumbsBackgroundColor = Settings::thumbsBackgroundColor;

    // thumbsViewer text color
    QLabel *txtThumbTxtLab = new QLabel("\t" + tr("Label color:"));
    colThumbTextButton = new QToolButton();
    colThumbTextButton->setFixedSize(48, 24);
    thumbsBackgroundColorHbox->addWidget(txtThumbTxtLab);
    thumbsBackgroundColorHbox->addWidget(colThumbTextButton);
    thumbsBackgroundColorHbox->addStretch(1);
    connect(colThumbTextButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
    setButtonBgColor(Settings::thumbsTextColor, colThumbTextButton);
    colThumbTextButton->setAutoFillBackground(true);
    thumbsTextColor = Settings::thumbsTextColor;

    // thumbview background image
    QLabel *thumbsBackImageLab = new QLabel(tr("Background image:"));
    thumbsBackgroundImageLineEdit = new QLineEdit;
    thumbsBackgroundImageLineEdit->setClearButtonEnabled(true);
    thumbsBackgroundImageLineEdit->setMinimumWidth(200);

    QToolButton *chooseThumbsBackImageButton = new QToolButton();
    chooseThumbsBackImageButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    chooseThumbsBackImageButton->setFixedSize(26, 26);
    chooseThumbsBackImageButton->setIconSize(QSize(16, 16));
    connect(chooseThumbsBackImageButton, SIGNAL(clicked()), this, SLOT(pickBgImage()));

    QHBoxLayout *thumbsBackImageEditBox = new QHBoxLayout;
    thumbsBackImageEditBox->addWidget(thumbsBackImageLab);
    thumbsBackImageEditBox->addWidget(thumbsBackgroundImageLineEdit);
    thumbsBackImageEditBox->addWidget(chooseThumbsBackImageButton);
    thumbsBackImageEditBox->addStretch(1);
    thumbsBackgroundImageLineEdit->setText(Settings::thumbsBackImage);

    // Thumbnail pages to read ahead
    QLabel *thumbPagesLab = new QLabel(tr("Number of thumbnail pages to read ahead:"));
    thumbPagesSpinBox = new QSpinBox;
    thumbPagesSpinBox->setRange(1, 10);
    thumbPagesSpinBox->setValue(Settings::thumbPagesReadahead);
    QHBoxLayout *thumbPagesHbox = new QHBoxLayout;
    thumbPagesHbox->addWidget(thumbPagesLab);
    thumbPagesHbox->addWidget(thumbPagesSpinBox);
    thumbPagesHbox->addStretch(1);

    enableThumbExifCheckBox = new QCheckBox(tr("Rotate thumbnails according to Exif orientation"), this);
    enableThumbExifCheckBox->setChecked(Settings::exifThumbRotationEnabled);

    // Thumbnail options
    QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
    thumbsOptsBox->addLayout(thumbsBackgroundColorHbox);
    thumbsOptsBox->addLayout(thumbsBackImageEditBox);
    thumbsOptsBox->addWidget(enableThumbExifCheckBox);
    thumbsOptsBox->addLayout(thumbPagesHbox);
    thumbsOptsBox->addStretch(1);

    // Slide show delay
    QLabel *slideDelayLab = new QLabel(tr("Delay between slides in seconds:"));
    slideDelaySpinBox = new QSpinBox;
    slideDelaySpinBox->setRange(1, 3600);
    slideDelaySpinBox->setValue(Settings::slideShowDelay);
    QHBoxLayout *slideDelayHbox = new QHBoxLayout;
    slideDelayHbox->addWidget(slideDelayLab);
    slideDelayHbox->addWidget(slideDelaySpinBox);
    slideDelayHbox->addStretch(1);

    // Slide show random
    slideRandomCheckBox = new QCheckBox(tr("Show random images"), this);
    slideRandomCheckBox->setChecked(Settings::slideShowRandom);

    // Slide show options
    QVBoxLayout *slideShowVbox = new QVBoxLayout;
    slideShowVbox->addLayout(slideDelayHbox);
    slideShowVbox->addWidget(slideRandomCheckBox);
    slideShowVbox->addStretch(1);

    // Startup directory
    QGroupBox *startupDirGroupBox = new QGroupBox(tr("Startup Directory"));
    startupDirRadioButtons[Settings::DefaultDir] =
            new QRadioButton(tr("Default, or specified by command line argument"));
    startupDirRadioButtons[Settings::RememberLastDir] = new QRadioButton(tr("Remember last"));
    startupDirRadioButtons[Settings::SpecifiedDir] = new QRadioButton(tr("Specify:"));

    startupDirLineEdit = new QLineEdit;
    startupDirLineEdit->setClearButtonEnabled(true);
    startupDirLineEdit->setMinimumWidth(300);
    startupDirLineEdit->setMaximumWidth(400);

    QToolButton *chooseStartupDirButton = new QToolButton();
    chooseStartupDirButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    chooseStartupDirButton->setFixedSize(26, 26);
    chooseStartupDirButton->setIconSize(QSize(16, 16));
    connect(chooseStartupDirButton, SIGNAL(clicked()), this, SLOT(pickStartupDir()));

    QHBoxLayout *startupDirEditBox = new QHBoxLayout;
    startupDirEditBox->addWidget(startupDirRadioButtons[2]);
    startupDirEditBox->addWidget(startupDirLineEdit);
    startupDirEditBox->addWidget(chooseStartupDirButton);
    startupDirEditBox->addStretch(1);

    QVBoxLayout *startupDirVbox = new QVBoxLayout;
    for (int i = 0; i < 2; ++i) {
        startupDirVbox->addWidget(startupDirRadioButtons[i]);
        startupDirRadioButtons[i]->setChecked(false);
    }
    startupDirVbox->addLayout(startupDirEditBox);
    startupDirVbox->addStretch(1);
    startupDirGroupBox->setLayout(startupDirVbox);

    if (Settings::startupDir == Settings::SpecifiedDir)
        startupDirRadioButtons[Settings::SpecifiedDir]->setChecked(true);
    else if (Settings::startupDir == Settings::RememberLastDir)
        startupDirRadioButtons[Settings::RememberLastDir]->setChecked(true);
    else
        startupDirRadioButtons[Settings::DefaultDir]->setChecked(true);
    startupDirLineEdit->setText(Settings::specifiedStartDir);

    // Keyboard shortcuts widgets
    ShortcutsTableView *keysTable = new ShortcutsTableView();
    QMapIterator<QString, QAction *> it(Settings::actionKeys);
    while (it.hasNext()) {
        it.next();
        keysTable->addRow(it.key(), Settings::actionKeys.value(it.key())->text(),
                          Settings::actionKeys.value(it.key())->shortcut().toString());
    }

    // Mouse settings
    reverseMouseCheckBox = new QCheckBox(tr("Swap mouse left-click and middle-click actions"), this);
    reverseMouseCheckBox->setChecked(Settings::reverseMouseBehavior);

    // Delete confirmation setting
    deleteConfirmCheckBox = new QCheckBox(tr("Delete confirmation"), this);
    deleteConfirmCheckBox->setChecked(Settings::deleteConfirm);

    // Keyboard and mouse
    QGroupBox *keyboardGrp = new QGroupBox(tr("Keyboard Shortcuts"));
    QVBoxLayout *keyboardVbox = new QVBoxLayout;
    keyboardVbox->addWidget(keysTable);
    keyboardGrp->setLayout(keyboardVbox);

    QVBoxLayout *generalVbox = new QVBoxLayout;
    generalVbox->addWidget(keyboardGrp);
    generalVbox->addWidget(reverseMouseCheckBox);
    generalVbox->addWidget(deleteConfirmCheckBox);
    generalVbox->addWidget(startupDirGroupBox);
    generalVbox->addStretch(1);

    /* Confirmation buttons */
    QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
    okButton->setDefault(true);
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

void SettingsDialog::saveSettings() {
    int i;

    for (i = 0; i < nZoomRadios; ++i) {
        if (fitLargeRadios[i]->isChecked()) {
            Settings::zoomOutFlags = i;
            Settings::appSettings->setValue("zoomOutFlags", (int) Settings::zoomOutFlags);
            break;
        }
    }

    for (i = 0; i < nZoomRadios; ++i) {
        if (fitSmallRadios[i]->isChecked()) {
            Settings::zoomInFlags = i;
            Settings::appSettings->setValue("zoomInFlags", (int) Settings::zoomInFlags);
            break;
        }
    }

    Settings::backgroundColor = imageViewerBackgroundColor;
    Settings::thumbsBackgroundColor = thumbsBackgroundColor;
    Settings::thumbsTextColor = thumbsTextColor;
    Settings::thumbsBackImage = thumbsBackgroundImageLineEdit->text();
    Settings::thumbPagesReadahead = thumbPagesSpinBox->value();
    Settings::exitInsteadOfClose = exitCliCheckBox->isChecked();
    Settings::wrapImageList = wrapListCheckBox->isChecked();
    Settings::defaultSaveQuality = saveQualitySpinBox->value();
    Settings::slideShowDelay = slideDelaySpinBox->value();
    Settings::slideShowRandom = slideRandomCheckBox->isChecked();
    Settings::enableAnimations = enableAnimCheckBox->isChecked();
    Settings::exifRotationEnabled = enableExifCheckBox->isChecked();
    Settings::exifThumbRotationEnabled = enableThumbExifCheckBox->isChecked();
    Settings::enableImageInfoFS = imageInfoCheckBox->isChecked();
    Settings::reverseMouseBehavior = reverseMouseCheckBox->isChecked();
    Settings::deleteConfirm = deleteConfirmCheckBox->isChecked();

    if (startupDirRadioButtons[0]->isChecked())
        Settings::startupDir = Settings::DefaultDir;
    else if (startupDirRadioButtons[1]->isChecked())
        Settings::startupDir = Settings::RememberLastDir;
    else {
        Settings::startupDir = Settings::SpecifiedDir;
        Settings::specifiedStartDir = startupDirLineEdit->text();
    }

    accept();
}

void SettingsDialog::abort() {
    reject();
}

void SettingsDialog::pickColor() {
    QColor userColor = QColorDialog::getColor(Settings::backgroundColor, this);
    if (userColor.isValid()) {
        setButtonBgColor(userColor, backgroundColorButton);
        imageViewerBackgroundColor = userColor;
    }
}

void SettingsDialog::setButtonBgColor(QColor &color, QToolButton *button) {
    QString style = "background: rgb(%1, %2, %3);";
    style = style.arg(color.red()).arg(color.green()).arg(color.blue());
    button->setStyleSheet(style);
}

void SettingsDialog::pickThumbsColor() {
    QColor userColor = QColorDialog::getColor(Settings::thumbsBackgroundColor, this);
    if (userColor.isValid()) {
        setButtonBgColor(userColor, thumbsColorPickerButton);
        thumbsBackgroundColor = userColor;
    }
}

void SettingsDialog::pickThumbsTextColor() {
    QColor userColor = QColorDialog::getColor(Settings::thumbsTextColor, this);
    if (userColor.isValid()) {
        setButtonBgColor(userColor, colThumbTextButton);
        thumbsTextColor = userColor;
    }
}

void SettingsDialog::pickStartupDir() {
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Startup Folder"), "",
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    startupDirLineEdit->setText(dirName);
}

void SettingsDialog::pickBgImage() {
    QString dirName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                   tr("Images") +
                                                   " (*.jpg *.jpeg *.jpe *.png *.bmp *.tiff *.tif *.ppm *.xbm *.xpm)");
    thumbsBackgroundImageLineEdit->setText(dirName);
}

CropDialog::CropDialog(QWidget *parent, ImageViewer *imageViewer) : QDialog(parent) {
    setWindowTitle(tr("Cropping"));
    setWindowIcon(QIcon(":/images/crop.png"));
    resize(350, 100);
    if (Settings::dialogLastX)
        move(Settings::dialogLastX, Settings::dialogLastY);
    this->imageViewer = imageViewer;

    QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *resetButton = new QPushButton(tr("Reset"));
    resetButton->setIcon(QIcon::fromTheme("document-revert"));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
    okButton->setDefault(true);
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

    topSpinBox = new QSpinBox;
    topSpinBox->setPrefix("% ");
    bottomSpinBox = new QSpinBox;
    bottomSpinBox->setPrefix("% ");
    leftSpinBox = new QSpinBox;
    leftSpinBox->setPrefix("% ");
    rightSpinBox = new QSpinBox;
    rightSpinBox->setPrefix("% ");

    QLabel *leftLab = new QLabel(tr("Left"));
    QLabel *rightLab = new QLabel(tr("Right"));
    QLabel *topLab = new QLabel(tr("Top"));
    QLabel *bottomLab = new QLabel(tr("Bottom"));

    QGridLayout *mainGbox = new QGridLayout;
    mainGbox->addWidget(leftLab, 0, 0, 1, 1);
    mainGbox->addWidget(leftSlide, 0, 1, 1, 1);
    mainGbox->addWidget(leftSpinBox, 0, 2, 1, 1);

    mainGbox->addWidget(rightLab, 1, 0, 1, 1);
    mainGbox->addWidget(rightSlide, 1, 1, 1, 1);
    mainGbox->addWidget(rightSpinBox, 1, 2, 1, 1);

    mainGbox->addWidget(topLab, 2, 0, 1, 1);
    mainGbox->addWidget(topSlide, 2, 1, 1, 1);
    mainGbox->addWidget(topSpinBox, 2, 2, 1, 1);

    mainGbox->addWidget(bottomLab, 3, 0, 1, 1);
    mainGbox->addWidget(bottomSlide, 3, 1, 1, 1);
    mainGbox->addWidget(bottomSpinBox, 3, 2, 1, 1);


    QVBoxLayout *mainVbox = new QVBoxLayout;
    mainVbox->addLayout(mainGbox);
    mainVbox->addLayout(buttonsHbox);
    setLayout(mainVbox);

    topSpinBox->setRange(0, 100);
    bottomSpinBox->setRange(0, 100);
    leftSpinBox->setRange(0, 100);
    rightSpinBox->setRange(0, 100);
    topSlide->setRange(0, 100);
    bottomSlide->setRange(0, 100);
    leftSlide->setRange(0, 100);
    rightSlide->setRange(0, 100);

    connect(topSlide, SIGNAL(valueChanged(int)), topSpinBox, SLOT(setValue(int)));
    connect(bottomSlide, SIGNAL(valueChanged(int)), bottomSpinBox, SLOT(setValue(int)));
    connect(leftSlide, SIGNAL(valueChanged(int)), leftSpinBox, SLOT(setValue(int)));
    connect(rightSlide, SIGNAL(valueChanged(int)), rightSpinBox, SLOT(setValue(int)));
    connect(topSpinBox, SIGNAL(valueChanged(int)), topSlide, SLOT(setValue(int)));
    connect(bottomSpinBox, SIGNAL(valueChanged(int)), bottomSlide, SLOT(setValue(int)));
    connect(leftSpinBox, SIGNAL(valueChanged(int)), leftSlide, SLOT(setValue(int)));
    connect(rightSpinBox, SIGNAL(valueChanged(int)), rightSlide, SLOT(setValue(int)));

    connect(topSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
    connect(bottomSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
    connect(leftSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
    connect(rightSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
}

void CropDialog::applyCrop(int) {
    Settings::cropLeftPercent = leftSpinBox->value();
    Settings::cropTopPercent = topSpinBox->value();
    Settings::cropWidthPercent = rightSpinBox->value();
    Settings::cropHeightPercent = bottomSpinBox->value();
    imageViewer->refresh();
}

void CropDialog::ok() {
    Settings::dialogLastX = pos().x();
    Settings::dialogLastY = pos().y();
    accept();
}

void CropDialog::reset() {
    leftSpinBox->setValue(0);
    rightSpinBox->setValue(0);
    topSpinBox->setValue(0);
    bottomSpinBox->setValue(0);
}

ResizeDialog::ResizeDialog(QWidget *parent, ImageViewer *imageViewer) : QDialog(parent) {
    setWindowTitle(tr("Scale Image"));
    setWindowIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/phototonic.png")));
    newWidth = newHeight = 0;

    if (Settings::dialogLastX)
        move(Settings::dialogLastX, Settings::dialogLastY);
    this->imageViewer = imageViewer;

    width = lastWidth = imageViewer->getImageWidthPreCropped();
    height = lastHeight = imageViewer->getImageHeightPreCropped();

    QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("Scale"));
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
    okButton->setDefault(true);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(abort()));
    buttonsHbox->addWidget(cancelButton, 1, Qt::AlignRight);
    buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

    widthSpinBox = new QSpinBox;
    widthSpinBox->setRange(0, width * 10);
    widthSpinBox->setValue(width);
    connect(widthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(adjustSizes()));
    heightSpinBox = new QSpinBox;
    heightSpinBox->setRange(0, height * 10);
    heightSpinBox->setValue(height);
    connect(heightSpinBox, SIGNAL(valueChanged(int)), this, SLOT(adjustSizes()));

    QGridLayout *mainGbox = new QGridLayout;
    QLabel *origSizeLab = new QLabel(tr("Original size:"));
    QString imageSizeStr = QString::number(width) + " x " + QString::number(height);
    QLabel *origSizePixelsLab = new QLabel(imageSizeStr);
    QLabel *widthLab = new QLabel(tr("Width:"));
    QLabel *heightLab = new QLabel(tr("Height:"));
    QLabel *unitsLab = new QLabel(tr("Units:"));

    QLabel *newSizeLab = new QLabel(tr("New size:"));
    newSizePixelsLabel = new QLabel(imageSizeStr);

    pixelsRadioButton = new QRadioButton(tr("Pixels"));
    connect(pixelsRadioButton, SIGNAL(clicked()), this, SLOT(setUnits()));
    percentRadioButton = new QRadioButton(tr("Percent"));
    connect(percentRadioButton, SIGNAL(clicked()), this, SLOT(setUnits()));
    pixelsRadioButton->setChecked(true);
    pixelUnits = true;

    QCheckBox *lockAspectCb = new QCheckBox(tr("Lock aspect ratio"), this);
    lockAspectCb->setChecked(true);
    connect(lockAspectCb, SIGNAL(clicked()), this, SLOT(setAspectLock()));
    aspectLocked = true;

    QHBoxLayout *radiosHbox = new QHBoxLayout;
    radiosHbox->addStretch(1);
    radiosHbox->addWidget(pixelsRadioButton);
    radiosHbox->addWidget(percentRadioButton);

    mainGbox->addWidget(origSizeLab, 2, 2, 1, 1);
    mainGbox->addWidget(origSizePixelsLab, 2, 4, 1, 1);
    mainGbox->addWidget(widthLab, 6, 2, 1, 1);
    mainGbox->addWidget(heightLab, 7, 2, 1, 1);
    mainGbox->addWidget(unitsLab, 3, 2, 1, 1);
    mainGbox->addWidget(widthSpinBox, 6, 4, 1, 2);
    mainGbox->addWidget(heightSpinBox, 7, 4, 1, 2);
    mainGbox->addLayout(radiosHbox, 3, 4, 1, 3);
    mainGbox->addWidget(lockAspectCb, 5, 2, 1, 3);
    mainGbox->addWidget(newSizeLab, 8, 2, 1, 1);
    mainGbox->addWidget(newSizePixelsLabel, 8, 4, 1, 1);
    mainGbox->setRowStretch(9, 1);
    mainGbox->setColumnStretch(3, 1);

    QVBoxLayout *mainVbox = new QVBoxLayout;
    mainVbox->addLayout(mainGbox);
    mainVbox->addLayout(buttonsHbox);
    setLayout(mainVbox);
    widthSpinBox->setFocus(Qt::OtherFocusReason);
}

void ResizeDialog::setAspectLock() {
    aspectLocked = ((QCheckBox *) QObject::sender())->isChecked();
    adjustSizes();
}

void ResizeDialog::setUnits() {
    int newWidth;
    int newHeight;

    if (pixelsRadioButton->isChecked() && !pixelUnits) {
        newWidth = (width * widthSpinBox->value()) / 100;
        newHeight = (height * heightSpinBox->value()) / 100;
        widthSpinBox->setRange(0, width * 10);
        heightSpinBox->setRange(0, height * 10);
        pixelUnits = true;
    } else {
        newWidth = (100 * widthSpinBox->value()) / width;
        newHeight = (100 * heightSpinBox->value()) / height;
        widthSpinBox->setRange(0, 100 * 10);
        heightSpinBox->setRange(0, 100 * 10);
        pixelUnits = false;
    }

    widthSpinBox->setValue(newWidth);
    if (!aspectLocked)
        heightSpinBox->setValue(newHeight);
}

void ResizeDialog::adjustSizes() {
    static bool busy = false;
    if (busy)
        return;
    busy = true;

    if (aspectLocked) {
        if (pixelUnits) {
            QSize imageSize(width, height);
            if (widthSpinBox->value() > lastWidth || heightSpinBox->value() > lastHeight) {
                imageSize.scale(widthSpinBox->value(), heightSpinBox->value(), Qt::KeepAspectRatioByExpanding);
            } else {
                imageSize.scale(widthSpinBox->value(), heightSpinBox->value(), Qt::KeepAspectRatio);
            }

            widthSpinBox->setValue(imageSize.width());
            heightSpinBox->setValue(imageSize.height());
            lastWidth = widthSpinBox->value();
            lastHeight = heightSpinBox->value();
            newWidth = imageSize.width();
            newHeight = imageSize.height();
        } else {
            if (widthSpinBox->value() != lastWidth) {
                heightSpinBox->setValue(widthSpinBox->value());
            } else {
                widthSpinBox->setValue(heightSpinBox->value());
            }


            lastWidth = widthSpinBox->value();
            lastHeight = heightSpinBox->value();

            newWidth = (width * widthSpinBox->value()) / 100;
            newHeight = (height * heightSpinBox->value()) / 100;
        }
    } else {
        if (pixelUnits) {
            newWidth = widthSpinBox->value();
            newHeight = heightSpinBox->value();
        } else {
            newWidth = (width * widthSpinBox->value()) / 100;
            newHeight = (height * heightSpinBox->value()) / 100;
        }
    }

    newSizePixelsLabel->setText(QString::number(newWidth) + " x " + QString::number(newHeight));
    busy = false;
}

void ResizeDialog::ok() {
    if (newWidth || newHeight) {
        Settings::scaledWidth = newWidth;
        Settings::scaledHeight = newHeight;
        imageViewer->refresh();
    }
    accept();
}

void ResizeDialog::abort() {
    reject();
}

ColorsDialog::ColorsDialog(QWidget *parent, ImageViewer *imageViewer) : QDialog(parent) {
    setWindowTitle(tr("Colors"));
    setWindowIcon(QIcon(":/images/colors.png"));
    resize(350, 300);
    this->imageViewer = imageViewer;

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
    okButton->setDefault(true);

    /* hue saturation */
    QLabel *hueLab = new QLabel(tr("Hue"));
    QLabel *satLab = new QLabel(tr("Saturation"));
    QLabel *lightLab = new QLabel(tr("Lightness"));

    hueSlider = new QSlider(Qt::Horizontal);
    hueSlider->setTickPosition(QSlider::TicksAbove);
    hueSlider->setTickInterval(25);
    hueSlider->setRange(-100, 100);
    hueSlider->setTracking(false);
    connect(hueSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    colorizeCheckBox = new QCheckBox(tr("Colorize"), this);
    colorizeCheckBox->setCheckState(Settings::colorizeEnabled ? Qt::Checked : Qt::Unchecked);
    connect(colorizeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(enableColorize(int)));

    rNegateCheckBox = new QCheckBox(tr("Negative"), this);
    rNegateCheckBox->setCheckState(Settings::rNegateEnabled ? Qt::Checked : Qt::Unchecked);
    connect(rNegateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(redNegative(int)));

    gNegateCheckBox = new QCheckBox(tr("Negative"), this);
    gNegateCheckBox->setCheckState(Settings::gNegateEnabled ? Qt::Checked : Qt::Unchecked);
    connect(gNegateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(greenNegative(int)));

    bNegateCheckBox = new QCheckBox(tr("Negative"), this);
    bNegateCheckBox->setCheckState(Settings::bNegateEnabled ? Qt::Checked : Qt::Unchecked);
    connect(bNegateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(blueNegative(int)));

    saturationSlider = new QSlider(Qt::Horizontal);
    saturationSlider->setTickPosition(QSlider::TicksAbove);
    saturationSlider->setTickInterval(25);
    saturationSlider->setRange(-100, 100);
    saturationSlider->setTracking(false);
    connect(saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    lightnessSlider = new QSlider(Qt::Horizontal);
    lightnessSlider->setTickPosition(QSlider::TicksAbove);
    lightnessSlider->setTickInterval(25);
    lightnessSlider->setRange(-100, 100);
    lightnessSlider->setTracking(false);
    connect(lightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    QHBoxLayout *channelsHbox = new QHBoxLayout;
    redCheckBox = new QCheckBox(tr("Red"));
    redCheckBox->setCheckable(true);
    redCheckBox->setChecked(Settings::hueRedChannel);
    connect(redCheckBox, SIGNAL(clicked()), this, SLOT(setRedChannel()));
    channelsHbox->addWidget(redCheckBox, 0, Qt::AlignLeft);
    greenCheckBox = new QCheckBox(tr("Green"));
    greenCheckBox->setCheckable(true);
    greenCheckBox->setChecked(Settings::hueGreenChannel);
    connect(greenCheckBox, SIGNAL(clicked()), this, SLOT(setGreenChannel()));
    channelsHbox->addWidget(greenCheckBox, 0, Qt::AlignLeft);
    blueCheckBox = new QCheckBox(tr("Blue"));
    blueCheckBox->setCheckable(true);
    blueCheckBox->setChecked(Settings::hueBlueChannel);
    connect(blueCheckBox, SIGNAL(clicked()), this, SLOT(setBlueChannel()));
    channelsHbox->addWidget(blueCheckBox, 0, Qt::AlignLeft);
    channelsHbox->addStretch(1);

    QGridLayout *hueSatLay = new QGridLayout;
    hueSatLay->addWidget(hueLab, 1, 0, 1, 1);
    hueSatLay->addWidget(hueSlider, 1, 1, 1, 1);
    hueSatLay->addWidget(colorizeCheckBox, 2, 1, 1, 1);
    hueSatLay->addWidget(satLab, 3, 0, 1, 1);
    hueSatLay->addWidget(saturationSlider, 3, 1, 1, 1);
    hueSatLay->addWidget(lightLab, 4, 0, 1, 1);
    hueSatLay->addWidget(lightnessSlider, 4, 1, 1, 1);
    hueSatLay->setColumnMinimumWidth(0, 70);

    QGroupBox *hueSatGroup = new QGroupBox(tr("Hue and Saturation"));
    hueSatGroup->setLayout(hueSatLay);

    QGridLayout *channelsLay = new QGridLayout;
    channelsLay->addLayout(channelsHbox, 5, 1, 1, 1);
    channelsLay->setColumnMinimumWidth(0, 70);
    QGroupBox *channelsGroup = new QGroupBox(tr("Affected Channels"));
    channelsGroup->setLayout(channelsLay);

    /* brightness contrast */
    QLabel *brightLab = new QLabel(tr("Brightness"));
    QLabel *contrastLab = new QLabel(tr("Contrast"));

    brightSlider = new QSlider(Qt::Horizontal);
    brightSlider->setTickPosition(QSlider::TicksAbove);
    brightSlider->setTickInterval(25);
    brightSlider->setRange(-100, 100);
    brightSlider->setTracking(false);
    connect(brightSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    contrastSlider = new QSlider(Qt::Horizontal);
    contrastSlider->setTickPosition(QSlider::TicksAbove);
    contrastSlider->setTickInterval(25);
    contrastSlider->setRange(-100, 100);
    contrastSlider->setTracking(false);
    contrastSlider->setInvertedAppearance(true);
    connect(contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    QGridLayout *brightContrastbox = new QGridLayout;
    brightContrastbox->addWidget(brightLab, 1, 0, 1, 1);
    brightContrastbox->addWidget(brightSlider, 1, 1, 1, 1);
    brightContrastbox->addWidget(contrastLab, 2, 0, 1, 1);
    brightContrastbox->addWidget(contrastSlider, 2, 1, 1, 1);
    brightContrastbox->setColumnMinimumWidth(0, 70);

    QGroupBox *brightContrastGroup = new QGroupBox(tr("Brightness and Contrast"));
    brightContrastGroup->setLayout(brightContrastbox);

    /* Channel mixer */
    QLabel *redLab = new QLabel(tr("Red"));
    redSlider = new QSlider(Qt::Horizontal);
    redSlider->setTickPosition(QSlider::TicksAbove);
    redSlider->setTickInterval(25);
    redSlider->setRange(-100, 100);
    redSlider->setTracking(false);
    connect(redSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    QLabel *greenLab = new QLabel(tr("Green"));
    greenSlider = new QSlider(Qt::Horizontal);
    greenSlider->setTickPosition(QSlider::TicksAbove);
    greenSlider->setTickInterval(25);
    greenSlider->setRange(-100, 100);
    greenSlider->setTracking(false);
    connect(greenSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    QLabel *blueLab = new QLabel(tr("Blue"));
    blueSlider = new QSlider(Qt::Horizontal);
    blueSlider->setTickPosition(QSlider::TicksAbove);
    blueSlider->setTickInterval(25);
    blueSlider->setRange(-100, 100);
    blueSlider->setTracking(false);
    connect(blueSlider, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

    QGridLayout *channelMixbox = new QGridLayout;
    channelMixbox->addWidget(redLab, 1, 0, 1, 1);
    channelMixbox->addWidget(redSlider, 1, 1, 1, 1);
    channelMixbox->addWidget(rNegateCheckBox, 1, 2, 1, 1);
    channelMixbox->addWidget(greenLab, 2, 0, 1, 1);
    channelMixbox->addWidget(greenSlider, 2, 1, 1, 1);
    channelMixbox->addWidget(gNegateCheckBox, 2, 2, 1, 1);
    channelMixbox->addWidget(blueLab, 3, 0, 1, 1);
    channelMixbox->addWidget(blueSlider, 3, 1, 1, 1);
    channelMixbox->addWidget(bNegateCheckBox, 3, 2, 1, 1);
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

void ColorsDialog::applyColors(int) {
    if (brightSlider->value() >= 0)
        Settings::brightVal = (brightSlider->value() * 500 / 100) + 100;
    else
        Settings::brightVal = brightSlider->value() + 100;

    if (contrastSlider->value() >= 0)
        Settings::contrastVal = (contrastSlider->value() * 79 / 100) + 78;
    else
        Settings::contrastVal = contrastSlider->value() + 79;

    Settings::hueVal = hueSlider->value() * 127 / 100;

    if (saturationSlider->value() >= 0)
        Settings::saturationVal = (saturationSlider->value() * 500 / 100) + 100;
    else
        Settings::saturationVal = saturationSlider->value() + 100;

    if (lightnessSlider->value() >= 0)
        Settings::lightnessVal = (lightnessSlider->value() * 200 / 100) + 100;
    else
        Settings::lightnessVal = lightnessSlider->value() + 100;

    Settings::redVal = redSlider->value();
    Settings::greenVal = greenSlider->value();
    Settings::blueVal = blueSlider->value();

    imageViewer->refresh();
}

void ColorsDialog::ok() {
    Settings::dialogLastX = pos().x();
    Settings::dialogLastY = pos().y();
    accept();
}

void ColorsDialog::reset() {
    hueSlider->setValue(0);
    colorizeCheckBox->setChecked(false);
    rNegateCheckBox->setChecked(false);
    gNegateCheckBox->setChecked(false);
    bNegateCheckBox->setChecked(false);
    saturationSlider->setValue(0);
    lightnessSlider->setValue(0);
    redCheckBox->setChecked(true);
    greenCheckBox->setChecked(true);
    blueCheckBox->setChecked(true);
    Settings::hueRedChannel = true;
    Settings::hueGreenChannel = true;
    Settings::hueBlueChannel = true;

    contrastSlider->setValue(0);
    brightSlider->setValue(0);

    redSlider->setValue(0);
    greenSlider->setValue(0);
    blueSlider->setValue(0);

    imageViewer->refresh();
}

void ColorsDialog::enableColorize(int state) {
    Settings::colorizeEnabled = state;
    imageViewer->refresh();
}

void ColorsDialog::redNegative(int state) {
    Settings::rNegateEnabled = state;
    imageViewer->refresh();
}

void ColorsDialog::greenNegative(int state) {
    Settings::gNegateEnabled = state;
    imageViewer->refresh();
}

void ColorsDialog::blueNegative(int state) {
    Settings::bNegateEnabled = state;
    imageViewer->refresh();
}

void ColorsDialog::setRedChannel() {
    Settings::hueRedChannel = redCheckBox->isChecked();
    imageViewer->refresh();
}

void ColorsDialog::setGreenChannel() {
    Settings::hueGreenChannel = greenCheckBox->isChecked();
    imageViewer->refresh();
}

void ColorsDialog::setBlueChannel() {
    Settings::hueBlueChannel = blueCheckBox->isChecked();
    imageViewer->refresh();
}

void AppMgmtDialog::addTableModelItem(QStandardItemModel *model, QString &key, QString &val) {
    int atRow = model->rowCount();
    QStandardItem *itemKey = new QStandardItem(key);
    QStandardItem *itemKey2 = new QStandardItem(val);
    model->insertRow(atRow, itemKey);
    model->setItem(atRow, 1, itemKey2);
}

AppMgmtDialog::AppMgmtDialog(QWidget *parent) : QDialog(parent) {
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
    appsTable->setShowGrid(false);

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
    QMapIterator<QString, QString> it(Settings::externalApps);
    while (it.hasNext()) {
        it.next();
        key = it.key();
        val = it.value();
        addTableModelItem(appsTableModel, key, val);
    }
}

void AppMgmtDialog::ok() {
    int row = appsTableModel->rowCount();
    Settings::externalApps.clear();
    for (int i = 0; i < row; ++i) {
        if (!appsTableModel->itemFromIndex(appsTableModel->index(i, 1))->text().isEmpty()) {
            Settings::externalApps[appsTableModel->itemFromIndex(appsTableModel->index(i, 0))->text()] =
                    appsTableModel->itemFromIndex(appsTableModel->index(i, 1))->text();
        }
    }
    accept();
}

void AppMgmtDialog::add() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Application"), "", "");
    if (fileName.isEmpty())
        return;

    QFileInfo fileInfo = QFileInfo(fileName);
    QString appName = fileInfo.fileName();
    addTableModelItem(appsTableModel, appName, fileName);
}

void AppMgmtDialog::entry() {
    int atRow = appsTableModel->rowCount();
    QStandardItem *itemKey = new QStandardItem(QString(tr("New Application")));
    appsTableModel->insertRow(atRow, itemKey);
}

void AppMgmtDialog::remove() {
    QModelIndexList indexesList;
    while ((indexesList = appsTable->selectionModel()->selectedIndexes()).size()) {
        appsTableModel->removeRow(indexesList.first().row());
    }
}

CopyMoveToDialog::CopyMoveToDialog(QWidget *parent, QString thumbsPath, bool move) : QDialog(parent) {
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
    pathsTable->setShowGrid(false);

    connect(pathsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(selection(QItemSelection, QItemSelection)));
    connect(pathsTable, SIGNAL(doubleClicked(
                                       const QModelIndex &)),
            this, SLOT(pathDoubleClick(
                               const QModelIndex &)));

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
    okButton->setDefault(true);

    connect(okButton, SIGNAL(clicked()), this, SLOT(copyOrMove()));

    buttonsHbox->addStretch(1);
    buttonsHbox->addWidget(cancelButton, 0, Qt::AlignRight);
    buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

    destinationLabel = new QLabel(tr("Destination:"));
    QFrame *line = new QFrame(this);
    line->setObjectName(QString::fromUtf8("line"));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *mainVbox = new QVBoxLayout;
    mainVbox->addWidget(pathsTable);
    mainVbox->addLayout(addRemoveHbox);
    mainVbox->addWidget(line);
    mainVbox->addWidget(destinationLabel);
    mainVbox->addLayout(buttonsHbox);
    setLayout(mainVbox);

    // Load paths list
    QSetIterator<QString> it(Settings::bookmarkPaths);
    while (it.hasNext()) {
        QStandardItem *item = new QStandardItem(QIcon(":/images/bookmarks.png"), it.next());
        pathsTableModel->insertRow(pathsTableModel->rowCount(), item);
    }
    pathsTableModel->sort(0);
}

void CopyMoveToDialog::selection(const QItemSelection &, const QItemSelection &) {
    if (pathsTable->selectionModel()->selectedRows().size() > 0) {
        destinationLabel->setText(tr("Destination:") + " " +
                                  pathsTableModel->item(
                                          pathsTable->selectionModel()->selectedRows().at(0).row())->text());
    }
}

void CopyMoveToDialog::pathDoubleClick(const QModelIndex &) {
    copyOrMove();
}

void CopyMoveToDialog::savePaths() {
    Settings::bookmarkPaths.clear();
    for (int i = 0; i < pathsTableModel->rowCount(); ++i) {
        Settings::bookmarkPaths.insert
                (pathsTableModel->itemFromIndex(pathsTableModel->index(i, 0))->text());
    }
}

void CopyMoveToDialog::copyOrMove() {
    savePaths();

    QModelIndexList indexesList;
    if ((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
        selectedPath = pathsTableModel->itemFromIndex(indexesList.first())->text();
        accept();
    } else {
        reject();
    }
}

void CopyMoveToDialog::justClose() {
    savePaths();
    reject();
}

void CopyMoveToDialog::add() {
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

void CopyMoveToDialog::remove() {
    QModelIndexList indexesList;
    if ((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
        pathsTableModel->removeRow(indexesList.first().row());
    }
}

ProgressDialog::ProgressDialog(QWidget *parent) : QDialog(parent) {
    opLabel = new QLabel("");
    abortOp = false;

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

void ProgressDialog::abort() {
    abortOp = true;
}

