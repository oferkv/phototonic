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

    setFocusPolicy(Qt::ClickFocus);

    rubberband = new QRubberBand(QRubberBand::Rectangle, this);
    rubberband->setStyleSheet("background-color: rgb(255, 255, 255)");
    rubberband->show();
}

void CropRubberBand::showEvent(QShowEvent *) {
    setFocus();
}

void CropRubberBand::keyPressEvent(QKeyEvent *event) {
    QPoint cursorPosGlobal = QCursor::pos();
    QPoint cursorPos = mapFromGlobal(cursorPosGlobal);
    QRect geom = geometry();
qDebug() << "cursor" << cursorPos << "geom" << geom;
    switch (event->key()) {
    case Qt::LeftArrow:
    case Qt::Key_H:
        if (cursorPos.x() > -10 && cursorPos.x() < 10) {
            geom.setLeft(geom.left() - 1);
            QCursor::setPos(cursorPosGlobal.x() - 1, cursorPosGlobal.y());
        } else if (cursorPos.x() > width() - 10 && cursorPos.x() < width() + 10) {
            geom.setRight(geom.right() - 1);
            QCursor::setPos(cursorPosGlobal.x() - 1, cursorPosGlobal.y());
        } else if (QRect(geom).translated(-geom.x(), -geom.y()).contains(cursorPos)) {
            geom.moveLeft(geom.x() - 1);
        }
        setGeometry(geom);
        emit selectionChanged(geom);
        event->accept();
        break;
    case Qt::DownArrow:
    case Qt::Key_J:
        if (cursorPos.y() > -10 && cursorPos.y() < 10) {
            geom.setTop(geom.top() + 1);
            QCursor::setPos(cursorPosGlobal.x(), cursorPosGlobal.y() + 1);
        } else if (cursorPos.y() > height() - 10 && cursorPos.y() < height() + 10) {
            geom.setBottom(geom.bottom() + 1);
            QCursor::setPos(cursorPosGlobal.x(), cursorPosGlobal.y() + 1);
        } else if (QRect(geom).translated(-geom.x(), -geom.y()).contains(cursorPos)) {
            geom.moveTop(geom.top() + 1);
        }
        setGeometry(geom);
        emit selectionChanged(geom);
        event->accept();
        break;
    case Qt::UpArrow:
    case Qt::Key_K:
        if (cursorPos.y() > -10 && cursorPos.y() < 10) {
            geom.setTop(geom.top() - 1);
            QCursor::setPos(cursorPosGlobal.x(), cursorPosGlobal.y() - 1);
        } else if (cursorPos.y() > height() - 10 && cursorPos.y() < height() + 10) {
            geom.setBottom(geom.bottom() - 1);
            QCursor::setPos(cursorPosGlobal.x(), cursorPosGlobal.y() - 1);
        } else if (QRect(geom).translated(-geom.x(), -geom.y()).contains(cursorPos)) {
            geom.moveTop(geom.top() - 1);
        }
        setGeometry(geom);
        emit selectionChanged(geom);
        event->accept();
        break;
    case Qt::RightArrow:
    case Qt::Key_L:
        if (cursorPos.x() > -10 && cursorPos.x() < 10) {
            geom.setLeft(geom.left() + 1);
            QCursor::setPos(cursorPosGlobal.x() + 1, cursorPosGlobal.y());
        } else if (cursorPos.x() > width() - 10 && cursorPos.x() < width() + 10) {
            geom.setRight(geom.right() + 1);
            QCursor::setPos(cursorPosGlobal.x() + 1, cursorPosGlobal.y());
        } else if (QRect(geom).translated(-geom.x(), -geom.y()).contains(cursorPos)) {
            geom.moveLeft(geom.x() + 1);
        }
        setGeometry(geom);
        emit selectionChanged(geom);
        event->accept();
        break;
    }
}

void CropRubberBand::resizeEvent(QResizeEvent *) {
    rubberband->resize(size());
    emit selectionChanged(rubberband->geometry());
}

