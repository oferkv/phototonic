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

#include <QtWidgets>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/QtWidgets>
#include "ImageViewer.h"
#include "ColorsDialog.h"
#include "Settings.h"

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
    if (brightSlider->value() >= 0) {
        Settings::brightVal = (brightSlider->value() * 500 / 100) + 100;
    } else {
        Settings::brightVal = brightSlider->value() + 100;
    }

    if (contrastSlider->value() >= 0) {
        Settings::contrastVal = (contrastSlider->value() * 79 / 100) + 78;
    } else {
        Settings::contrastVal = contrastSlider->value() + 79;
    }

    Settings::hueVal = hueSlider->value() * 127 / 100;

    if (saturationSlider->value() >= 0) {
        Settings::saturationVal = (saturationSlider->value() * 500 / 100) + 100;
    } else {
        Settings::saturationVal = saturationSlider->value() + 100;
    }

    if (lightnessSlider->value() >= 0) {
        Settings::lightnessVal = (lightnessSlider->value() * 200 / 100) + 100;
    } else {
        Settings::lightnessVal = lightnessSlider->value() + 100;
    }

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
