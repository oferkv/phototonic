/*
 *  Copyright (C) 2013-2014 Ofer Kashayov - oferkv@live.com
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

#include "CropRubberband.h"

CropRubberBand::CropRubberBand(QWidget *parent) : QWidget(parent) {

    setWindowFlags(Qt::SubWindow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    QSizeGrip *grip1 = new QSizeGrip(this);
    QSizeGrip *grip2 = new QSizeGrip(this);
    QSizeGrip *grip3 = new QSizeGrip(this);
    QSizeGrip *grip4 = new QSizeGrip(this);

    grip1->setStyleSheet("background-color: rgba(0, 0, 0, 0%)");
    grip2->setStyleSheet("background-color: rgba(0, 0, 0, 0%)");
    grip3->setStyleSheet("background-color: rgba(0, 0, 0, 0%)");
    grip4->setStyleSheet("background-color: rgba(0, 0, 0, 0%)");

    topLayout->addWidget(grip1, 0, Qt::AlignTop | Qt::AlignLeft);
    topLayout->addWidget(grip2, 1, Qt::AlignTop | Qt::AlignRight);
    bottomLayout->addWidget(grip3, 0, Qt::AlignBottom | Qt::AlignLeft);
    bottomLayout->addWidget(grip4, 1, Qt::AlignBottom | Qt::AlignRight);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(bottomLayout);

    rubberband = new QRubberBand(QRubberBand::Rectangle, this);
    rubberband->setStyleSheet("background-color: rgb(255, 255, 255)");
    rubberband->show();
}

void CropRubberBand::resizeEvent(QResizeEvent *) {
    rubberband->resize(size());
    emit selectionChanged(rubberband->geometry());
}

