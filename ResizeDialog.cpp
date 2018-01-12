/*
 *  Copyright (C) 2013-2018 Ofer Kashayov <oferkv@live.com>
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

#include <QtWidgets/QWidget>
#include "ImageViewer.h"
#include "ResizeDialog.h"
#include "Settings.h"

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
