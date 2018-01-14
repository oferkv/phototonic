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

#include "SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Preferences"));
    setWindowIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/phototonic.png")));

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

    // Wrap image list
    wrapListCheckBox = new QCheckBox(tr("Wrap image list when reaching last or first image"), this);
    wrapListCheckBox->setChecked(Settings::wrapImageList);

    // Save quality
    QLabel *saveQualityLabel = new QLabel(tr("Default quality when saving:"));
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
    enableExifCheckBox = new QCheckBox(tr("Rotate image according to Exif orientation value"), this);
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
    viewerOptsBox->addStretch(1);

    // thumbsViewer background color
    QLabel *thumbsBackgroundColorLabel = new QLabel(tr("Thumbnails and Preview Background Color:"));
    thumbsColorPickerButton = new QToolButton();
    thumbsColorPickerButton->setFixedSize(48, 24);
    QHBoxLayout *thumbsBackgroundColorLayout = new QHBoxLayout;
    thumbsBackgroundColorLayout->addWidget(thumbsBackgroundColorLabel);
    thumbsBackgroundColorLayout->addWidget(thumbsColorPickerButton);
    thumbsBackgroundColorLayout->addStretch(1);
    connect(thumbsColorPickerButton, SIGNAL(clicked()), this, SLOT(pickThumbsColor()));
    setButtonBgColor(Settings::thumbsBackgroundColor, thumbsColorPickerButton);
    thumbsColorPickerButton->setAutoFillBackground(true);
    thumbsBackgroundColor = Settings::thumbsBackgroundColor;

    // thumbsViewer text color
    QLabel *thumbLabelColorLabel = new QLabel(tr("Label color:"));
    thumbsLabelColorButton = new QToolButton();
    thumbsLabelColorButton->setFixedSize(48, 24);
    QHBoxLayout *thumbsLabelColorLayout = new QHBoxLayout;
    thumbsLabelColorLayout->addWidget(thumbLabelColorLabel);
    thumbsLabelColorLayout->addWidget(thumbsLabelColorButton);
    thumbsLabelColorLayout->addStretch(1);
    connect(thumbsLabelColorButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
    setButtonBgColor(Settings::thumbsTextColor, thumbsLabelColorButton);
    thumbsLabelColorButton->setAutoFillBackground(true);
    thumbsTextColor = Settings::thumbsTextColor;

    // thumbsViewer background image
    QLabel *thumbsBackgroundImageLabel = new QLabel(tr("Background image:"));
    thumbsBackgroundImageLineEdit = new QLineEdit;
    thumbsBackgroundImageLineEdit->setClearButtonEnabled(true);
    thumbsBackgroundImageLineEdit->setMinimumWidth(200);

    QToolButton *chooseThumbsBackImageButton = new QToolButton();
    chooseThumbsBackImageButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    chooseThumbsBackImageButton->setFixedSize(26, 26);
    chooseThumbsBackImageButton->setIconSize(QSize(16, 16));
    connect(chooseThumbsBackImageButton, SIGNAL(clicked()), this, SLOT(pickBgImage()));

    QHBoxLayout *thumbsBackgroundImageLayout = new QHBoxLayout;
    thumbsBackgroundImageLayout->addWidget(thumbsBackgroundImageLabel);
    thumbsBackgroundImageLayout->addWidget(thumbsBackgroundImageLineEdit);
    thumbsBackgroundImageLayout->addWidget(chooseThumbsBackImageButton);
    thumbsBackgroundImageLayout->addStretch(1);
    thumbsBackgroundImageLineEdit->setText(Settings::thumbsBackImage);

    // Thumbnail pages to read ahead
    QLabel *thumbsPagesReadLabel = new QLabel(tr("Number of thumbnail pages to read ahead:"));
    thumbPagesSpinBox = new QSpinBox;
    thumbPagesSpinBox->setRange(1, 10);
    thumbPagesSpinBox->setValue(Settings::thumbPagesReadahead);
    QHBoxLayout *thumbPagesReadLayout = new QHBoxLayout;
    thumbPagesReadLayout->addWidget(thumbsPagesReadLabel);
    thumbPagesReadLayout->addWidget(thumbPagesSpinBox);
    thumbPagesReadLayout->addStretch(1);

    enableThumbExifCheckBox = new QCheckBox(tr("Rotate thumbnail according to Exif orientation value"), this);
    enableThumbExifCheckBox->setChecked(Settings::exifThumbRotationEnabled);

    // Thumbnail options
    QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
    thumbsOptsBox->addLayout(thumbsBackgroundColorLayout);

    thumbsOptsBox->addLayout(thumbsBackgroundImageLayout);
    thumbsOptsBox->addLayout(thumbsLabelColorLayout);
    thumbsOptsBox->addWidget(enableThumbExifCheckBox);
    thumbsOptsBox->addLayout(thumbPagesReadLayout);
    thumbsOptsBox->addStretch(1);

    // Mouse settings
    reverseMouseCheckBox = new QCheckBox(tr("Swap mouse double-click and middle-click actions"), this);
    reverseMouseCheckBox->setChecked(Settings::reverseMouseBehavior);

    // Delete confirmation setting
    deleteConfirmCheckBox = new QCheckBox(tr("Delete confirmation"), this);
    deleteConfirmCheckBox->setChecked(Settings::deleteConfirm);

    // Startup directory
    QGroupBox *startupDirGroupBox = new QGroupBox(tr("Startup directory if not specified by command line"));
    startupDirectoryRadioButtons[Settings::RememberLastDir] = new QRadioButton(tr("Remember last"));
    startupDirectoryRadioButtons[Settings::DefaultDir] = new QRadioButton(tr("Default"));
    startupDirectoryRadioButtons[Settings::SpecifiedDir] = new QRadioButton(tr("Specify:"));

    startupDirLineEdit = new QLineEdit;
    startupDirLineEdit->setClearButtonEnabled(true);
    startupDirLineEdit->setMinimumWidth(300);
    startupDirLineEdit->setMaximumWidth(400);

    QToolButton *chooseStartupDirButton = new QToolButton();
    chooseStartupDirButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    chooseStartupDirButton->setFixedSize(26, 26);
    chooseStartupDirButton->setIconSize(QSize(16, 16));
    connect(chooseStartupDirButton, SIGNAL(clicked()), this, SLOT(pickStartupDir()));

    QHBoxLayout *startupDirectoryLayout = new QHBoxLayout;
    startupDirectoryLayout->addWidget(startupDirectoryRadioButtons[2]);
    startupDirectoryLayout->addWidget(startupDirLineEdit);
    startupDirectoryLayout->addWidget(chooseStartupDirButton);
    startupDirectoryLayout->addStretch(1);

    QVBoxLayout *startupDirectoryMainLayout = new QVBoxLayout;
    for (int i = 0; i < 2; ++i) {
        startupDirectoryMainLayout->addWidget(startupDirectoryRadioButtons[i]);
        startupDirectoryRadioButtons[i]->setChecked(false);
    }
    startupDirectoryMainLayout->addLayout(startupDirectoryLayout);
    startupDirectoryMainLayout->addStretch(1);
    startupDirGroupBox->setLayout(startupDirectoryMainLayout);

    if (Settings::startupDir == Settings::SpecifiedDir) {
        startupDirectoryRadioButtons[Settings::SpecifiedDir]->setChecked(true);
    } else if (Settings::startupDir == Settings::RememberLastDir) {
        startupDirectoryRadioButtons[Settings::RememberLastDir]->setChecked(true);
    } else {
        startupDirectoryRadioButtons[Settings::DefaultDir]->setChecked(true);
    }
    startupDirLineEdit->setText(Settings::specifiedStartDir);

    // Keyboard shortcuts
    ShortcutsTable *shortcutsTable = new ShortcutsTable();
    shortcutsTable->refreshShortcuts();
    QGroupBox *keyboardGroupBox = new QGroupBox(tr("Keyboard"));
    QVBoxLayout *keyboardSettingsLayout = new QVBoxLayout;

    QHBoxLayout *filterShortcutsLayout = new QHBoxLayout;
    QLineEdit *shortcutsFilterLineEdit = new QLineEdit;
    shortcutsFilterLineEdit->setClearButtonEnabled(true);
    shortcutsFilterLineEdit->setPlaceholderText(tr("Filter Items"));
    connect(shortcutsFilterLineEdit, SIGNAL(textChanged(
                                                    const QString&)), shortcutsTable, SLOT(setFilter(
                                                                                                   const QString&)));
    keyboardSettingsLayout->addWidget(new QLabel(tr("Select an entry and press a key to set a new shortcut")));
    keyboardSettingsLayout->addWidget(shortcutsFilterLineEdit);
    keyboardSettingsLayout->addWidget(shortcutsTable);
    keyboardSettingsLayout->addLayout(filterShortcutsLayout);
    keyboardGroupBox->setLayout(keyboardSettingsLayout);

    QVBoxLayout *generalSettingsLayout = new QVBoxLayout;
    generalSettingsLayout->addWidget(reverseMouseCheckBox);
    generalSettingsLayout->addWidget(deleteConfirmCheckBox);
    generalSettingsLayout->addWidget(startupDirGroupBox);

    // Slide show delay
    QLabel *slideDelayLab = new QLabel(tr("Delay between slides in seconds:"));
    slideDelaySpinBox = new QSpinBox;
    slideDelaySpinBox->setRange(1, 3600);
    slideDelaySpinBox->setValue(Settings::slideShowDelay);
    QHBoxLayout *slideDelayLayout = new QHBoxLayout;
    slideDelayLayout->addWidget(slideDelayLab);
    slideDelayLayout->addWidget(slideDelaySpinBox);
    slideDelayLayout->addStretch(1);

    // Slide show random
    slideRandomCheckBox = new QCheckBox(tr("Show random images"), this);
    slideRandomCheckBox->setChecked(Settings::slideShowRandom);

    // Slide show options
    QVBoxLayout *slideshowLayout = new QVBoxLayout;
    slideshowLayout->addLayout(slideDelayLayout);
    slideshowLayout->addWidget(slideRandomCheckBox);
    slideshowLayout->addStretch(1);

    QGroupBox *slideshowGroupBox = new QGroupBox(tr("Slide Show"));
    slideshowGroupBox->setLayout(slideshowLayout);
    generalSettingsLayout->addWidget(slideshowGroupBox);
    generalSettingsLayout->addStretch(1);

    /* Confirmation buttons */
    QHBoxLayout *confirmSettingsLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
    okButton->setDefault(true);
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    closeButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(abort()));
    confirmSettingsLayout->addWidget(closeButton, 1, Qt::AlignRight);
    confirmSettingsLayout->addWidget(okButton, 0, Qt::AlignRight);

    /* Tabs */
    QTabWidget *settingsTabs = new QTabWidget;

    QWidget *viewerSettings = new QWidget;
    viewerSettings->setLayout(viewerOptsBox);
    settingsTabs->addTab(viewerSettings, tr("Viewer"));

    QWidget *thumbSettings = new QWidget;
    thumbSettings->setLayout(thumbsOptsBox);
    settingsTabs->addTab(thumbSettings, tr("Thumbnails"));

    QWidget *generalSettings = new QWidget;
    generalSettings->setLayout(generalSettingsLayout);
    settingsTabs->addTab(generalSettings, tr("General"));

    QWidget *keyboardSettings = new QWidget;
    keyboardSettings->setLayout(keyboardSettingsLayout);
    settingsTabs->addTab(keyboardSettings, tr("Keyboard"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(settingsTabs);
    mainLayout->addLayout(confirmSettingsLayout);
    setLayout(mainLayout);
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

    if (startupDirectoryRadioButtons[Settings::RememberLastDir]->isChecked()) {
        Settings::startupDir = Settings::RememberLastDir;
    } else if (startupDirectoryRadioButtons[Settings::DefaultDir]->isChecked()) {
        Settings::startupDir = Settings::DefaultDir;
    } else {
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
        setButtonBgColor(userColor, thumbsLabelColorButton);
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
