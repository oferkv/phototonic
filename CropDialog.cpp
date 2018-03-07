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

#include "CropDialog.h"
#include "Settings.h"

CropDialog::CropDialog(QWidget *parent, ImageViewer *imageViewer) : QDialog(parent) {
    setWindowTitle(tr("Cropping"));
    setWindowIcon(QIcon(":/images/crop.png"));
    resize(350, 100);
    if (Settings::dialogLastX)
        move(Settings::dialogLastX, Settings::dialogLastY);
    this->imageViewer = imageViewer;

    QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *resetButton = new QPushButton(tr("Reset"));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
    QPushButton *okButton = new QPushButton(tr("OK"));
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