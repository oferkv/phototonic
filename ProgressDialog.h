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

#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H

#include <QtWidgets/qlabel.h>
#include <QtWidgets/QWidget>
#include <QtWidgets>
#include "Settings.h"

class ProgressDialog : public QDialog {
    Q_OBJECT

public slots:

    void abort();

public:
    QLabel *opLabel;
    bool abortOp;

    ProgressDialog(QWidget *parent);

private:
    QPushButton *cancelButton;
};

#endif // PROGRESS_DIALOG_H
