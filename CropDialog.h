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

#ifndef CROP_DIALOG_H
#define CROP_DIALOG_H

#include <QtWidgets/QWidget>
#include "ImageViewer.h"

class CropDialog : public QDialog {
    Q_OBJECT

public:
    CropDialog(QWidget *parent, ImageViewer *imageViewer);

public slots:

    void ok();

    void reset();

    void applyCrop(int);

private:
    QSpinBox *topSpinBox;
    QSpinBox *bottomSpinBox;
    QSpinBox *leftSpinBox;
    QSpinBox *rightSpinBox;
    ImageViewer *imageViewer;
};

#endif // CROP_DIALOG_H